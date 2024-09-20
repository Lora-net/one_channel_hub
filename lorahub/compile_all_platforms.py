#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import argparse

# Define the configurations
configurations = {
    "heltec_wifi_lora_32_v3": """
# Build options for Heltec WiFi LoRa 32 v3
CONFIG_HELTEC_WIFI_LORA_32_V3=y
CONFIG_RADIO_TYPE_SX1262=y
CONFIG_GATEWAY_DISPLAY=y
""",
    "semtech_devkit_sx1261": """
# Build options for Semtech devkit with SX1261 radio
CONFIG_SEMTECH_DEVKIT=y
CONFIG_RADIO_TYPE_SX1261=y
CONFIG_GATEWAY_DISPLAY=y
""",
    "semtech_devkit_sx1262": """
# Build options for Semtech devkit with SX1262 radio
CONFIG_SEMTECH_DEVKIT=y
CONFIG_RADIO_TYPE_SX1262=y
CONFIG_GATEWAY_DISPLAY=y
""",
    "semtech_devkit_sx1268": """
# Build options for Semtech devkit with SX1268 radio
CONFIG_SEMTECH_DEVKIT=y
CONFIG_RADIO_TYPE_SX1268=y
CONFIG_GATEWAY_DISPLAY=y
""",
    "semtech_devkit_llcc68": """
# Build options for Semtech devkit with LLCC68 radio
CONFIG_SEMTECH_DEVKIT=y
CONFIG_RADIO_TYPE_LLCC68=y
CONFIG_GATEWAY_DISPLAY=n
""",
    "semtech_devkit_lr1121": """
# Build options for Semtech devkit with LR1121 radio
CONFIG_SEMTECH_DEVKIT=y
CONFIG_RADIO_TYPE_LR1121=y
CONFIG_GATEWAY_DISPLAY=y
"""
}

# Function to write the sdkconfig.defaults file
def write_sdkconfig_defaults(base_config, extra_config, config_name):
    filename = f"sdkconfig.defaults.{config_name}"
    with open(filename, "w") as file:
        file.write(base_config)
        file.write(extra_config)
    shutil.copyfile(filename, "sdkconfig.defaults")

# Function to build the project
def build_project():
    # Delete the existing sdkconfig file
    if os.path.exists("sdkconfig"):
        os.remove("sdkconfig")

    subprocess.run(["idf.py", "fullclean"], check=True)
    subprocess.run(["idf.py", "build"], check=True)

# Function to copy build files to the specified directory
def copy_build_files(build_dir, dest_dir, config_name):
    files_to_copy = {
        "lorahub.bin": f"{config_name}_lorahub.bin",
        "partition_table/partition-table.bin": f"{config_name}_partition-table.bin",
        "bootloader/bootloader.bin": f"{config_name}_bootloader.bin"
    }

    for src, dest in files_to_copy.items():
        src_path = os.path.join(build_dir, src)
        dest_path = os.path.join(dest_dir, dest)
        if not os.path.exists(os.path.dirname(dest_path)):
            os.makedirs(os.path.dirname(dest_path))
        shutil.copyfile(src_path, dest_path)

# Parse command line arguments
parser = argparse.ArgumentParser(description="Build LoRaHub project with multiple configurations.")
parser.add_argument("output_dir", help="Directory to copy the precompiled binaries.")
args = parser.parse_args()

# Backup the original sdkconfig.defaults
shutil.copyfile("sdkconfig.defaults", "sdkconfig.defaults.base")

# Read the base configuration (common settings)
with open("sdkconfig.defaults.base", "r") as file:
    base_config = file.read()

try:
    # Loop through each configuration and build the project
    for config_name, config in configurations.items():
        print(f"Building with configuration: {config_name}")

        # Write the sdkconfig.defaults with the specific configuration
        write_sdkconfig_defaults(base_config, config, config_name)

        # Build the project
        build_project()

        # Save the sdkconfig file used for this build
        sdkconfig_filename = f"sdkconfig.{config_name}"
        shutil.copyfile("sdkconfig", sdkconfig_filename)

        # Optionally, copy the build artifacts to a specific directory
        build_dir = f"build_{config_name}"
        if os.path.exists(build_dir):
            shutil.rmtree(build_dir)
        shutil.copytree("build", build_dir)

        # Copy the specified files to the output directory if provided
        if args.output_dir:
            copy_build_files(build_dir, args.output_dir, config_name)

        print(f"Build for {config_name} completed and stored in {build_dir}")

        # Restore the original sdkconfig.defaults
        shutil.copyfile("sdkconfig.defaults.base", "sdkconfig.defaults")

except subprocess.CalledProcessError as e:
    print(f"Build failed for configuration: {config_name}. Error: {e}")
    print("Stopping the script.")
    sys.exit(1)

print("All builds completed and original sdkconfig.defaults restored.")
