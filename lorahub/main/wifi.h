/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub Packet Forwarder WiFi helpers definitions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _PKTFWD_WIFI_H
#define _PKTFWD_WIFI_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <stdint.h>  /* C99 types */
#include <stdbool.h> /* bool type */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

typedef enum
{
    WIFI_STATUS_UNKNOWN,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTED
} wifi_status_t;

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

int wifi_sta_init( bool reset_provisioning );

int wifi_get_mac_address( uint8_t mac_address[6] );

wifi_status_t wifi_get_status( void );

#endif  // _PKTFWD_WIFI_H

/* --- EOF ------------------------------------------------------------------ */