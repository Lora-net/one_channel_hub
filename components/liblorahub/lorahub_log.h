/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub HAL logging options

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _LORAHUB_LOG_H
#define _LORAHUB_LOG_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include "esp_log.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* Change below log levels from less verbose to more verbose (ESP_LOG_NONE, ESP_LOG_ERROR, ESP_LOG_WARN, ESP_LOG_INFO,
 * ESP_LOG_DEBUG, ESP_LOG_VERBOSE) */
/* Change CONFIG_LOG_MAXIMUM_LEVEL in menuconfig accordingly */
#define LRHB_LOG_DEVEL_HAL ESP_LOG_INFO
#define LRHB_LOG_DEVEL_HAL_RX ESP_LOG_INFO
#define LRHB_LOG_DEVEL_HAL_TX ESP_LOG_INFO
#define LRHB_LOG_DEVEL_HAL_AUX ESP_LOG_INFO

#define LRHB_LOG_HAL "LRHB_HAL"
#define LRHB_LOG_HAL_RX "LRHB_HAL_RX"
#define LRHB_LOG_HAL_TX "LRHB_HAL_TX"
#define LRHB_LOG_HAL_AUX "LRHB_HAL_AUX"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

#endif  // _LORAHUB_LOG_H

/* --- EOF ------------------------------------------------------------------ */
