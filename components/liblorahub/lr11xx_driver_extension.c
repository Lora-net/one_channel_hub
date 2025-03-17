/*!
 * @file      lr11xx_driver_extension.c
 *
 * @brief     Driver extension definition for LR11XX
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "lr11xx_regmem.h"
#include "lr11xx_driver_extension.h"

#include "esp_log.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define LR11XX_REG_LORA_MODEM_RX_CFG 0xF20428
#define LR11XX_REG_LORA_MODEM_RX_DFE_AGC 0xF20448
#define LR11XX_REG_LORA_MODEM_DAGC_CFG 0xF2044C
#define LR11XX_REG_LORA_MODEM_RX_STATUS 0xF2047C
#define LR11XX_REG_LORA_MODEM_SIDEDET_0 0xF20500

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

lr11xx_status_t lr11xx_get_lora_side_detector_cfg( const void* context, uint8_t idx, lr11xx_side_detector_cfg_t* cfg )
{
    if( idx > 2 )
    {
        return LR11XX_STATUS_ERROR;
    }

    uint32_t        reg_val = 0;
    lr11xx_status_t status =
        lr11xx_regmem_read_regmem32( context, LR11XX_REG_LORA_MODEM_SIDEDET_0 + ( 4 * idx ), &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }

    cfg->half_bin         = ( reg_val >> 31 ) & 0x1;   // Bit 31
    cfg->msp_peak_nb      = ( reg_val >> 28 ) & 0x7;   // Bits 30:28
    cfg->msp_pnr          = ( reg_val >> 20 ) & 0x7F;  // Bits 26:20
    cfg->peak2_pos        = ( reg_val >> 15 ) & 0x1F;  // Bits 19:15
    cfg->peak1_pos        = ( reg_val >> 10 ) & 0x1F;  // Bits 14:10
    cfg->ppm_offset_hc    = ( reg_val >> 8 ) & 0x3;    // Bits 9:8
    cfg->header_diff_mode = ( reg_val >> 7 ) & 0x1;    // Bit 7
    cfg->fine_synch       = ( reg_val >> 6 ) & 0x1;    // Bit 6
    cfg->chirp_invert     = ( reg_val >> 5 ) & 0x1;    // Bit 5
    cfg->enable           = ( reg_val >> 4 ) & 0x1;    // Bit 4
    cfg->sf_log           = reg_val & 0xF;             // Bits 3:0

    return LR11XX_STATUS_OK;
}

lr11xx_status_t lr11xx_set_lora_side_detector_cfg( const void* context, uint8_t idx, lr11xx_side_detector_cfg_t* cfg )
{
    if( idx > 2 )
    {
        return LR11XX_STATUS_ERROR;
    }

    uint32_t reg_val = 0;
    reg_val |= ( cfg->half_bin & 0x1 ) << 31;         // Bit 31
    reg_val |= ( cfg->msp_peak_nb & 0x7 ) << 28;      // Bits 30:28
    reg_val |= ( cfg->msp_pnr & 0x7F ) << 20;         // Bits 26:20
    reg_val |= ( cfg->peak2_pos & 0x1F ) << 15;       // Bits 19:15
    reg_val |= ( cfg->peak1_pos & 0x1F ) << 10;       // Bits 14:10
    reg_val |= ( cfg->ppm_offset_hc & 0x3 ) << 8;     // Bits 9:8
    reg_val |= ( cfg->header_diff_mode & 0x1 ) << 7;  // Bit 7
    reg_val |= ( cfg->fine_synch & 0x1 ) << 6;        // Bit 6
    reg_val |= ( cfg->chirp_invert & 0x1 ) << 5;      // Bit 5
    reg_val |= ( cfg->enable & 0x1 ) << 4;            // Bit 4
    reg_val |= ( cfg->sf_log & 0xF );                 // Bits 3:0

    lr11xx_status_t status =
        lr11xx_regmem_write_regmem32( context, LR11XX_REG_LORA_MODEM_SIDEDET_0 + ( 4 * idx ), &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }

    return LR11XX_STATUS_OK;
}

lr11xx_status_t lr11xx_get_last_rx_status( const void* context, lr11xx_last_rx_status_t* rx_status )
{
    uint32_t        reg_val = 0;
    lr11xx_status_t status  = lr11xx_regmem_read_regmem32( context, LR11XX_REG_LORA_MODEM_RX_STATUS, &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }

    rx_status->last_ack_rx_ok   = ( reg_val >> 31 ) & 0x1;                                // Bit 31
    rx_status->rf_en_request    = ( reg_val >> 28 ) & 0x3;                                // Bits 29:28
    rx_status->last_cad         = ( reg_val >> 20 ) & 0xFF;                               // Bits 27:20
    rx_status->last_detect_path = ( lr11xx_detector_path_t )( ( reg_val >> 16 ) & 0x7 );  // Bits 18:16
    rx_status->payload_length   = ( reg_val >> 8 ) & 0xFF;                                // Bits 15:8
    rx_status->hdr_crc16_en     = ( reg_val >> 4 ) & 0x1;                                 // Bit 4
    rx_status->hdr_coding_rate  = ( reg_val >> 1 ) & 0x7;                                 // Bits 3:1
    rx_status->hdr_signals_ppm  = reg_val & 0x1;                                          // Bit 0

    return LR11XX_STATUS_OK;
}

lr11xx_status_t lr11xx_set_agc_freeze_after_sync( const void* context )
{
    lr11xx_status_t status;
    uint32_t        reg_val = 0;

    /* Freeze RF AGC after synchronisation */
    status = lr11xx_regmem_read_regmem32( context, LR11XX_REG_LORA_MODEM_RX_CFG, &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }
    uint8_t rx_cfg_freeze_rf_agc = ( reg_val >> 24 ) & 0x3;
    if( rx_cfg_freeze_rf_agc != 3 )
    {
        reg_val = ( reg_val & ~( 0x3 << 24 ) ) | ( 3 << 24 ); /* clear bits 24:25 and set value to 3 */
        status  = lr11xx_regmem_write_regmem32( context, LR11XX_REG_LORA_MODEM_RX_CFG, &reg_val, 1 );
        if( status != LR11XX_STATUS_OK )
        {
            return LR11XX_STATUS_ERROR;
        }
    }

    /* Freeze DAGC after synchronisation */
    status = lr11xx_regmem_read_regmem32( context, LR11XX_REG_LORA_MODEM_RX_DFE_AGC, &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }
    uint8_t rx_dfe_agc_freeze_on_sync = ( reg_val >> 8 ) & 0x3;
    if( rx_dfe_agc_freeze_on_sync != 3 )
    {
        reg_val = ( reg_val & ~( 0x3 << 8 ) ) | ( 3 << 8 ); /* clear bits 8:9 and set value to 3 */
        status  = lr11xx_regmem_write_regmem32( context, LR11XX_REG_LORA_MODEM_RX_DFE_AGC, &reg_val, 1 );
        if( status != LR11XX_STATUS_OK )
        {
            return LR11XX_STATUS_ERROR;
        }
    }

    /* Freeze DAGC after synchronisation */
    status = lr11xx_regmem_read_regmem32( context, LR11XX_REG_LORA_MODEM_DAGC_CFG, &reg_val, 1 );
    if( status != LR11XX_STATUS_OK )
    {
        return LR11XX_STATUS_ERROR;
    }
    uint8_t dagc_cfg_freeze_on_sync = ( reg_val >> 0 ) & 0x3;
    if( dagc_cfg_freeze_on_sync != 3 )
    {
        reg_val = ( reg_val & ~( 0x3 << 0 ) ) | ( 3 << 0 ); /* clear bits 0:1 and set value to 3 */
        status  = lr11xx_regmem_write_regmem32( context, LR11XX_REG_LORA_MODEM_DAGC_CFG, &reg_val, 1 );
        if( status != LR11XX_STATUS_OK )
        {
            return LR11XX_STATUS_ERROR;
        }
    }

    return LR11XX_STATUS_OK;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
