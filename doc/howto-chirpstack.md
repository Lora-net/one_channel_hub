# How to configure Chirpstack for operating with a LoRa 1-Channel Hub

To properly operate a LoRaWAN network with a LoRa 1-channel Hub, it is necessary to configure the Network Server to take into account the constraints of having 1 channel and 1 datarate only.

Chirpstack Network Server configuration is flexible enough to achieve this.

The goal of this document is not to describe how to install Chirpstack, as this is covered in detail [here](https://www.chirpstack.io/docs/getting-started/debian-ubuntu.html).

Once Chirpstack is installed, do the following modifications. The example is given for operating with the EU868 region, but the same principle can be applied for any other region.

## 1. Chirpstack Gateway Bridge

As described in the instructions for installing Chirpstack, it is necessary to update `/etc/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml` to add the region prefix in the `[integration.mqtt]` section.

Example:

```toml
[integration.mqtt]

event_topic_template="eu868/gateway/{{ .GatewayID }}/event/{{ .EventType }}"

command_topic_template="eu868/gateway/{{ .GatewayID }}/command/#"
```

## 2. Chirpstack

There are 2 steps to be done here:

- Define a new region description file (e.g., `region_eu868_1ch.toml`)
- Indicate to Chirpstack the new region defined to be selected (`/etc/chirpstack/chirpstack.toml`)

### 2.1. EU868 1-Channel Region Definition

After Chirpstack installation, the `/etc/chirpstack` directory contains several region definition files. To define a new region for 1-channel operation in the EU868 region, you can duplicate the `/etc/chirpstack.region_eu868.toml` file and modify it as below.

The following will be done:

- New region ID: `eu868_1ch`
- ADR is disabled
- Channel 0 is selected as the only channel (868.1 MHz)

```toml
# ID is a user-defined identifier for this region.
id="eu868_1ch"

# Description is a short description for this region.
description="EU868_1CH"

# Common-name refers to the common name of this region as defined by
# the LoRa Alliance.
common_name="EU868"

# ADR is disabled.
adr_disabled=true

# Add the following after min_dr/max_dr configuration
enabled_uplink_channels=[0]
```

**Notes:**

- The whole `#Gateway channel configuration` section can be removed.
- The section `[regions.network.rejoin_request]` can be removed.
- The section `[regions.network.class_b]` can be removed.
- All `[[regions.network.extra_channels]]` can be removed.
- For the US915 region, select a 500KHz channel: `enabled_uplink_channels=[64]`

### 2.2. Select the 1-Channel Region

Modify the `/etc/chirpstack/chirpstack.toml` to enable the newly defined region.

```toml
enabled_regions=[
    "eu868_1ch"
]
```

After all this is done, restart both Chirpstack services. After a configured Hub has received an uplink, the Region ID displayed should match the newly created one.

