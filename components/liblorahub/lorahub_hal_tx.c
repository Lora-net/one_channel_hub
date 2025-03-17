/*______                              _
/ _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
\____ \| ___ |    (_   _) ___ |/ ___)  _ \
_____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
(C)2024 Semtech

Description:
    LoRaHub Hardware Abstraction Layer - TX

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <string.h>

#include "lorahub_log.h"
#include "lorahub_aux.h"
#include "lorahub_hal.h"
#include "lorahub_hal_tx.h"

#include "ral.h"
#include "radio_context.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ----------------------------------------------------- */

static const char* TAG_HAL_TX = LRHB_LOG_HAL_TX;

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES --------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void set_led_rx( const ral_t* ral, bool on )
{
    const radio_context_t* radio_context = ( const radio_context_t* ) ( ral->context );

    if( radio_context->gpio_led_rx != 0xFF )
    {
        gpio_set_level( radio_context->gpio_led_rx, ( on == true ) ? 1 : 0 );
    }
}

static void set_led_tx( const ral_t* ral, bool on )
{
    const radio_context_t* radio_context = ( const radio_context_t* ) ( ral->context );

    if( radio_context->gpio_led_tx != 0xFF )
    {
        gpio_set_level( radio_context->gpio_led_tx, ( on == true ) ? 1 : 0 );
    }
}

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int lgw_radio_configure_tx( const ral_t* ral, struct lgw_pkt_tx_s* pkt_data )
{
    set_led_rx( ral, false );
    set_led_tx( ral, true );

    /* Check parameters */
    if( lgw_check_lora_mod_params( pkt_data->freq_hz, pkt_data->bandwidth, pkt_data->coderate ) != LGW_HAL_SUCCESS )
    {
        ESP_LOGE( TAG_HAL_TX, "Invalid parameters to configure for TX" );
        return LGW_HAL_ERROR;
    }

    /* Configure for TX */
    ASSERT_RAL_RC( ral_set_standby( ral, RAL_STANDBY_CFG_RC ) );

    ASSERT_RAL_RC( ral_set_pkt_type( ral, RAL_PKT_TYPE_LORA ) );
    ASSERT_RAL_RC( ral_set_rf_freq( ral, pkt_data->freq_hz ) );
    ASSERT_RAL_RC( ral_set_tx_cfg( ral, pkt_data->rf_power, pkt_data->freq_hz ) );

    ral_lora_sf_t         ral_sf          = lgw_convert_hal_to_ral_sf( pkt_data->datarate );
    ral_lora_bw_t         ral_bw          = lgw_convert_hal_to_ral_bw( pkt_data->bandwidth );
    ral_lora_cr_t         ral_cr          = lgw_convert_hal_to_ral_cr( pkt_data->coderate );
    ral_lora_mod_params_t lora_mod_params = {
        .sf = ral_sf, .bw = ral_bw, .cr = ral_cr, .ldro = ral_compute_lora_ldro( ral_sf, ral_bw )
    };
    ASSERT_RAL_RC( ral_set_lora_mod_params( ral, &lora_mod_params ) );

    const ral_lora_pkt_params_t lora_pkt_params = {
        .preamble_len_in_symb = pkt_data->preamble,
        .header_type          = ( pkt_data->no_header ) ? RAL_LORA_PKT_IMPLICIT : RAL_LORA_PKT_EXPLICIT,
        .pld_len_in_bytes     = pkt_data->size,
        .crc_is_on            = !pkt_data->no_crc,
        .invert_iq_is_on      = pkt_data->invert_pol,
    };
    ASSERT_RAL_RC( ral_set_lora_pkt_params( ral, &lora_pkt_params ) );

    ASSERT_RAL_RC( ral_set_lora_sync_word( ral, lgw_get_lora_sync_word( pkt_data->freq_hz, pkt_data->datarate ) ) );

    const ral_irq_t tx_irq_mask = RAL_IRQ_TX_DONE | RAL_IRQ_RX_TIMEOUT;
    ASSERT_RAL_RC( ral_set_dio_irq_params( ral, tx_irq_mask ) );
    ASSERT_RAL_RC( ral_clear_irq_status( ral, RAL_IRQ_ALL ) );

    ASSERT_RAL_RC( ral_set_pkt_payload( ral, pkt_data->payload, pkt_data->size ) );

    return LGW_HAL_SUCCESS;
}

/* --- EOF ------------------------------------------------------------------ */