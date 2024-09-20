import os
import subprocess
import argparse

def flash_esp32(prefix, port, baud_rate=460800):
    # Define the filenames
    bootloader_file = f"{prefix}_bootloader.bin"
    partition_table_file = f"{prefix}_partition-table.bin"
    lorahub_file = f"{prefix}_lorahub.bin"

    # Print the filenames
    print("Files to be flashed:")
    print(f"  Bootloader: {bootloader_file}")
    print(f"  Partition Table: {partition_table_file}")
    print(f"  LoraHub: {lorahub_file}")

    # Check if the files exist
    if not (os.path.isfile(bootloader_file) and os.path.isfile(partition_table_file) and os.path.isfile(lorahub_file)):
        print("One or more files are missing.")
        return

    # Define the esptool.py command
    cmd = [
        'esptool.py',
        '--chip', 'esp32s3',
        '-p', port,
        '-b', str(baud_rate),
        '--before=default_reset',
        '--after=hard_reset',
        'write_flash',
        '--flash_mode', 'dio',
        '--flash_freq', '80m',
        '--flash_size', '2MB',
        '0x0', bootloader_file,
        '0x10000', lorahub_file,
        '0x8000', partition_table_file
    ]

    # Print the esptool command
    print("\nExecuting command:")
    print(' '.join(cmd))

    # Run the command
    try:
        subprocess.run(cmd, check=True)
        print("Flashing completed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"An error occurred while flashing the ESP32: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Flash ESP32 with specified files.")
    parser.add_argument('prefix', type=str, help="The prefix of the filenames")
    parser.add_argument('port', type=str, help="The port to which the ESP32 is connected")

    args = parser.parse_args()

    flash_esp32(args.prefix, args.port)
