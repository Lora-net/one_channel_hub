	  ______                              _
	 / _____)             _              | |
	( (____  _____ ____ _| |_ _____  ____| |__
	 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
	 _____) ) ____| | | || |_| ____( (___| | | |
	(______/|_____)_|_|_| \__)_____)\____)_| |_|
	  (C)2024 Semtech

Utility: Downlink server / Packet Logger
========================================

## 1. Introduction

This utility allows to send regular downlink requests to the packet forwarder
running on the hub.

The downlinks are sent in 'immediate' mode, meaning that the hub will send the
incoming packets over the air as soon as it receives it.

The net_downlink utility constructs a JSON 'txpk' object based on given command
line arguments, and send it on a UDP socket on the given port. Then the packet
forwarder receives it on its downlink socket, parses the JSON object to build
the packet buffer to be sent to the concentrator board.

This utility acts as a basic network server so it can be compiled and run on any
remote machine (PC, Raspberry Pi, ...).

This utility can also be used as a UDP packet logger, logging all uplinks in a
CSV file.

## 2. Dependencies

A packet forwarder must be running to receive downlink packets and send it to
the concentrator board.

## 3. Usage

### 3.1. Packet Forwarder configuration

The packet forwarder running on the Hub must connect to the IP address of the
machine on which the net_downlink utility is running, and the UDP port should
match.

### 3.2. Launching net_downlink

The net_downlink utility can be started with various command line arguments.

In order to get the available options, and some examples, run:

`./net_downlink -h`

To stop the application, press Ctrl+C.
