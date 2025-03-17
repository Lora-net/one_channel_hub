/*!
 * @file      lr11xx_driver_extension.h
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

#ifndef LR11XX_DRIVER_EXTENSION_H
#define LR11XX_DRIVER_EXTENSION_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "lr11xx_types.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

typedef enum lr11xx_detector_path_e
{
    LR11XX_LORA_DETECT_PATH_MAIN   = 0,  //!< Main detector
    LR11XX_LORA_DETECT_PATH_SIDE_0 = 1,  //!< Side detector 0
    LR11XX_LORA_DETECT_PATH_SIDE_1 = 2,  //!< Side detector 1
    LR11XX_LORA_DETECT_PATH_SIDE_2 = 3,  //!< Side detector 2
} lr11xx_detector_path_t;

typedef struct lr11xx_side_detector_cfg_s
{
    uint8_t half_bin;          // Bit 31
    uint8_t msp_peak_nb;       // Bits 30:28
    uint8_t msp_pnr;           // Bits 26:20
    uint8_t peak2_pos;         // Bits 19:15
    uint8_t peak1_pos;         // Bits 14:10
    uint8_t ppm_offset_hc;     // Bits 9:8
    uint8_t header_diff_mode;  // Bit 7
    uint8_t fine_synch;        // Bit 6
    uint8_t chirp_invert;      // Bit 5
    uint8_t enable;            // Bit 4
    uint8_t sf_log;            // Bits 3:0
} lr11xx_side_detector_cfg_t;

typedef struct lr11xx_last_rx_status_s
{
    uint8_t                last_ack_rx_ok;    // Bit 31
    uint8_t                rf_en_request;     // Bits 29:28
    uint8_t                last_cad;          // Bits 27:20
    lr11xx_detector_path_t last_detect_path;  // Bits 18:16
    uint8_t                payload_length;    // Bits 15:8
    uint8_t                hdr_crc16_en;      // Bit 4
    uint8_t                hdr_coding_rate;   // Bits 3:1
    uint8_t                hdr_signals_ppm;   // Bit 0
} lr11xx_last_rx_status_t;

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

lr11xx_status_t lr11xx_get_lora_side_detector_cfg( const void* context, uint8_t idx, lr11xx_side_detector_cfg_t* cfg );

lr11xx_status_t lr11xx_set_lora_side_detector_cfg( const void* context, uint8_t idx, lr11xx_side_detector_cfg_t* cfg );

lr11xx_status_t lr11xx_get_last_rx_status( const void* context, lr11xx_last_rx_status_t* rx_status );

lr11xx_status_t lr11xx_set_agc_freeze_after_sync( const void* context );

#ifdef __cplusplus
}
#endif

#endif  // LR11XX_DRIVER_EXTENSION_H

/* --- EOF ------------------------------------------------------------------ */
