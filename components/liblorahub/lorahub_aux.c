/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub HAL auxiliary functions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <math.h> /* pow, ceil */

#include "lorahub_log.h"
#include "lorahub_aux.h"
#include "lorahub_hal.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ----------------------------------------------------- */

static const char* TAG_AUX = LRHB_LOG_HAL_AUX;

#define FREQ_HZ_SUBGHZ_MIN 150000000
#define FREQ_HZ_SUBGHZ_MAX 960000000
#define FREQ_HZ_2_4GHZ_MIN 2400000000
#define FREQ_HZ_2_4GHZ_MAX 2500000000

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

uint32_t lgw_get_lora_bw_in_hz( uint8_t bw )
{
    uint32_t bw_in_hz = 0;

    switch( bw )
    {
    case BW_125KHZ:
        bw_in_hz = 125000UL;
        break;
    case BW_250KHZ:
        bw_in_hz = 250000UL;
        break;
    case BW_500KHZ:
        bw_in_hz = 500000UL;
        break;
    case BW_200KHZ:
        bw_in_hz = 203000UL;
        break;
    case BW_400KHZ:
        bw_in_hz = 406000UL;
        break;
    case BW_800KHZ:
        bw_in_hz = 812000UL;
        break;
    default:
        ESP_LOGE( TAG_AUX, "bandwidth %u not supported", bw );
        break;
    }

    return bw_in_hz;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

ral_lora_sf_t lgw_convert_hal_to_ral_sf( uint8_t sf )
{
    switch( sf )
    {
    case DR_LORA_SF5:
        return RAL_LORA_SF5;
    case DR_LORA_SF6:
        return RAL_LORA_SF6;
    case DR_LORA_SF7:
        return RAL_LORA_SF7;
    case DR_LORA_SF8:
        return RAL_LORA_SF8;
    case DR_LORA_SF9:
        return RAL_LORA_SF9;
    case DR_LORA_SF10:
        return RAL_LORA_SF10;
    case DR_LORA_SF11:
        return RAL_LORA_SF11;
    case DR_LORA_SF12:
        return RAL_LORA_SF12;
    default:
        ESP_LOGE( TAG_AUX, "spreading factor %u not supported", sf );
        return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

ral_lora_bw_t lgw_convert_hal_to_ral_bw( uint8_t bw )
{
    switch( bw )
    {
    /* Sub-Ghz bandwidths */
    case BW_125KHZ:
        return RAL_LORA_BW_125_KHZ;
    case BW_250KHZ:
        return RAL_LORA_BW_250_KHZ;
    case BW_500KHZ:
        return RAL_LORA_BW_500_KHZ;
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    /* 2.4GHz bandwidths */
    case BW_200KHZ:
        return RAL_LORA_BW_200_KHZ;
    case BW_400KHZ:
        return RAL_LORA_BW_400_KHZ;
    case BW_800KHZ:
        return RAL_LORA_BW_800_KHZ;
#endif
    default:
        ESP_LOGE( TAG_AUX, "bandwidth %u not supported", bw );
        return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

ral_lora_cr_t lgw_convert_hal_to_ral_cr( uint8_t cr )
{
    switch( cr )
    {
    case CR_LORA_4_5:
        return RAL_LORA_CR_4_5;
    case CR_LORA_4_6:
        return RAL_LORA_CR_4_6;
    case CR_LORA_4_7:
        return RAL_LORA_CR_4_7;
    case CR_LORA_4_8:
        return RAL_LORA_CR_4_8;
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    case CR_LORA_LI_4_5:
        return RAL_LORA_CR_LI_4_5;
    case CR_LORA_LI_4_6:
        return RAL_LORA_CR_LI_4_6;
    case CR_LORA_LI_4_8:
        return RAL_LORA_CR_LI_4_8;
#endif
    default:
        ESP_LOGE( TAG_AUX, "coderate %u not supported", cr );
        return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int lgw_check_lora_mod_params( uint32_t freq_hz, uint8_t bw, uint8_t cr )
{
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    if( ( freq_hz < FREQ_HZ_SUBGHZ_MIN ) || ( ( freq_hz > FREQ_HZ_SUBGHZ_MAX ) && ( freq_hz < FREQ_HZ_2_4GHZ_MIN ) ) ||
        ( freq_hz > FREQ_HZ_2_4GHZ_MAX ) )
    {
        ESP_LOGE( TAG_AUX, "frequency not supported" );
        return LGW_HAL_ERROR;
    }

    if( freq_hz <= FREQ_HZ_SUBGHZ_MAX )
    {
        /* Check sub-ghz bandwidths */
        if( ( bw != BW_125KHZ ) && ( bw != BW_250KHZ ) && ( bw != BW_500KHZ ) )
        {
            ESP_LOGE( TAG_AUX, "bandwidth not supported for sub-ghz" );
            return LGW_HAL_ERROR;
        }
    }
    else
    {
        /* Check 2.4ghz bandwidths */
        if( ( bw != BW_200KHZ ) && ( bw != BW_400KHZ ) && ( bw != BW_800KHZ ) )
        {
            ESP_LOGE( TAG_AUX, "bandwidth not supported for 2.4ghz" );
            return LGW_HAL_ERROR;
        }
    }
#else
    if( ( freq_hz < FREQ_HZ_SUBGHZ_MIN ) || ( freq_hz > FREQ_HZ_SUBGHZ_MAX ) )
    {
        ESP_LOGE( TAG_AUX, "frequency not supported" );
        return LGW_HAL_ERROR;
    }

    if( ( bw != BW_125KHZ ) && ( bw != BW_250KHZ ) && ( bw != BW_500KHZ ) )
    {
        ESP_LOGE( TAG_AUX, "bandwidth not supported" );
        return LGW_HAL_ERROR;
    }

    if( ( cr != CR_LORA_4_5 ) && ( cr != CR_LORA_4_6 ) && ( cr != CR_LORA_4_7 ) && ( cr != CR_LORA_4_8 ) )
    {
        ESP_LOGE( TAG_AUX, "coderate not supported" );
        return LGW_HAL_ERROR;
    }
#endif

    return LGW_HAL_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint8_t lgw_get_lora_sync_word( uint32_t freq_hz, uint8_t sf )
{
    uint8_t lora_sync_word = LORA_SYNC_WORD_PRIVATE;

    if( LPWAN_NETWORK_TYPE == LPWAN_NETWORK_TYPE_PUBLIC )
    {
        if( freq_hz >= 2400000000 )
        {
            /* 2.4ghz */
            lora_sync_word = LORA_SYNC_WORD_PUBLIC_WW2G4;
        }
        else
        {
            /* sub-ghz */
            if( sf >= DR_LORA_SF7 )
            {
                lora_sync_word = LORA_SYNC_WORD_PUBLIC_SUBGHZ;
            }
        }
    }
    ESP_LOGD( TAG_AUX, "LoRa sync word: 0x%02X", lora_sync_word );

    return lora_sync_word;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int lgw_check_lora_dualsf_conf( uint8_t bw, uint8_t sf1, uint8_t sf2 )
{
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    if( ( sf1 == DR_UNDEFINED ) || ( sf2 == DR_UNDEFINED ) )
    {
        /* Single-SF configuration, no constraints */
        return LGW_HAL_SUCCESS;
    }

    if( bw != BW_800KHZ )
    {
        /* If SF12 is used, only single-SF is supported */
        if( ( sf1 == DR_LORA_SF12 ) || ( sf2 == DR_LORA_SF12 ) )
        {
            ESP_LOGE( TAG_AUX, "no dual-SF support if SF12 is used" );
            return LGW_HAL_ERROR;
        }
    }
    else
    {
        /* If SF12 is used, only single-SF is supported */
        if( ( sf1 >= DR_LORA_SF10 ) || ( sf2 >= DR_LORA_SF10 ) )
        {
            ESP_LOGE( TAG_AUX, "no dual-SF support if SF10,SF11,SF12 at BW800 is used" );
            return LGW_HAL_ERROR;
        }
    }

    /* Cannot use same spreading factors for dual-SF configuration */
    if( sf1 == sf2 )
    {
        ESP_LOGE( TAG_AUX, "dual-SF requires different spreading factors to be configured" );
        return LGW_HAL_ERROR;
    }

    /* The maximum spreading factor distance supported is 4 */
    if( abs( sf1 - sf2 ) > 4 )
    {
        ESP_LOGE( TAG_AUX, "dual-SF configuration is not supported (>4)" );
        return LGW_HAL_ERROR;
    }

    /* dual-SF configuration should have a unique PPM offset configuration */
    ral_lora_sf_t ral_sf1 = lgw_convert_hal_to_ral_sf( sf1 );
    ral_lora_sf_t ral_sf2 = lgw_convert_hal_to_ral_sf( sf2 );
    ral_lora_bw_t ral_bw  = lgw_convert_hal_to_ral_bw( bw );
    if( ral_compute_lora_ldro( ral_sf1, ral_bw ) != ral_compute_lora_ldro( ral_sf2, ral_bw ) )
    {
        ESP_LOGE( TAG_AUX, "incompatible ppm offset for dual-SF configuration" );
        return LGW_HAL_ERROR;
    }
#endif

    return LGW_HAL_SUCCESS;
}

/* --- EOF ------------------------------------------------------------------ */
