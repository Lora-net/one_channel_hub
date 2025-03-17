/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub configuration storage helpers

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _CONFIG_NVS_H
#define _CONFIG_NVS_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define CFG_NVS_KEY_STR_MAX_SIZE ( 16 )

/* Size of the following string must be < CFG_NVS_KEY_STR_MAX_SIZE */
#define CFG_NVS_KEY_LNS_ADDRESS "lns_addr"
#define CFG_NVS_KEY_LNS_PORT "lns_port"
#define CFG_NVS_KEY_CHAN_FREQ "chan_freq"
#define CFG_NVS_KEY_CHAN_DR_1 "chan_dr"
#define CFG_NVS_KEY_CHAN_DR_2 "chan_dr_2"
#define CFG_NVS_KEY_CHAN_BW "chan_bw"
#define CFG_NVS_KEY_SNTP_ADDRESS "sntp_addr"

#define LNS_ADDRESS_STR_MAX_SIZE ( 64 )
#define SNTP_ADDRESS_STR_MAX_SIZE ( 64 )

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

typedef struct
{
    char     lns_address[LNS_ADDRESS_STR_MAX_SIZE];
    uint16_t lns_port;
    uint32_t chan_freq_hz;
    uint8_t  chan_datarate_1;
    uint8_t  chan_datarate_2;
    uint16_t chan_bandwidth_khz;
    char     sntp_address[SNTP_ADDRESS_STR_MAX_SIZE];
} lgw_nvs_cfg_t;

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

esp_err_t lgw_nvs_load_config( void );

esp_err_t lgw_nvs_save_config( void );

esp_err_t lgw_nvs_get_config( const lgw_nvs_cfg_t** config );

esp_err_t lgw_nvs_set_config( const lgw_nvs_cfg_t* config );

#endif  // _CONFIG_NVS_H

/* --- EOF ------------------------------------------------------------------ */