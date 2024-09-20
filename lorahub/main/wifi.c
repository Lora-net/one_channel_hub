/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
(C)2024 Semtech

Description:
    LoRaHub Packet Forwarder WiFi helpers

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_mac.h>

#if CONFIG_WIFI_PROV_OVER_BLE
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"
#endif  // CONFIG_WIFI_PROV_OVER_BLE

#include "wifi.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#if CONFIG_WIFI_PROV_OVER_BLE

/* WiFi provisioning over BLE */
#define EXAMPLE_PROV_SEC2_USERNAME "wifiprov"
#define EXAMPLE_PROV_SEC2_PWD "abcd1234"
#define PROV_TRANSPORT_BLE "ble"
#define PROV_QR_VERSION "v1"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

/* This salt,verifier has been generated for username = "wifiprov" and password = "abcd1234"
 * IMPORTANT NOTE: For production cases, this must be unique to every device
 * and should come from device manufacturing partition.*/
static const char sec2_salt[] = { 0x03, 0x6e, 0xe0, 0xc7, 0xbc, 0xb9, 0xed, 0xa8,
                                  0x4c, 0x9e, 0xac, 0x97, 0xd9, 0x3d, 0xec, 0xf4 };

static const char sec2_verifier[] = {
    0x7c, 0x7c, 0x85, 0x47, 0x65, 0x08, 0x94, 0x6d, 0xd6, 0x36, 0xaf, 0x37, 0xd7, 0xe8, 0x91, 0x43, 0x78, 0xcf, 0xfd,
    0x61, 0x6c, 0x59, 0xd2, 0xf8, 0x39, 0x08, 0x12, 0x72, 0x38, 0xde, 0x9e, 0x24, 0xa4, 0x70, 0x26, 0x1c, 0xdf, 0xa9,
    0x03, 0xc2, 0xb2, 0x70, 0xe7, 0xb1, 0x32, 0x24, 0xda, 0x11, 0x1d, 0x97, 0x18, 0xdc, 0x60, 0x72, 0x08, 0xcc, 0x9a,
    0xc9, 0x0c, 0x48, 0x27, 0xe2, 0xae, 0x89, 0xaa, 0x16, 0x25, 0xb8, 0x04, 0xd2, 0x1a, 0x9b, 0x3a, 0x8f, 0x37, 0xf6,
    0xe4, 0x3a, 0x71, 0x2e, 0xe1, 0x27, 0x86, 0x6e, 0xad, 0xce, 0x28, 0xff, 0x54, 0x46, 0x60, 0x1f, 0xb9, 0x96, 0x87,
    0xdc, 0x57, 0x40, 0xa7, 0xd4, 0x6c, 0xc9, 0x77, 0x54, 0xdc, 0x16, 0x82, 0xf0, 0xed, 0x35, 0x6a, 0xc4, 0x70, 0xad,
    0x3d, 0x90, 0xb5, 0x81, 0x94, 0x70, 0xd7, 0xbc, 0x65, 0xb2, 0xd5, 0x18, 0xe0, 0x2e, 0xc3, 0xa5, 0xf9, 0x68, 0xdd,
    0x64, 0x7b, 0xb8, 0xb7, 0x3c, 0x9c, 0xfc, 0x00, 0xd8, 0x71, 0x7e, 0xb7, 0x9a, 0x7c, 0xb1, 0xb7, 0xc2, 0xc3, 0x18,
    0x34, 0x29, 0x32, 0x43, 0x3e, 0x00, 0x99, 0xe9, 0x82, 0x94, 0xe3, 0xd8, 0x2a, 0xb0, 0x96, 0x29, 0xb7, 0xdf, 0x0e,
    0x5f, 0x08, 0x33, 0x40, 0x76, 0x52, 0x91, 0x32, 0x00, 0x9f, 0x97, 0x2c, 0x89, 0x6c, 0x39, 0x1e, 0xc8, 0x28, 0x05,
    0x44, 0x17, 0x3f, 0x68, 0x02, 0x8a, 0x9f, 0x44, 0x61, 0xd1, 0xf5, 0xa1, 0x7e, 0x5a, 0x70, 0xd2, 0xc7, 0x23, 0x81,
    0xcb, 0x38, 0x68, 0xe4, 0x2c, 0x20, 0xbc, 0x40, 0x57, 0x76, 0x17, 0xbd, 0x08, 0xb8, 0x96, 0xbc, 0x26, 0xeb, 0x32,
    0x46, 0x69, 0x35, 0x05, 0x8c, 0x15, 0x70, 0xd9, 0x1b, 0xe9, 0xbe, 0xcc, 0xa9, 0x38, 0xa6, 0x67, 0xf0, 0xad, 0x50,
    0x13, 0x19, 0x72, 0x64, 0xbf, 0x52, 0xc2, 0x34, 0xe2, 0x1b, 0x11, 0x79, 0x74, 0x72, 0xbd, 0x34, 0x5b, 0xb1, 0xe2,
    0xfd, 0x66, 0x73, 0xfe, 0x71, 0x64, 0x74, 0xd0, 0x4e, 0xbc, 0x51, 0x24, 0x19, 0x40, 0x87, 0x0e, 0x92, 0x40, 0xe6,
    0x21, 0xe7, 0x2d, 0x4e, 0x37, 0x76, 0x2f, 0x2e, 0xe2, 0x68, 0xc7, 0x89, 0xe8, 0x32, 0x13, 0x42, 0x06, 0x84, 0x84,
    0x53, 0x4a, 0xb3, 0x0c, 0x1b, 0x4c, 0x8d, 0x1c, 0x51, 0x97, 0x19, 0xab, 0xae, 0x77, 0xff, 0xdb, 0xec, 0xf0, 0x10,
    0x95, 0x34, 0x33, 0x6b, 0xcb, 0x3e, 0x84, 0x0f, 0xb9, 0xd8, 0x5f, 0xb8, 0xa0, 0xb8, 0x55, 0x53, 0x3e, 0x70, 0xf7,
    0x18, 0xf5, 0xce, 0x7b, 0x4e, 0xbf, 0x27, 0xce, 0xce, 0xa8, 0xb3, 0xbe, 0x40, 0xc5, 0xc5, 0x32, 0x29, 0x3e, 0x71,
    0x64, 0x9e, 0xde, 0x8c, 0xf6, 0x75, 0xa1, 0xe6, 0xf6, 0x53, 0xc8, 0x31, 0xa8, 0x78, 0xde, 0x50, 0x40, 0xf7, 0x62,
    0xde, 0x36, 0xb2, 0xba
};

#endif  // CONFIG_WIFI_PROV_OVER_BLE

static const char* TAG_WIFI = "wifi";

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static EventGroupHandle_t wifi_event_group;
static int                wifi_connect_retry_num = 0;

static wifi_status_t wifi_status = WIFI_STATUS_UNKNOWN;

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void event_handler( void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data )
{
    if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START )
    {
        esp_wifi_connect( );
    }
    else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED )
    {
        wifi_event_sta_disconnected_t* disconnected_event = ( wifi_event_sta_disconnected_t* ) event_data;
        ESP_LOGI( TAG_WIFI, "WIFI_EVENT_STA_DISCONNECTED" );
        ESP_LOGI( TAG_WIFI, "Disconnected from %s for reason %u", disconnected_event->ssid,
                  disconnected_event->reason );
        wifi_status = WIFI_STATUS_DISCONNECTED;
        if( wifi_connect_retry_num < CONFIG_WIFI_CONNECT_MAXIMUM_RETRY )
        {
            esp_wifi_connect( );
            wifi_connect_retry_num++;
            ESP_LOGI( TAG_WIFI, "retry to connect to the AP (%d)", wifi_connect_retry_num );
        }
        else
        {
            xEventGroupSetBits( wifi_event_group, WIFI_FAIL_BIT );
        }
    }
    else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED )
    {
        wifi_event_sta_connected_t* event = ( wifi_event_sta_connected_t* ) event_data;
        ESP_LOGI( TAG_WIFI, "WIFI_EVENT_STA_CONNECTED" );
        ESP_LOGI( TAG_WIFI, "Connected to %s (BSSID: " MACSTR ", Channel: %d)", event->ssid, MAC2STR( event->bssid ),
                  event->channel );
    }
    else if( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP )
    {
        wifi_status              = WIFI_STATUS_CONNECTED;
        ip_event_got_ip_t* event = ( ip_event_got_ip_t* ) event_data;
        ESP_LOGI( TAG_WIFI, "got ip:" IPSTR, IP2STR( &event->ip_info.ip ) );
        wifi_connect_retry_num = 0;
        xEventGroupSetBits( wifi_event_group, WIFI_CONNECTED_BIT );
    }
#if CONFIG_WIFI_PROV_OVER_BLE
    else if( event_base == PROTOCOMM_TRANSPORT_BLE_EVENT )
    {
        switch( event_id )
        {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI( TAG_WIFI, "BLE transport: Connected!" );
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI( TAG_WIFI, "BLE transport: Disconnected!" );
            break;
        default:
            break;
        }
    }
    else if( event_base == PROTOCOMM_SECURITY_SESSION_EVENT )
    {
        switch( event_id )
        {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI( TAG_WIFI, "Secured session established!" );
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE( TAG_WIFI, "Received invalid security parameters for establishing secure session!" );
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE( TAG_WIFI, "Received incorrect username and/or PoP for establishing secure session!" );
            break;
        default:
            break;
        }
    }
#endif  // CONFIG_WIFI_PROV_OVER_BLE
}

#if CONFIG_WIFI_PROV_OVER_BLE

static void get_device_service_name( char* service_name, size_t max )
{
    uint8_t     eth_mac[6];
    const char* ssid_prefix = "LRHB_";
    esp_wifi_get_mac( WIFI_IF_STA, eth_mac );
    snprintf( service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5] );
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t custom_prov_data_handler( uint32_t session_id, const uint8_t* inbuf, ssize_t inlen, uint8_t** outbuf,
                                    ssize_t* outlen, void* priv_data )
{
    if( inbuf )
    {
        ESP_LOGI( TAG_WIFI, "Received data: %.*s", inlen, ( char* ) inbuf );
    }
    char response[] = "SUCCESS";
    *outbuf         = ( uint8_t* ) strdup( response );
    if( *outbuf == NULL )
    {
        ESP_LOGE( TAG_WIFI, "System out of memory" );
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen( response ) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

static void get_sec2_salt( const char** salt, uint16_t* salt_len )
{
    ESP_LOGI( TAG_WIFI, "Development mode: using hard coded salt" );
    *salt     = sec2_salt;
    *salt_len = sizeof( sec2_salt );
}

static void get_sec2_verifier( const char** verifier, uint16_t* verifier_len )
{
    ESP_LOGI( TAG_WIFI, "Development mode: using hard coded verifier" );
    *verifier     = sec2_verifier;
    *verifier_len = sizeof( sec2_verifier );
}

static void wifi_prov_print_qr( const char* name, const char* username, const char* pop, const char* transport )
{
    if( !name || !transport )
    {
        ESP_LOGW( TAG_WIFI, "Cannot generate QR code payload. Data missing." );
        return;
    }
    char payload[150] = { 0 };
    if( pop )
    {
        snprintf( payload, sizeof( payload ),
                  "{\"ver\":\"%s\",\"name\":\"%s\""
                  ",\"username\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
                  PROV_QR_VERSION, name, username, pop, transport );
    }
    else
    {
        snprintf( payload, sizeof( payload ),
                  "{\"ver\":\"%s\",\"name\":\"%s\""
                  ",\"transport\":\"%s\"}",
                  PROV_QR_VERSION, name, transport );
    }

    /* Show QR code */
    ESP_LOGI( TAG_WIFI, "Scan this QR code from the provisioning application for Provisioning." );
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT( );
    esp_qrcode_generate( &cfg, payload );

    ESP_LOGI( TAG_WIFI, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s",
              QRCODE_BASE_URL, payload );
}

#endif  // CONFIG_WIFI_PROV_OVER_BLE

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int wifi_sta_init( bool reset_provisioning )
{
    esp_err_t err;
    wifi_event_group = xEventGroupCreate( );

    /* esp_netif_init() must have been called before */

    /* Create loop to received WiFi / TCP/IP events */
    err = esp_event_loop_create_default( );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_event_loop_create_default failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }

#if CONFIG_WIFI_PROV_OVER_BLE
    /* Register event handler for WiFi provisioning */
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL ) );
    ESP_ERROR_CHECK(
        esp_event_handler_register( PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL ) );
    ESP_ERROR_CHECK(
        esp_event_handler_register( PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL ) );
#endif

    /* Register event handler for WiFi events */
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL ) );
    ESP_ERROR_CHECK( esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL ) );

    /* Bind WiFi driver to network interface instance */
    esp_netif_create_default_wifi_sta( );

    /* Initialize WiFi - Allocate resource for WiFi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT( );
    err                    = esp_wifi_init( &cfg );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_init failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }

#if CONFIG_WIFI_PROV_OVER_BLE
    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = { .scheme               = wifi_prov_scheme_ble,
                                      .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM };
    /* Initialize provisioning manager with the configuration parameters set above */
    err = wifi_prov_mgr_init( config );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: wifi_prov_mgr_init failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }

    bool provisioned = false;
    if( reset_provisioning == true )
    {
        ESP_LOGI( TAG_WIFI, "Reset provisioning" );
        wifi_prov_mgr_reset_provisioning( );
    }
    else
    {
        /* Let's find out if the device is provisioned */
        err = wifi_prov_mgr_is_provisioned( &provisioned );
        if( err != ESP_OK )
        {
            ESP_LOGE( TAG_WIFI, "ERROR: wifi_prov_mgr_is_provisioned failed with %s\n", esp_err_to_name( err ) );
            return -1;
        }
    }
    /* If device is not yet provisioned start provisioning service */
    if( !provisioned )
    {
        ESP_LOGI( TAG_WIFI, "Starting provisioning" );

        char service_name[12];
        get_device_service_name( service_name, sizeof( service_name ) );

        wifi_prov_security_t security = WIFI_PROV_SECURITY_2;
        /* The username must be the same one, which has been used in the generation of salt and verifier */

        /* This pop field represents the password that will be used to generate salt and verifier.
         * The field is present here in order to generate the QR code containing password.
         * In production this password field shall not be stored on the device */
        const char* username = EXAMPLE_PROV_SEC2_USERNAME;
        const char* pop      = EXAMPLE_PROV_SEC2_PWD;

        /* This is the structure for passing security parameters for the protocomm security 2.
         * If dynamically allocated, sec2_params pointer and its content must be valid till WIFI_PROV_END event is
         * triggered. */
        wifi_prov_security2_params_t sec2_params = { };
        get_sec2_salt( &sec2_params.salt, &sec2_params.salt_len );
        get_sec2_verifier( &sec2_params.verifier, &sec2_params.verifier_len );
        wifi_prov_security2_params_t* sec_params = &sec2_params;

        const char* service_key = NULL; /* Ignored for BLE scheme */

        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        wifi_prov_scheme_ble_set_service_uuid( custom_service_uuid );

        wifi_prov_mgr_endpoint_create( "custom-data" );

        /* Start provisioning service */
        err = wifi_prov_mgr_start_provisioning( security, ( const void* ) sec_params, service_name, service_key );
        if( err != ESP_OK )
        {
            ESP_LOGE( TAG_WIFI, "ERROR: wifi_prov_mgr_start_provisioning failed with %s\n", esp_err_to_name( err ) );
            return -1;
        }

        wifi_prov_mgr_endpoint_register( "custom-data", custom_prov_data_handler, NULL );

        wifi_prov_print_qr( service_name, username, pop, PROV_TRANSPORT_BLE );
    }
    else
    {
        ESP_LOGI( TAG_WIFI, "Already provisioned, starting Wi-Fi STA" );

        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit( );

        /* Set WiFi Station mode (client) */
        err = esp_wifi_set_mode( WIFI_MODE_STA );
        if( err != ESP_OK )
        {
            ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_set_mode failed with %s\n", esp_err_to_name( err ) );
            return -1;
        }

        /* Start WiFi */
        err = esp_wifi_start( );
        if( err != ESP_OK )
        {
            ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_start failed with %s\n", esp_err_to_name( err ) );
            return -1;
        }
    }
#else
    /* Set WiFi Station mode (client) */
    err = esp_wifi_set_mode( WIFI_MODE_STA );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_set_mode failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    err = esp_wifi_set_config( WIFI_IF_STA, &wifi_config );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_set_config failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }

    /* Start WiFi */
    err = esp_wifi_start( );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_start failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }
#endif

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits =
        xEventGroupWaitBits( wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY );

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if( bits & WIFI_CONNECTED_BIT )
    {
        ESP_LOGI( TAG_WIFI, "connected to AP" );
        return 0;
    }
    else if( bits & WIFI_FAIL_BIT )
    {
        ESP_LOGE( TAG_WIFI, "Failed to connect to AP" );
        return -1;
    }
    else
    {
        ESP_LOGW( TAG_WIFI, "UNEXPECTED EVENT" );
        return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int wifi_get_mac_address( uint8_t mac_address[6] )
{
    esp_err_t err;

    err = esp_wifi_get_mac( WIFI_IF_STA, mac_address );
    if( err != ESP_OK )
    {
        ESP_LOGE( TAG_WIFI, "ERROR: esp_wifi_get_mac failed with %s\n", esp_err_to_name( err ) );
        return -1;
    }
    ESP_LOG_BUFFER_HEX_LEVEL( TAG_WIFI, mac_address, 6, ESP_LOG_INFO );

    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

wifi_status_t wifi_get_status( void )
{
    return wifi_status;
}