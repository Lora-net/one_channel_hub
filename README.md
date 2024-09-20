      ______                              _
     / _____)             _              | |
    ( (____  _____ ____ _| |_ _____  ____| |__
     \____ \| ___ |    (_   _) ___ |/ ___)  _ \
     _____) ) ____| | | || |_| ____( (___| | | |
    (______/|_____)_|_|_| \__)_____)\____)_| |_|
      (C)2024 Semtech

One-Channel Hub project (LoRaHub)
=================================

This project demonstrates how to implement a single channel LoRaWAN "gateway"
(called One-Channel Hub) using an Espressif chip as host MCU and a Semtech LoRa
radio.

This project contains the following components:
* `bin`: contains precompiled binaries for all supported platforms.
* `components/liblorahub`: the Hardware Abstraction Layer (HAL) on top of the radio driver.
* `components/radio_drivers`: the sx126x, llcc68, lr11xx radio drivers and associated hardware abstraction layer.
* `components/smtc_ral`: the radio abstraction layer and board/shields definitions.
* `doc`: various documentation for operating a local LoRaWAN network with a one channel hub.
* `lorahub/main`:
    * a packet forwarder implementation to connect the One-Channel Hub to a LoRaWAN network server, using Semtech UDP protocol (see PROTOCOL.md file).
    * connectivity and configuration: an example implementation for WiFi provisioning, and One-Channel Hub configuration through an HTTP server (web interface and RESTful API).
    * an example of OLED display handling for platforms supporting it.
* `tests`: python scripts for testing (HTTP Rest API, ....)
* `tools\util_net_downlink`: utility for packet logging, downlink testing, through packet forwarder UDP protocol.

# 1. Components

## 1.1. liblorahub

The Hardware Abstraction Layer (HAL) sits on top of the radio driver to and aims
to handle proper configuration for RX and TX for One-Channel Hub needs.

The HAL API is similar to the one used for LoRa gateways (sx130x) and exposes
the following functions:
* `lgw_rxrf_setconf()`: set radio parameters
* `lgw_rxif_setconf()`: set modulation parameters
* `lgw_start()`: connect the host to the radio (SPI) and configure the radio for RX
* `lgw_stop()`: stop the radio
* `lgw_receive()`: check for received packet
* `lgw_send()`: send a packet and configure the radio back to RX after TX done
* `lgw_status()`: returns current hub status (free, emitting, ...)
* `lgw_get_instcnt()`: returns the current hub internal counter value
* `lgw_time_on_air()`: computes the time on air of a packet
* `lgw_get_min_max_freq_hz()`: returns minimum and maximum frequency supported by the radio.
* `lgw_get_min_max_power_dbm()`: returns minimum and maximum TX power supported by the radio.

The HAL is responsible for timestamping received uplinks as accurately as
possible to enable timely downlink responses to the end device.
For this, the HAL configures the radio to raise an interrupt when a packet is
received. When the interrupt is raised, the HAL retrieves the current
One-Channel Hub counter value and returns. The received packet is retrieved when
the user calls lgw_receive(). A compensation will be applied to take into
account processing delays.

## 1.2. radio drivers & hal

This project relies on the official Semtech's radio drivers for sx126x, llcc68
and lr11xx radio chips.

The drivers are available on Github:
* https://github.com/Lora-net/sx126x_driver
* https://github.com/Lora-net/llcc68_driver
* https://github.com/Lora-net/SWDR001

As usual, a radio HAL is implemented to define how to interact with the radios
depending on the platform it is used with (reset, SPI transfer, ...).

The Semtech Radio Abstraction Layer (smtc_ral) is also provided to abstract
radio usage.

# 2. Main application & Packet Forwarder

The main application handles the following:
* set WiFi provisioning mode via BLE if requested by the user
* start the packet forwarder.
* configure the display.
* start the HTTP server for configuration (channels, LNS address, ...).

## 2.1. Supported platforms

* Heltec WiFi LoRa 32 v3 (sx1262)
* Semtech One-Channel Hub devkit (sx1261, sx1262, sx1268, llcc68, lr1121)

Concerning the Semtech One-Channel Hub devkit, one Semtech radio shield is
supported for each radio type:

| ID     | Region | Radio Shield      | Components                 |
|--------|--------|-------------------|----------------------------|
| Sx1261 | EU868  | SX1261MB1BAS      | E406V03A, XTAL (*), DC-DC  |
| Sx1262 | US915  | SX1262MB1CAS      | E428V03A, XTAL    , LDO    |
| Sx1268 | CN470  | SX1268MB1GAS      | E512V01A, XTAL (*), LDO    |
| LLCC68 | EU868  | LLCC68MB2BAS (**) | E568V01A, XTAL (*), LDO    |
| LR1121 | EU868  | LR1121MB1DIS      | E655V01A, XTAL (*)         |
| LR1121 | US915  | LR1121MB1DIS      | E655V01A, XTAL             |
| LR1121 | CN470  | LR1121MB1GIS      | E655V01A, XTAL (*)         |

(*) Most regions outside of USA, Canada and Australia employ LoRa 125kHz,
generally with a channel spacing of 200kHz. With LoRa 125 kHz being employed,
it is required to use an accurate frequency reference on the One Channel Hub in
order to respect the frequency error tolerance of the technology. A Hub designed
for these regions should therefore employ a TCXO. Semtech shields do not use a
TCXO, but can still be used in nominal conditions to experiment. Full details on
the frequency error computations are provided in AN1200.xx in Section 3.2.

(**) There is no connector for the display board on Semtech's LLCC68 devkit
shield, so the display must be disabled in `menuconfig`.

## 2.2. Dependencies

This project has been tested with ESP-IDF v5.2.1.

Among others, it uses the following espressif managed components:
* LVGL: a graphical library for OLED display, and the associated port for espressif.
* QRCode: a QR code handling library used for WiFi provisoning.

The expected versions are described in the `main/idf_component.yml` file

# 3. Usage

This project includes precompiled binaries that can be flashed onto the
supported platforms listed above. The files are located in the `bin` directory
of this project.

For flashing without the ESP-IDF installed, skip directly to the "flash with
esptool" section below.

For each supported platform/radio, the following binary files are provided:
* `xxx_bootloader.bin`: esp32 bootloader
* `xxx_partition-table.bin`: esp32 flash partition table
* `xxx_lorahub.bin`: the One-Channel Hub firmware


## 3.1. Install environment

This project is based on the Espressif ESP-IDF development. As such, the
following guide can be used to setup the environment.

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation

Please note that the following instructions are for a linux setup. It may
slightly differ with a Windows setup.

## 3.2. Build

Get the radio drivers:

```console
cd ~/this_project_directory/components/radio_drivers
```

* sx126x driver (sx1261, sx1262, sx1268):

```console
git clone -b v2.3.2 https://github.com/Lora-net/sx126x_driver.git sx126x_driver
```

* llcc68 driver:

```console
git clone -b v2.3.2 https://github.com/Lora-net/llcc68_driver.git llcc68_driver
```

* lr11xx driver (lr1121):

```console
git clone -b v2.4.1 https://github.com/Lora-net/SWDR001.git lr11xx_driver
```

Enter the lorahub directory.

```console
cd ~/this_project_directory/lorahub
```

Prepare your Linux terminal for building with ESP-IDF from the command line.
This step can be skipped on Windows as the installed 'ESP-IDF x.x CMD' tool will
prepare the environment automatically.

```console
. ~/esp/esp-idf/export.sh
```

Configure the ESP32 target to build for.

```console
idf.py set-target esp32s3
```

Customize the build if necessary (it is configured for the Heltec board by default).

```console
idf.py menuconfig
```

Build the project.

```console
idf.py all
```

## 3.3. Flash

### 3.3.1 with esp-idf

Identify the serial device associated to the One-Channel Hub to be flashed, here
we suppose it is `/dev/ttyUSB0`.

```console
idf.py -p /dev/ttyUSB0 flash
```

If a permission error is returned, check check that the current user is part of
the `dialout` group. If not, do the following, reboot the linux machine and try
again:

```console
sudo usermod -a -G dialout $USERNAME
```

On a Windows setup, let's suppose that the device is mounted as COM14, the above
command would be like:

```console
idf.py -p COM14 flash
```

Launch the monitor console to see logs (optional).

```console
idf.py -p /dev/ttyUSB0 monitor
```

### 3.3.2 with esptool

If not using the complete ESP-IDF environment, it is also possible to flash the
provided binary files using the `esptool` utility.

https://docs.espressif.com/projects/esptool/en/latest/esp32/

```console
esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x0 bootloader.bin 0x10000 lorahub.bin 0x8000 partition-table.bin
```

On a Windows setup the esptool command for flashing would be:

```console
py -m esptool --chip esp32s3 -p COM14 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x0 bootloader.bin 0x10000 lorahub.bin 0x8000 partition-table.bin
```

## 3.4. Configure One-Channel Hub at build time

It is possible to customize the default configuration before build by using the
`menuconfig`.

```console
idf.py menuconfig
```

The configuration specific to One-Channel Hub are located in the following menu:
* `LoRaWAN 1-CH HUB configuration`

It contains the following submenus:

* `Hardware Configuration`: contains board selection, radio type selection and
display enable/disable.

* `Packet Forwarder Configuration`: contains various options the channel
parameters, the LoRaWAN Network Server address and port etc...

Details are provided in the menus, but the following options are available:
* `Gateway_ID AUTO`: when checked, the MAC address is used to generate the
Gateway ID. When unchecked, a custom ID can be given.
* `Get config from flash in priority`: when checked, the channel configuration,
LNS configuration, ... is retrieved from the flash memory. If there is no
configuration stored in flash, it takes the configuration of the `menuconfig`.

In order to write a configuration in flash memory, the web interface or the REST
API have to be used. Of course, WiFi needs to be configured before.

## 3.5. Configure WiFi

To configure the WiFi Access Point for the One-Channel Hub, enable BLE
provisioning mode on the Hub.
For more details on the provisioning scheme, please refer to this page:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/provisioning/provisioning.html

In order to enter provisioning mode, do the following:
- press the `RST` button.
- within the next few seconds (5 by default, configurable in menuconfig), while
the LED is on, press the `PRG/USR/BOOT` button.

The OLED screen should display `LoRaHub - WiFi provisioning` to confirm that the
Hub has entered provisioning mode.

Please note that the current WiFi provisioning has been reset by the previous
steps, so if the Hub is being reset before entering new WiFi credentials, it
will still need to be configured. But it may not display `WiFi provisioning`
after reset.

Then, in order to configure the WiFi access point, the following app should be
installed on an Android or iOS phone.

* https://play.google.com/store/apps/details?id=com.espressif.provble
* https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141

Once installed, launch the application and follow these steps:

* Press the `Provision New Device` button.
* Press the `I don't have a QR code` button.
* Change the prefix to `LRHB_`.
* If your One-Channel Hub has entered provisioning mode, a device should be
listed with a name prefixed with `LRHB_`.
* Click on the device name, enter the `proof of possession PIN`. Default is
`abcd1234`.
* Click `Next`.
* Select the WiFi network to be joined and enter credentials.

The application will check the checkbox next to `Checking provisioning status`
if everything went well, and the One-Channel Hub OLED screen should display
`LoRaHub - Receiving`.

## 3.6. Configure One-Channel Hub from the web interface

A web interface is available at:

```
http://xx.xx.xx.xx:8000
```

The IP address can be seen on the OLED screen if a valid WiFi connection could
be set.

The web interface allows to configure the following:
* channel parameters: frequency, datarate, bandwidth
* LoRaWAN network server: address, port
* SNTP server address (to get UTC time)

There are 2 buttons at the bottom of the configuration form:
* `configure`: when pressed, the parameters set in the HTML form are written to
flash memory (NVS).
* `reboot`: when pressed, a reboot command is triggered, the One-Channel Hub
will restart and the new configuration is applied.

Note that the configuration written in flash memory is only taken into account
on the next reboot.

## 3.7. Configure One-Channel Hub from the Rest API

It is also possible to configure the One-Channel Hub from a Rest API. The
following APIs are available:

* `/api/v1/set_config`: write the given configuration to flash memory

A JSON object as follows is expected:

```json
{
    "lns_addr":"eu1.cloud.thethings.network",
    "lns_port":1700,
    "chan_freq":868.1,
    "chan_dr":7,
    "chan_bw":125,
    "sntp_addr":"pool.ntp.org"
}
```

It is possible to send only few fields, as needed.

* `/api/v1/reboot`: trigger a reboot of the One-Channel Hub

No associated data expected.
As for the web interface, a reboot is necessary in order to take the new
configuration into account.

* `/api/v1/get_config`: read the current configuration from flash memory

A JSON object with same format as `set_config` API will be returned.

* `/api/v1/get_info`: get a set of information about the One-Channel Hub.

A JSON object as follows is expected:

```json
{
    "fw_version": "1.0.0",
    "radio_type": "SX1261"
}
```

# 4. Known limitations

* FSK modulation not supported
* LR1121 support is currently experimental, it has not been fully validated.

# 5. Changelog

### v1.0.0 ###

* Implementation of a LoRa(WAN) one-channel hub on ESP32(s3) with a
sx126x/llcc68 radio.