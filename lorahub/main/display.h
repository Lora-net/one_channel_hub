/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub Packet Forwarder Display definitions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _PKTFWD_DISPLAY_H
#define _PKTFWD_DISPLAY_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include "lorahub_hal.h"
#include "main_defs.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

typedef enum
{
    DISPLAY_STATUS_UNKNOWN,
    DISPLAY_STATUS_INITIALIZING,
    DISPLAY_STATUS_WIFI_PROVISIONING,
    DISPLAY_STATUS_RECEIVING,
} display_status_t;

typedef struct
{
    uint32_t freq_hz;
    uint8_t  datarate[LGW_MULTI_SF_NB]; /* spreading factor(s) */
    uint16_t bw_khz;
} display_channel_conf_t;

typedef struct
{
    uint32_t nb_rx;
    uint32_t nb_tx;
} display_stats_t;

typedef struct
{
    uint64_t gateway_id;
    // TODO: add LNS address, port
    // TODO: add IP address
} display_connection_info_t;

typedef struct
{
    uint32_t devaddr;
    float    rssi;
    float    snr;
    uint8_t  sf;
} display_last_rx_packet_t;

typedef struct
{
    lorahub_error_t err;
    int             line;
} display_error_t;

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

void display_init( void );

void display_refresh( void );

void display_update_status( display_status_t status );

void display_update_connection_info( const display_connection_info_t* info );

void display_update_channel_config( const display_channel_conf_t* chan_cfg );

void display_update_statistics( const display_stats_t* stats );

void display_update_last_rx_packet( const display_last_rx_packet_t* last_pkt );

void display_update_error( const display_error_t* error );

#endif  // _PKTFWD_DISPLAY_H

/* --- EOF ------------------------------------------------------------------ */