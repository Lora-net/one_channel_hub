# How to Configure LoRa Basics Modem (LBM) for LoRa 1-Channel Hub

On the end-device side, when using the LoRa Basics Modem LoRaWAN stack, it is possible to optimize the join time and secure the alignment of the channel and datarate between an end-device and a LoRa 1-channel Hub.

In order to achieve this, LBM provides few advanced APIs which can be called before `smtc_modem_join_network()`:

* `smtc_modem_adr_set_join_distribution()`: set datarate to be used for join requests.
* `smtc_modem_adr_set_profile()`: set datarate to be used for all uplinks after join.

Both functions take as parameter a distribution list of the datarates to be used. Each entry of the list represents
1/16th of the distribution.
For example, in order to use DR5 only datarate, this list would have to be defined as below:

```c
static const uint8_t adr_custom_list[16] = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
}
```

Optionally, it is also possible to force the number of retransmissions for each uplink to optimize the range and/or the hub capacity.

* `smtc_modem_set_nb_trans()`

With this configuration, LBM will still try to join on all mandatory join channels (according to LoRaWAN regional parameters), but it ensures that it uses to proper datarate when reaching the hub channel frequency.

After this, it is expected that the LNS (Chirpstack) send a downlink back to the end-device to set the channel mask to the selected channel.

**Notes:**

* The `smtc_modem_adr_set_join_distribution()` function has no effect for FCC regulated regions (US915...). In this case it is currently not possible to speed up the join procedure from the LBM (v4.5.0) API.