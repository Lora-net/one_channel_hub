/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    LoRaHub HAL common auxiliary functions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _LORAHUB_AUX_H
#define _LORAHUB_AUX_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <stdint.h>   /* C99 types */
#include <stdbool.h>  /* bool type */
#include <sys/time.h> /* gettimeofday, structtimeval */

#include "esp_rom_sys.h"
#include "esp_log.h"

#include "ral.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

/**
@brief Get a particular bit value from a byte
@param b [in]   Any byte from which we want a bit value
@param p [in]   Position of the bit in the byte [0..7]
@param n [in]   Number of bits we want to get
@return The value corresponding the requested bits
*/
#define TAKE_N_BITS_FROM( b, p, n ) ( ( ( b ) >> ( p ) ) & ( ( 1 << ( n ) ) - 1 ) )

/**
@brief Wait for a certain time (microsecond accuracy)
@param us number of microseconds to wait.
*/
#define WAIT_US( us ) esp_rom_delay_us( us )

/**
@brief Wait for a certain time (millisecond accuracy)
@param ms number of milliseconds to wait.
*/
#define WAIT_MS( ms ) esp_rom_delay_us( ms * 1000 )

/*!
 * @brief Stringify constants
 */
#define xstr( a ) str( a )
#define str( a ) #a

/*!
 * @brief Helper macro that returned a human-friendly message if a command does not return RAL_STATUS_OK
 *
 * @remark The macro is implemented to be used with functions returning a @ref ral_status_t
 *
 * @param[in] rc  Return code
 */
#define ASSERT_RAL_RC( rc )                                                                                     \
    {                                                                                                           \
        const ral_status_t status = rc;                                                                         \
        if( status != RAL_STATUS_OK )                                                                           \
        {                                                                                                       \
            if( status == RAL_STATUS_ERROR )                                                                    \
            {                                                                                                   \
                ESP_LOGE( "RAL_STATUS", "In %s - %s (line %d): %s", __FILE__, __func__, __LINE__,               \
                          xstr( RAL_STATUS_ERROR ) );                                                           \
                return -1;                                                                                      \
            }                                                                                                   \
            else                                                                                                \
            {                                                                                                   \
                ESP_LOGW( "RAL_STATUS", "In %s - %s (line %d): Status code = %d", __FILE__, __func__, __LINE__, \
                          status );                                                                             \
                return -1;                                                                                      \
            }                                                                                                   \
        }                                                                                                       \
    }

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

uint32_t lgw_get_lora_bw_in_hz( uint8_t bw );

ral_lora_sf_t lgw_convert_hal_to_ral_sf( uint8_t sf );

ral_lora_bw_t lgw_convert_hal_to_ral_bw( uint8_t bw );

ral_lora_cr_t lgw_convert_hal_to_ral_cr( uint8_t cr );

int lgw_check_lora_mod_params( uint32_t freq_hz, uint8_t bw, uint8_t cr );

uint8_t lgw_get_lora_sync_word( uint32_t freq_hz, uint8_t sf );

int lgw_check_lora_dualsf_conf( uint8_t bw, uint8_t sf1, uint8_t sf2 );

#endif

/* --- EOF ------------------------------------------------------------------ */
