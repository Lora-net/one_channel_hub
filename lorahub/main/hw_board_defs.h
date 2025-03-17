/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub Hardware specific definitions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _HW_BOARD_DEFS_H
#define _HW_BOARD_DEFS_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* Board specific constants */
#if defined( CONFIG_HELTEC_WIFI_LORA_32_V3 )
#define USER_LED_GPIO 35   /* User LED */
#define USER_BUTTON_GPIO 0 /* User button (PRG) from Heltec board */
#define OLED_DISPLAY_PIN_NUM_SDA 17
#define OLED_DISPLAY_PIN_NUM_SCL 18
#define OLED_DISPLAY_PIN_NUM_RST 21
#elif defined( CONFIG_SEEED_XIAO_ESP32S3_DEVKIT )
#define USER_LED_GPIO 48            /* User LED from Xiao Wio-sx1262 board */
#define USER_BUTTON_GPIO 21         /* User button from Xiao Wio-sx1262 board */
#define OLED_DISPLAY_PIN_NUM_SDA 5  /* Only if connected on a Seeed Studio Expansion Board Base for XIAO */
#define OLED_DISPLAY_PIN_NUM_SCL 6  /* Only if connected on a Seeed Studio Expansion Board Base for XIAO */
#define OLED_DISPLAY_PIN_NUM_RST -1 /* Not used */
#elif defined( CONFIG_LILYGO_T3S3_LORA32 )
#define USER_LED_GPIO 37   /* User LED */
#define USER_BUTTON_GPIO 0 /* User button (Boot) from Lilygo board */
#define OLED_DISPLAY_PIN_NUM_SDA 18
#define OLED_DISPLAY_PIN_NUM_SCL 17
#define OLED_DISPLAY_PIN_NUM_RST -1
#elif defined( CONFIG_SEMTECH_DEVKIT )
#define USER_LED_GPIO 38   /* User LED from Display board */
#define USER_BUTTON_GPIO 0 /* User button from Display board */
#define OLED_DISPLAY_PIN_NUM_SDA 17
#define OLED_DISPLAY_PIN_NUM_SCL 18
#define OLED_DISPLAY_PIN_NUM_RST -1
#elif defined( CONFIG_EBYTES_ESP32_LR1121 )
#define USER_LED_GPIO 35   /* User LED */
#define USER_BUTTON_GPIO 0 /* User button (PRG) from EBytes board */
#define OLED_DISPLAY_PIN_NUM_SDA 18
#define OLED_DISPLAY_PIN_NUM_SCL 17
#define OLED_DISPLAY_PIN_NUM_RST 21
#else
#error "No supported board selected"
#endif

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

#endif  // _HW_BOARD_DEFS_H

/* --- EOF ------------------------------------------------------------------ */