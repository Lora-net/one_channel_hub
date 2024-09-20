/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2024 Semtech

Description:
    Configure LoRaHub and forward packets to a server over UDP.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <stdint.h>  /* C99 types */
#include <stdbool.h> /* bool type */
#include <stdlib.h>  /* atoi, exit */

#include <sys/types.h>
#include <sys/socket.h> /* socket specific definitions */
#include <netdb.h>
#include <arpa/inet.h> /* IP address conversion stuff */
#include <pthread.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_pthread.h>
#include <driver/gpio.h>
#include <driver/temperature_sensor.h>

/* WiFi, SNTP, ... */
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>

/* Services */
#include "wifi.h"
#include "display.h"
#include "http_server.h"
#include "pkt_fwd.h"

#include "lorahub_version.h"
#include "main_defs.h"
#include "config_nvs.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* Board specific constants */
#ifdef CONFIG_SEMTECH_DEVKIT
#define USER_LED_GPIO 38 /* RGB LED from Display shield */
#else                    // CONFIG_HELTEC_WIFI_LORA_32_V3
#define USER_LED_GPIO 35
#endif
#define USER_BUTTON_GPIO 0

/* ESP32 logging tags */
static const char* TAG_MAIN = "main";

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES (GLOBAL) ------------------------------------------- */

/* signal handling variables */
volatile bool exit_sig = false; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */

/* SNTP configuration */
static char ntp_serv_addr[64] = CONFIG_SNTP_SERVER_ADDRESS; /* address of the SNTP server (host name or IPv4) */

/* Board specific */
static int user_button_pressed = 0; /* Number of time the user button has been pressed */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

static void configure_user_led( void );
static void set_user_led( bool on );
static void configure_user_button( void );

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

void wait_on_error( lorahub_error_t error, int line )
{
    bool led_status = true;

    /* Send error code to display (if available) */
    display_error_t err = { .err = error, .line = line };
    display_update_error( &err );

    /* loop and blink LED forever */
    while( 1 )
    {
        led_status = !led_status;
        set_user_led( led_status );
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    };
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void IRAM_ATTR gpio_interrupt_handler( void* args )
{
    user_button_pressed += 1;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void configure_user_led( void )
{
#ifdef USER_LED_GPIO
    gpio_reset_pin( USER_LED_GPIO );
    gpio_set_direction( USER_LED_GPIO, GPIO_MODE_OUTPUT );
    gpio_set_level( USER_LED_GPIO, 0 );
#endif
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void set_user_led( bool on )
{
#ifdef USER_LED_GPIO
    gpio_set_level( USER_LED_GPIO, ( on == true ) ? 1 : 0 );
#endif
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void configure_user_button( void )
{
    gpio_reset_pin( USER_BUTTON_GPIO );
    gpio_set_direction( USER_BUTTON_GPIO, GPIO_MODE_INPUT );
    gpio_set_intr_type( USER_BUTTON_GPIO, GPIO_INTR_POSEDGE );

    gpio_install_isr_service( 0 );
    gpio_isr_handler_add( USER_BUTTON_GPIO, gpio_interrupt_handler, NULL );
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void thread_display( void )
{
    ESP_LOGI( TAG_MAIN, "start Display thread" );

#if defined( CONFIG_GATEWAY_DISPLAY )
    display_init( );
#else
    ESP_LOGW( TAG_MAIN, "No display initialized" );
    return;
#endif

    while( !exit_sig )
    {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
        display_refresh( );
    }
}

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

void app_main( )
{
    int                         i; /* loop variable and temporary variable for return value */
    esp_err_t                   esp_err;
    temperature_sensor_handle_t temp_sensor             = NULL;
    bool                        reset_wifi_provisioning = false;

    /* threads */
    pthread_t thrid_display;

    /* display version informations */
    ESP_LOGI( TAG_MAIN, "*** LoRaHUB v%s ***", LORAHUB_FW_VERSION_STR );

    /* Initialize display */
    i = pthread_create( &thrid_display, NULL, ( void* ( * ) ( void* ) ) thread_display, NULL );
    if( i != 0 )
    {
        ESP_LOGE( TAG_MAIN, "ERROR: [main] impossible to create display thread\n" );
        wait_on_error( LRHB_ERROR_OS, __LINE__ );
    }

    /* Update display */
    display_update_status( DISPLAY_STATUS_INITIALIZING );

    /* configure LED */
    configure_user_led( );

    /* configure user button for wifi provisioning reset */
    configure_user_button( );

    /* Initialize NVS to store WiFi configuration */
    ESP_ERROR_CHECK( nvs_flash_init( ) );

    /* Initialize the underlying TCP/IP stack */
    ESP_ERROR_CHECK( esp_netif_init( ) );

    /* Wait for user request for WiFi provisioning reset */
#if CONFIG_WIFI_PROV_OVER_BLE
    uint8_t wait_prov_nb        = 0;
    uint8_t wait_prov_delay_max = ( uint8_t ) CONFIG_WIFI_PROV_DELAY_S;
    set_user_led( true );
    while( ( wait_prov_nb < wait_prov_delay_max ) && ( user_button_pressed == 0 ) )
    {
        ESP_LOGI( TAG_MAIN, "WiFi provisioning reset? (%u)", wait_prov_delay_max - wait_prov_nb );
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
        wait_prov_nb += 1;
    }
    if( user_button_pressed > 0 )
    {
        reset_wifi_provisioning = true;
        /* Update status for display */
        display_update_status( DISPLAY_STATUS_WIFI_PROVISIONING );
    }
    set_user_led( false );
#endif  // CONFIG_WIFI_PROV_OVER_BLE

    /* Initialize WiFi driver and configure for STAtion mode */
    i = wifi_sta_init( reset_wifi_provisioning );
    if( i != 0 )
    {
        ESP_LOGE( TAG_MAIN, "ERROR: failed to initialize WiFi station\n" );
        wait_on_error( LRHB_ERROR_WIFI, __LINE__ );
    }

    /* Update display */
    display_update_status( DISPLAY_STATUS_INITIALIZING );

    /* Initialize SNTP to get time from network */
    if( wifi_get_status( ) == WIFI_STATUS_CONNECTED )
    {
        /* Configure LNS address and port from NVS */
#ifdef CONFIG_GET_CFG_FROM_FLASH
        ESP_LOGI( TAG_MAIN, "Get SNTP sever address from NVS\n" );

        /* Get configuration from NVS */
        printf( "Opening Non-Volatile Storage (NVS) handle for reading... " );
        nvs_handle_t my_handle;
        esp_err = nvs_open( "storage", NVS_READONLY, &my_handle );
        if( esp_err != ESP_OK )
        {
            printf( "Error (%s) opening NVS handle!\n", esp_err_to_name( esp_err ) );
        }
        else
        {
            printf( "Done\n" );

            size_t size = sizeof( ntp_serv_addr );
            esp_err     = nvs_get_str( my_handle, CFG_NVS_KEY_SNTP_ADDRESS, ntp_serv_addr, &size );
            if( esp_err == ESP_OK )
            {
                printf( "NVS -> %s = %s\n", CFG_NVS_KEY_SNTP_ADDRESS, ntp_serv_addr );
            }
            else
            {
                printf( "Failed to get %s from NVS - %s\n", CFG_NVS_KEY_SNTP_ADDRESS, esp_err_to_name( esp_err ) );
            }
        }
        nvs_close( my_handle );
        printf( "Closed NVS handle for reading.\n" );
#endif  // CONFIG_GET_CFG_FROM_FLASH
        ESP_LOGI( TAG_MAIN, "Initializing SNTP from %s...", ntp_serv_addr );
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG( ntp_serv_addr );
        esp_netif_sntp_init( &config );
        if( ( esp_err = esp_netif_sntp_sync_wait( pdMS_TO_TICKS( 10000 ) ) ) != ESP_OK )
        {
            ESP_LOGE( TAG_MAIN, "ERROR: SNTP timeout with %s", esp_err_to_name( esp_err ) );
        }
        else
        {
            ESP_LOGI( TAG_MAIN, "SNTP initialized." );
        }
    }

    /* Initialize HTTP server */
    http_server_init( );

    /* Initialize temperature sensor (before running the pkt fwd thread) */
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT( -10, 80 );
    ESP_ERROR_CHECK( temperature_sensor_install( &temp_sensor_config, &temp_sensor ) );
    ESP_ERROR_CHECK( temperature_sensor_enable( temp_sensor ) );

    /* Start Packet Forwarder */
    launch_pkt_fwd( temp_sensor );

    while( !exit_sig )
    {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }

    /* ----------------------------------------------- */
    /* no need to clean anything as we will restart ;) */
    /* ----------------------------------------------- */

    ESP_LOGI( TAG_MAIN, "INFO: Exiting LoRaHUB\n" );

    /* Reset the ESP32 */
    esp_restart( );
}
