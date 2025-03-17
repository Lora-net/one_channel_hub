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

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <string.h>  // strncpy

#include <esp_log.h>
#include <nvs_flash.h>

#include "config_nvs.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ----------------------------------------------------- */

static const char* TAG_NVS = "NVS";

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

static lgw_nvs_cfg_t lgw_nvs_config = { .lns_address        = CONFIG_NETWORK_SERVER_ADDRESS,
                                        .lns_port           = ( uint16_t ) CONFIG_NETWORK_SERVER_PORT,
                                        .chan_freq_hz       = ( uint32_t ) CONFIG_CHANNEL_FREQ_HZ,
                                        .chan_datarate_1    = ( uint8_t ) CONFIG_CHANNEL_LORA_SPREADING_FACTOR_1,
                                        .chan_datarate_2    = ( uint8_t ) CONFIG_CHANNEL_LORA_SPREADING_FACTOR_2,
                                        .chan_bandwidth_khz = ( uint16_t ) CONFIG_CHANNEL_LORA_BANDWIDTH,
                                        .sntp_address       = CONFIG_SNTP_SERVER_ADDRESS };

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

esp_err_t lgw_nvs_load_config( void )
{
    nvs_handle_t my_handle;
    esp_err_t    err = ESP_OK;

    /* Get configuration from NVS */
    ESP_LOGI( TAG_NVS, "Opening Non-Volatile Storage (NVS) handle for reading..." );
    err = nvs_open( "storage", NVS_READONLY, &my_handle );
    if( err != ESP_OK )
    {
        ESP_LOGW( TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name( err ) );
        return ESP_FAIL;
    }
    else
    {
        size_t size = sizeof( lgw_nvs_config.lns_address );
        err         = nvs_get_str( my_handle, CFG_NVS_KEY_LNS_ADDRESS, lgw_nvs_config.lns_address, &size );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %s\n", CFG_NVS_KEY_LNS_ADDRESS, lgw_nvs_config.lns_address );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_LNS_ADDRESS, esp_err_to_name( err ) );
        }

        err = nvs_get_u16( my_handle, CFG_NVS_KEY_LNS_PORT, &( lgw_nvs_config.lns_port ) );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %" PRIu16 "\n", CFG_NVS_KEY_LNS_PORT, lgw_nvs_config.lns_port );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_LNS_PORT, esp_err_to_name( err ) );
        }

        err = nvs_get_u32( my_handle, CFG_NVS_KEY_CHAN_FREQ, &( lgw_nvs_config.chan_freq_hz ) );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %" PRIu32 "hz\n", CFG_NVS_KEY_CHAN_FREQ, lgw_nvs_config.chan_freq_hz );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_CHAN_FREQ, esp_err_to_name( err ) );
        }

        err = nvs_get_u8( my_handle, CFG_NVS_KEY_CHAN_DR_1, &( lgw_nvs_config.chan_datarate_1 ) );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %" PRIu8 "\n", CFG_NVS_KEY_CHAN_DR_1, lgw_nvs_config.chan_datarate_1 );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_CHAN_DR_1, esp_err_to_name( err ) );
        }

        err = nvs_get_u8( my_handle, CFG_NVS_KEY_CHAN_DR_2, &( lgw_nvs_config.chan_datarate_2 ) );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %" PRIu8 "\n", CFG_NVS_KEY_CHAN_DR_2, lgw_nvs_config.chan_datarate_2 );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_CHAN_DR_2, esp_err_to_name( err ) );
        }

        err = nvs_get_u16( my_handle, CFG_NVS_KEY_CHAN_BW, &( lgw_nvs_config.chan_bandwidth_khz ) );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %" PRIu16 "khz\n", CFG_NVS_KEY_CHAN_BW, lgw_nvs_config.chan_bandwidth_khz );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_CHAN_BW, esp_err_to_name( err ) );
        }

        size = sizeof( lgw_nvs_config.sntp_address );
        err  = nvs_get_str( my_handle, CFG_NVS_KEY_SNTP_ADDRESS, lgw_nvs_config.sntp_address, &size );
        if( err == ESP_OK )
        {
            printf( "NVS -> %s = %s\n", CFG_NVS_KEY_SNTP_ADDRESS, lgw_nvs_config.sntp_address );
        }
        else
        {
            ESP_LOGW( TAG_NVS, "Failed to get %s from NVS - %s", CFG_NVS_KEY_SNTP_ADDRESS, esp_err_to_name( err ) );
        }
    }
    nvs_close( my_handle );
    ESP_LOGI( TAG_NVS, "Closed NVS handle for reading" );

    return ESP_OK;
}

esp_err_t lgw_nvs_save_config( void )
{
    esp_err_t    err = ESP_OK;
    nvs_handle_t my_handle;

    printf( "Opening Non-Volatile Storage (NVS) handle for writing... " );
    err = nvs_open( "storage", NVS_READWRITE, &my_handle );
    if( err != ESP_OK )
    {
        printf( "Error (%s) opening NVS handle!\n", esp_err_to_name( err ) );
        return ESP_FAIL;
    }
    else
    {
        printf( "Done\n" );
    }

    printf( "NVS <- %s = %s ... ", CFG_NVS_KEY_LNS_ADDRESS, lgw_nvs_config.lns_address );
    err = nvs_set_str( my_handle, CFG_NVS_KEY_LNS_ADDRESS, lgw_nvs_config.lns_address );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %" PRIu16 " ... ", CFG_NVS_KEY_LNS_PORT, lgw_nvs_config.lns_port );
    err = nvs_set_u16( my_handle, CFG_NVS_KEY_LNS_PORT, lgw_nvs_config.lns_port );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %" PRIu32 " ... ", CFG_NVS_KEY_CHAN_FREQ, lgw_nvs_config.chan_freq_hz );
    err = nvs_set_u32( my_handle, CFG_NVS_KEY_CHAN_FREQ, lgw_nvs_config.chan_freq_hz );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %" PRIu8 " ... ", CFG_NVS_KEY_CHAN_DR_1, lgw_nvs_config.chan_datarate_1 );
    err = nvs_set_u8( my_handle, CFG_NVS_KEY_CHAN_DR_1, lgw_nvs_config.chan_datarate_1 );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %" PRIu8 " ... ", CFG_NVS_KEY_CHAN_DR_2, lgw_nvs_config.chan_datarate_2 );
    err = nvs_set_u8( my_handle, CFG_NVS_KEY_CHAN_DR_2, lgw_nvs_config.chan_datarate_2 );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %" PRIu16 " ... ", CFG_NVS_KEY_CHAN_BW, lgw_nvs_config.chan_bandwidth_khz );
    err = nvs_set_u16( my_handle, CFG_NVS_KEY_CHAN_BW, lgw_nvs_config.chan_bandwidth_khz );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "NVS <- %s = %s ... ", CFG_NVS_KEY_SNTP_ADDRESS, lgw_nvs_config.sntp_address );
    err = nvs_set_str( my_handle, CFG_NVS_KEY_SNTP_ADDRESS, lgw_nvs_config.sntp_address );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    printf( "Committing updates in NVS ... " );
    err = nvs_commit( my_handle );
    if( err == ESP_OK )
    {
        printf( "Done\n" );
    }
    else
    {
        printf( "Failed\n" );
        nvs_close( my_handle );
        printf( "Closed NVS handle for writing.\n" );
        return ESP_FAIL;
    }

    nvs_close( my_handle );
    ESP_LOGI( TAG_NVS, "Closed NVS handle for writing" );

    return ESP_OK;
}

esp_err_t lgw_nvs_get_config( const lgw_nvs_cfg_t** config )
{
    if( config == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    *config = &lgw_nvs_config;

    return ESP_OK;
}

esp_err_t lgw_nvs_set_config( const lgw_nvs_cfg_t* config )
{
    if( config == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy( lgw_nvs_config.lns_address, config->lns_address, LNS_ADDRESS_STR_MAX_SIZE - 1 );
    lgw_nvs_config.lns_address[LNS_ADDRESS_STR_MAX_SIZE - 1] = '\0';

    lgw_nvs_config.lns_port           = config->lns_port;
    lgw_nvs_config.chan_freq_hz       = config->chan_freq_hz;
    lgw_nvs_config.chan_datarate_1    = config->chan_datarate_1;
    lgw_nvs_config.chan_datarate_2    = config->chan_datarate_2;
    lgw_nvs_config.chan_bandwidth_khz = config->chan_bandwidth_khz;

    strncpy( lgw_nvs_config.sntp_address, config->sntp_address, SNTP_ADDRESS_STR_MAX_SIZE - 1 );
    lgw_nvs_config.sntp_address[SNTP_ADDRESS_STR_MAX_SIZE - 1] = '\0';

    return ESP_OK;
}
/* --- EOF ------------------------------------------------------------------ */