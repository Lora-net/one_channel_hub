# How to configure The Things Network for operating with a LoRa 1-Channel Hub

In order to operate a local LoRaWAN network using The Things Network (thethingsnetwork.org) with one (or more) 1-Channel Hub(s),it is necessary to register the hubs and end-devices using the proper frequency plan to indicate to TTN LNS that a hub is being used.

## Frequency Plans

The following frequency plans are available, using 1 predefined join channel as communication channel:

* EU868: `Europe 868.1 MHz`
* US915: `United States 903.0 MHz`
* CN490: `China 470.3 MHz`

## Registering a one channel hub

The one channel hub has to be registered as a regular gateway, the only difference is the frequency plan selected, which has to be one of the previous list.

## Registering an end-device

Similarly, the end-device registration has to specify the same frequency plan as the one used for the hub.

Also, it is recommended to disable the Adaptative Datarate (ADR) for the registered end-device in order to ensure that the end-device keeps using the datarate configured on the hub.

This can be done on with the TTN user interface with the following steps:
* end-device -> `Settings`
* `Network layer` -> press the expand button
* `Advanced MAC settings`
* `Adaptative data rate (ADR)` -> select `Disabled` or `Static Mode` with `ADR data rate index` configured to the one used by the hub, and `ADR number of transmissions` set as wanted.