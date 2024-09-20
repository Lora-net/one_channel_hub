import requests
import json
import argparse
import sys
import time


# Try to import colorama for colored output, fall back to plain text if not available
try:
    from colorama import init, Fore, Style
    init(autoreset=True)
    COLORAMA_AVAILABLE = True
except ImportError:
    COLORAMA_AVAILABLE = False

# Define color constants based on the availability of colorama
if COLORAMA_AVAILABLE:
    COLOR_CYAN = Fore.CYAN
    COLOR_GREEN = Fore.GREEN
    COLOR_YELLOW = Fore.YELLOW
    COLOR_MAGENTA = Fore.MAGENTA
    COLOR_RED = Fore.RED
    COLOR_RESET = Style.RESET_ALL
else:
    COLOR_CYAN = ""
    COLOR_GREEN = ""
    COLOR_YELLOW = ""
    COLOR_MAGENTA = ""
    COLOR_RED = ""
    COLOR_RESET = ""

    # Information message on how to install colorama
    install_command = "pip install colorama"
    print("Colorama is not installed. To get colored output, install it using the following command:")
    print(f"  {install_command}")

def log_test_step(step_number, description, url, method, headers=None, data=None):
    """
    Log the details of each test step.

    Args:
        step_number (int): The test step number.
        description (str): Description of the test step.
        url (str): The URL being requested.
        method (str): HTTP method being used.
        headers (dict): Headers sent in the request.
        data (dict): Data sent in the request.
    """
    print(f"\n{COLOR_CYAN}Test Step {step_number}: {description}{COLOR_RESET}")
    print(f"{COLOR_GREEN}HTTP Method: {method}{COLOR_RESET}")
    print(f"{COLOR_GREEN}URL: {url}{COLOR_RESET}")
    if headers:
        print(f"{COLOR_YELLOW}Headers: {json.dumps(headers, indent=2)}{COLOR_RESET}")
    if data:
        print(f"{COLOR_YELLOW}Data: {json.dumps(data, indent=2)}{COLOR_RESET}")

def set_config(base_url, config, step_number):
    """
    Set the configuration of the LoRaHub.

    Args:
        base_url (str): The base URL of the LoRaHub API.
        config (dict): Configuration settings to be written to the device.
        step_number (int): The test step number.

    Returns:
        response (requests.Response): The response from the server.
    """
    url = f"{base_url}/api/v1/set_config"
    headers = {'Content-Type': 'application/json'}
    log_test_step(step_number, "Set Configuration", url, "POST", headers, config)
    response = requests.post(url, headers=headers, data=json.dumps(config))
    return response

def reboot(base_url, step_number):
    """
    Trigger a reboot of the LoRaHub.

    Args:
        base_url (str): The base URL of the LoRaHub API.
        step_number (int): The test step number.

    Returns:
        response (requests.Response): The response from the server.
    """
    url = f"{base_url}/api/v1/reboot"
    log_test_step(step_number, "Reboot Device", url, "POST")
    response = requests.post(url)
    return response

def get_config(base_url, step_number):
    """
    Get the current configuration of the LoRaHub.

    Args:
        base_url (str): The base URL of the LoRaHub API.
        step_number (int): The test step number.

    Returns:
        response (requests.Response): The response from the server containing the current configuration.
    """
    url = f"{base_url}/api/v1/get_config"
    log_test_step(step_number, "Get Configuration", url, "GET")
    response = requests.get(url)
    return response

def get_info(base_url, step_number):
    """
    Get information of the LoRaHub.

    Args:
        base_url (str): The base URL of the LoRaHub API.
        step_number (int): The test step number.

    Returns:
        response (requests.Response): The response from the server containing the information.
    """
    url = f"{base_url}/api/v1/get_info"
    log_test_step(step_number, "Get Information", url, "GET")
    response = requests.get(url)
    return response

def parse_arguments():
    """
    Parse command line arguments.

    Returns:
        argparse.Namespace: Parsed arguments.
    """
    parser = argparse.ArgumentParser(description="LoRaHub Configuration Script")
    parser.add_argument('--ip_address', type=str, required=True, help="IP address of the LoRaHub")
    parser.add_argument('--lns_addr', type=str, default="eu1.cloud.thethings.network", help="LNS address")
    parser.add_argument('--lns_port', type=int, default=1700, help="LNS port")
    parser.add_argument('--chan_freq', type=float, default=868.1, help="Channel frequency")
    parser.add_argument('--chan_dr', type=int, default=7, help="Channel data rate")
    parser.add_argument('--chan_bw', type=int, default=125, help="Channel bandwidth")
    parser.add_argument('--sntp_addr', type=str, default="pool.ntp.org", help="SNTP address")
    return parser.parse_args()

def print_response(response):
    """
    Print the response from the server, handling JSON and non-JSON responses.

    Args:
        response (requests.Response): The response from the server.
    """
    try:
        json_response = response.json()
        print(f"{COLOR_MAGENTA}Response JSON:{COLOR_RESET}", json.dumps(json_response, indent=2))
    except ValueError:
        print(f"{COLOR_MAGENTA}Response Text:{COLOR_RESET}", response.text)

def wait_for_server(base_url, timeout=120):
    """
    Wait for the server to come back online after a reboot.

    Args:
        base_url (str): The base URL of the LoRaHub API.
        timeout (int): Maximum time to wait for the server to be available (in seconds).

    Returns:
        bool: True if the server is back online within the timeout period, False otherwise.
    """
    url = f"{base_url}/api/v1/get_config"
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            response = requests.get(url)
            if response.status_code == 200:
                return True
        except requests.exceptions.RequestException:
            print(f"{COLOR_YELLOW}Waiting for server... (elapsed: {int(time.time() - start_time)}s){COLOR_RESET}")
        time.sleep(2)
    return False

# Example usage
if __name__ == "__main__":
    args = parse_arguments()

    base_url = f"http://{args.ip_address}:8000"

    # Create configuration dictionary from provided arguments
    config = {
        "lns_addr": args.lns_addr,
        "lns_port": args.lns_port,
        "chan_freq": args.chan_freq,
        "chan_dr": args.chan_dr,
        "chan_bw": args.chan_bw,
        "sntp_addr": args.sntp_addr
    }

    step_number = 1

    # Get information
    response = get_info(base_url, step_number)
    if response.status_code != 200:
        print(f"{COLOR_RED}Get Info Response: {response.status_code}{COLOR_RESET}")
        print_response(response)
        sys.exit(1)
    print(f"{COLOR_GREEN}Get Info Response: {response.status_code}{COLOR_RESET}")
    print_response(response)
    step_number += 1

    # Set the configuration
    response = set_config(base_url, config, step_number)
    if response.status_code != 200:
        print(f"{COLOR_RED}Set Config Response: {response.status_code}{COLOR_RESET}")
        print_response(response)
        sys.exit(1)
    print(f"{COLOR_GREEN}Set Config Response: {response.status_code}{COLOR_RESET}")
    print_response(response)
    step_number += 1

    # Reboot the device
    response = reboot(base_url, step_number)
    if response.status_code != 200:
        print(f"{COLOR_RED}Reboot Response: {response.status_code}{COLOR_RESET}")
        print_response(response)
        sys.exit(1)
    print(f"{COLOR_GREEN}Reboot Response: {response.status_code}{COLOR_RESET}")
    print_response(response)
    step_number += 1

    # Add a small delay to allow the server to start rebooting
    print(f"{COLOR_YELLOW}Waiting for the server to start rebooting...{COLOR_RESET}")
    time.sleep(10)

    # Wait for the server to come back online
    if not wait_for_server(base_url, timeout=120):
        print(f"{COLOR_RED}Error: Server did not come back online within the timeout period.{COLOR_RESET}")
        sys.exit(1)

    # Get the current configuration
    response = get_config(base_url, step_number)
    if response.status_code != 200:
        print(f"{COLOR_RED}Get Config Response: {response.status_code}{COLOR_RESET}")
        print_response(response)
        sys.exit(1)
    print(f"{COLOR_GREEN}Get Config Response: {response.status_code}{COLOR_RESET}")
    print_response(response)
