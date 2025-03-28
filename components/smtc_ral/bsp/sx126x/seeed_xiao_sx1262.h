/*!
 * @file      seeed_xiao_sx1262.h
 *
 * @brief     Interface specific to Seeed Xiao sx1262 board.
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2022. All rights reserved.
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
#ifndef SEEED_XIAO_SX1262_H
#define SEEED_XIAO_SX1262_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdbool.h>
#include <stdint.h>
#include "smtc_shield_sx126x_types.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

#define SEEED_XIAO_SX1262_INSTANTIATE                                                                       \
    {                                                                                                       \
        .get_pa_pwr_cfg           = seeed_xiao_sx1262_get_pa_pwr_cfg,                                       \
        .is_dio2_set_as_rf_switch = seeed_xiao_sx1262_is_dio2_set_as_rf_switch,                             \
        .get_reg_mode = seeed_xiao_sx1262_get_reg_mode, .get_xosc_cfg = seeed_xiao_sx1262_get_xosc_cfg,     \
        .get_pinout = seeed_xiao_sx1262_get_pinout, .get_capabilities = seeed_xiao_sx1262_get_capabilities, \
    }

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/**
 * @see smtc_shield_sx126x_get_pa_pwr_cfg
 */
const smtc_shield_sx126x_pa_pwr_cfg_t* seeed_xiao_sx1262_get_pa_pwr_cfg( const uint32_t rf_freq_in_hz,
                                                                         int8_t         expected_output_pwr_in_dbm );

/**
 * @see smtc_shield_sx126x_is_dio2_set_as_rf_switch
 */
bool seeed_xiao_sx1262_is_dio2_set_as_rf_switch( void );

/**
 * @see smtc_shield_sx126x_get_reg_mode
 */
sx126x_reg_mod_t seeed_xiao_sx1262_get_reg_mode( void );

/**
 * @see smtc_shield_sx126x_get_xosc_cfg
 */
const smtc_shield_sx126x_xosc_cfg_t* seeed_xiao_sx1262_get_xosc_cfg( void );

/**
 * @see smtc_shield_sx126x_get_pinout
 */
const smtc_shield_sx126x_pinout_t* seeed_xiao_sx1262_get_pinout( void );

/**
 * @see smtc_shield_sx126x_get_capabilities
 */
const smtc_shield_sx126x_capabilities_t* seeed_xiao_sx1262_get_capabilities( void );

#ifdef __cplusplus
}
#endif

#endif  // SEEED_XIAO_SX1262_H
