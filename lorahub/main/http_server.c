/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
(C)2024 Semtech

Description:
    LoRaHub HTTP server for configuration REST API and web interface

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <stdint.h>  /* C99 types */
#include <stdbool.h> /* bool type */
#include <string.h>

#include <esp_log.h>

#include <esp_http_server.h>

#include "http_server.h"
#include "wifi.h"
#include "parson.h"
#include "config_nvs.h"

#include "lorahub_aux.h"
#include "lorahub_hal.h"

#include "lorahub_version.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

static const char* TAG_WEB = "WEB";

const char html_header[] =
    "<head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<style type=\"text/css\">"
    "    * {"
    "        font-family: Arial, Helvetica, sans-serif;"
    "        font-size: x-large;"
    "    }"
    "    div {"
    "        border-radius: 5px;"
    "        background-color: #f2f2f2;"
    "        padding: 20px;"
    "    }"
    "    h2 {"
    "       color: #00AFAA;"
    "    }"
    "    input[type=text],input[type=number] {"
    "        width: 100%;"
    "        padding: 12px 20px;"
    "        margin: 8px 0;"
    "        display: inline-block;"
    "        border: 1px solid #ccc;"
    "        border-radius: 4px;"
    "        box-sizing: border-box;"
    "    }"
    "    input[type=radio] {"
    "        padding: 12px 20px;"
    "        margin: 8px 8px;"
    "    }"
    "    .btn_cfg {"
    "        background-color: #00afaa;"
    "        color: white;"
    "        padding: 14px 20px;"
    "        margin: 8px 8px;"
    "        border: none;"
    "        border-radius: 4px;"
    "        cursor: pointer;"
    "    }"
    "    .btn_cfg:hover {"
    "        background-color: #008c88;"
    "    }"
    "    .btn_rst {"
    "        background-color: #e74c3c;"
    "        color: white;"
    "        padding: 14px 20px;"
    "        margin: 8px 8px;"
    "        border: none;"
    "        border-radius: 4px;"
    "        cursor: pointer;"
    "    }"
    "    .btn_rst:hover {"
    "        background-color: #922b21;"
    "    }"
    "</style>"
    "</head>";

#define CHAN_DR_STR_MAX_SIZE ( 3 )       /* [7..12] + \0 */
#define CHAN_BW_STR_MAX_SIZE ( 4 )       /* [125,250,500] + \0 */
#define LNS_PORT_STR_MAX_SIZE ( 6 )      /* [0..65535] + \0 */
#define SUBMIT_VALUE_STR_MAX_SIZE ( 10 ) /* could be "configure" or "reboot" + \0 */
#define CHAN_FREQ_STR_MAX_SIZE ( 12 )    /* [150.000000..2500.000000] + \0 */

#define FORM_FIELD_VALUE_STR_MAX_SIZE ( CHAN_FREQ_STR_MAX_SIZE ) /* Max value of the above defines */
#define FORM_FIELD_NAME_STR_MAX_SIZE ( CFG_NVS_KEY_STR_MAX_SIZE )

#define FORM_FIELD_NB ( 8 ) /* Update this when adding new field returned by form */
/* Size of the following string must be < FORM_FIELD_REQ_STR_MAX_SIZE */
#define FORM_FIELD_NAME_LNS_ADDRESS CFG_NVS_KEY_LNS_ADDRESS
#define FORM_FIELD_NAME_LNS_PORT CFG_NVS_KEY_LNS_PORT
#define FORM_FIELD_NAME_CHAN_FREQ CFG_NVS_KEY_CHAN_FREQ
#define FORM_FIELD_NAME_CHAN_DR_1 CFG_NVS_KEY_CHAN_DR_1
#define FORM_FIELD_NAME_CHAN_DR_2 CFG_NVS_KEY_CHAN_DR_2
#define FORM_FIELD_NAME_CHAN_BW CFG_NVS_KEY_CHAN_BW
#define FORM_FIELD_NAME_SNTP_ADDRESS CFG_NVS_KEY_SNTP_ADDRESS
#define FORM_FIELD_NAME_SUBMIT "submit"

/* Maximum size of a configuration string resulting from the html web form */
#define FORM_FULL_CONTENT_MAX_SIZE                                                                          \
    ( ( FORM_FIELD_NB * 2 ) + ( FORM_FIELD_NB * FORM_FIELD_NAME_STR_MAX_SIZE ) + LNS_ADDRESS_STR_MAX_SIZE + \
      LNS_PORT_STR_MAX_SIZE + CHAN_FREQ_STR_MAX_SIZE + ( LGW_MULTI_SF_NB * CHAN_DR_STR_MAX_SIZE ) +         \
      CHAN_BW_STR_MAX_SIZE + SNTP_ADDRESS_STR_MAX_SIZE +                                                    \
      SUBMIT_VALUE_STR_MAX_SIZE ) /* sum of all fields max sizes + names + separators for each fields (=, &) */

/* Maximum size of a configuration string resulting from an API call in JSON format */
#define JSON_FULL_CONTENT_MAX_SIZE     \
    ( FORM_FULL_CONTENT_MAX_SIZE + 2 + \
      ( FORM_FIELD_NB *                \
        4 ) ) /* when converting a web form string to a json string, need to add {} and "" for each key/values */

/* Radio type configured */
#if defined( CONFIG_RADIO_TYPE_SX1261 )
const char* radio_type = "SX1261";
#elif defined( CONFIG_RADIO_TYPE_SX1262 )
const char* radio_type = "SX1262";
#elif defined( CONFIG_RADIO_TYPE_SX1268 )
const char* radio_type = "SX1268";
#elif defined( CONFIG_RADIO_TYPE_LLCC68 )
const char* radio_type = "LLCC68";
#elif defined( CONFIG_RADIO_TYPE_LR1121 )
const char* radio_type = "LR1121";
#else
const char* radio_type = "unknown";
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

typedef enum
{
    HTTP_POST_SRC_WEB_FORM,
    HTTP_POST_SRC_API
} http_post_src_t;

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static uint8_t web_inf_mac_addr[6]      = { 0 };
static char    web_inf_mac_addr_str[18] = "unknown";

static char post_content_form[FORM_FULL_CONTENT_MAX_SIZE] = { 0 };
static char post_content_json[JSON_FULL_CONTENT_MAX_SIZE] = { 0 };

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static esp_err_t http_root_get_handler( httpd_req_t* req )
{
    /* a string to hold the form field name property */
    char field_name_str[FORM_FIELD_NAME_STR_MAX_SIZE + 1] = { 0 };

    /* a string to hold the form field value property */
    char field_value_str[FORM_FIELD_VALUE_STR_MAX_SIZE + 1] = { 0 };

    /* a string to hold the form field max_length property */
    char field_maxlength_str[3 + 1] = { 0 }; /* hold a max_length of 999 max + null termination */

    ESP_LOGI( TAG_WEB, "root_get_handler req->uri=[%s]", req->uri );

    /* Get info to be displayed */
    wifi_get_mac_address( web_inf_mac_addr );
    snprintf( web_inf_mac_addr_str, sizeof web_inf_mac_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", web_inf_mac_addr[0],
              web_inf_mac_addr[1], web_inf_mac_addr[2], web_inf_mac_addr[3], web_inf_mac_addr[4], web_inf_mac_addr[5] );

    /* Get current loaded config */
    const lgw_nvs_cfg_t* nvs_config;
    lgw_nvs_get_config( &nvs_config );

    /* Send HTML header */
    httpd_resp_sendstr_chunk( req, "<!DOCTYPE html><html>" );
    httpd_resp_sendstr_chunk( req, html_header );
    httpd_resp_sendstr_chunk( req, "<body>" );

    /* Configuration form*/
    httpd_resp_sendstr_chunk( req, "<div>" );
    httpd_resp_sendstr_chunk( req, "<form method=\"post\" action=\"/submit\">" );

    /* HUB infos */
    httpd_resp_sendstr_chunk( req, "<h2>LoRaHUB info</h2>" );

    /* firmware version */
    httpd_resp_sendstr_chunk( req, "<label>fw version: " );
    httpd_resp_sendstr_chunk( req, LORAHUB_FW_VERSION_STR );
    httpd_resp_sendstr_chunk( req, "</label>" );
    httpd_resp_sendstr_chunk( req, "</br>" );

    /* radio type */
    httpd_resp_sendstr_chunk( req, "<label>radio type: " );
    httpd_resp_sendstr_chunk( req, radio_type );
    httpd_resp_sendstr_chunk( req, "</label>" );
    httpd_resp_sendstr_chunk( req, "</br>" );

    /* MAC address */
    httpd_resp_sendstr_chunk( req, "<label>MAC address: " );
    if( strlen( web_inf_mac_addr_str ) )
    {
        httpd_resp_sendstr_chunk( req, web_inf_mac_addr_str );
    }
    httpd_resp_sendstr_chunk( req, "</label>" );

    /* LNS configuration */
    httpd_resp_sendstr_chunk( req, "<h2>LoRaWAN network server</h2>" );

    /* LNS server address */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_ADDRESS );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\">address</label>" );

    httpd_resp_sendstr_chunk( req, "<input type=\"text\" id=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_ADDRESS );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_ADDRESS );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" ); /* close string */
    httpd_resp_sendstr_chunk( req, " maxlength=\"" );
    snprintf( field_maxlength_str, sizeof field_maxlength_str, "%d",
              LNS_ADDRESS_STR_MAX_SIZE - 1 ); /* keep 1 byte for null termination */
    httpd_resp_sendstr_chunk( req, field_maxlength_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( strlen( nvs_config->lns_address ) )
    {
        httpd_resp_sendstr_chunk( req, nvs_config->lns_address );
    }
    httpd_resp_sendstr_chunk( req, "\"><br>" );

    /* LNS server port */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_PORT );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\">port</label>" );

    httpd_resp_sendstr_chunk( req, "<input type=\"number\" id=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_PORT );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );
    httpd_resp_sendstr_chunk( req, " step=1 min=0 max=65535" ); /* 1 space prefix */
    httpd_resp_sendstr_chunk( req, " name=\"" );                /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_LNS_PORT );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( sizeof( field_value_str ) > LNS_PORT_STR_MAX_SIZE )
    {
        snprintf( field_value_str, sizeof field_value_str, "%u", nvs_config->lns_port );
        httpd_resp_sendstr_chunk( req, field_value_str );
    }
    httpd_resp_sendstr_chunk( req, "\"><br>" );

    /* RX channel configuration */
    httpd_resp_sendstr_chunk( req, "<h2>RX channel</h2>" );

    /* channel frequency */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_FREQ );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\">frequency (MHz)</label>" );

    httpd_resp_sendstr_chunk( req, "<input type=\"number\" id=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_FREQ );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    httpd_resp_sendstr_chunk( req, " step=\"any\" min=150 max=2500 lang=\"en\"" ); /* 1 space prefix */
#else
    httpd_resp_sendstr_chunk( req, " step=\"any\" min=150 max=960 lang=\"en\"" ); /* 1 space prefix */
#endif
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_FREQ );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( sizeof( field_value_str ) > CHAN_FREQ_STR_MAX_SIZE )
    {
        snprintf( field_value_str, sizeof field_value_str, "%.6f", ( double ) nvs_config->chan_freq_hz / 1e6 );
        httpd_resp_sendstr_chunk( req, field_value_str );
    }
    httpd_resp_sendstr_chunk( req, "\"><br>" );

    /* channel spreading factor 1 */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_1 );
    httpd_resp_sendstr_chunk( req, field_name_str );
#if defined( CONFIG_RADIO_TYPE_LR1121 )
    httpd_resp_sendstr_chunk( req, "\">spreading factor 1 - [5..12]</label>" );
#else
    httpd_resp_sendstr_chunk( req, "\">spreading factor - [5..12]</label>" );
#endif

    httpd_resp_sendstr_chunk( req, "<input type=\"number\" id=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_1 );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );
    httpd_resp_sendstr_chunk( req, " step=1 min=5 max=12" ); /* 1 space prefix */
    httpd_resp_sendstr_chunk( req, " name=\"" );             /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_1 );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( sizeof( field_value_str ) > CHAN_DR_STR_MAX_SIZE )
    {
        snprintf( field_value_str, sizeof field_value_str, "%u", nvs_config->chan_datarate_1 );
        httpd_resp_sendstr_chunk( req, field_value_str );
    }
    httpd_resp_sendstr_chunk( req, "\"><br>" );

#if defined( CONFIG_RADIO_TYPE_LR1121 )
    /* channel spreading factor 2 */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_2 );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\">spreading factor 2 - [5..12, 0:disable]</label>" );

    httpd_resp_sendstr_chunk( req, "<input type=\"number\" id=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_2 );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );
    httpd_resp_sendstr_chunk( req, " step=1 min=0 max=12" ); /* 1 space prefix */
    httpd_resp_sendstr_chunk( req, " name=\"" );             /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_DR_2 );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( sizeof( field_value_str ) > CHAN_DR_STR_MAX_SIZE )
    {
        snprintf( field_value_str, sizeof field_value_str, "%u", nvs_config->chan_datarate_2 );
        httpd_resp_sendstr_chunk( req, field_value_str );
    }
    httpd_resp_sendstr_chunk( req, "\">" );
    httpd_resp_sendstr_chunk( req, "<br>" );
#endif

    /* channel bandwidth */
    httpd_resp_sendstr_chunk( req, "<label>bandwidth</label>" );

    /* 125 khz */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_125\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"125\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 125 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<label for=\"bw_125\">125</label>" );

    /* 250 khz */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_250\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"250\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 250 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<label for=\"bw_250\">250</label>" );

    /* 500 khz */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_500\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"500\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 500 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<label for=\"bw_500\">500</label>" );

#if defined( CONFIG_RADIO_TYPE_LR1121 )
    /* 200 khz (for 2.4 GHz) */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_200\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"200\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 200 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<strong><label for=\"bw_200\">200</label></strong>" );

    /* 400 khz (for 2.4 GHz) */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_400\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"400\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 400 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<strong><label for=\"bw_400\">400</label></strong>" );

    /* 800 khz (for 2.4 GHz) */
    httpd_resp_sendstr_chunk( req, "<input type=\"radio\" id=\"bw_800\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_CHAN_BW );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );             /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"800\"" ); /* 1 space prefix */
    if( nvs_config->chan_bandwidth_khz == 800 )
    {
        httpd_resp_sendstr_chunk( req, " checked=\"checked\">" ); /* 1 space prefix */
    }
    else
    {
        httpd_resp_sendstr_chunk( req, ">" );
    }
    httpd_resp_sendstr_chunk( req, "<strong><strong><label for=\"bw_800\">800</label></strong>" );
#endif
    httpd_resp_sendstr_chunk( req, "<br>" );

    /* Miscellaneous */
    httpd_resp_sendstr_chunk( req, "<h2>Miscellaneous</h2>" );

    /* SNTP server address */
    httpd_resp_sendstr_chunk( req, "<label for=\"" );
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_SNTP_ADDRESS );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\">SNTP server address</label>" );

    httpd_resp_sendstr_chunk( req, "<input type=\"text\" id=\"sntp_address\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_SNTP_ADDRESS );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" ); /* close string */
    httpd_resp_sendstr_chunk( req, " maxlength=\"" );
    snprintf( field_maxlength_str, sizeof field_maxlength_str, "%d",
              SNTP_ADDRESS_STR_MAX_SIZE - 1 ); /* keep 1 byte for null termination */
    httpd_resp_sendstr_chunk( req, field_maxlength_str );
    httpd_resp_sendstr_chunk( req, "\"" );        /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"" ); /* 1 space prefix */
    if( strlen( nvs_config->sntp_address ) )
    {
        httpd_resp_sendstr_chunk( req, nvs_config->sntp_address );
    }
    httpd_resp_sendstr_chunk( req, "\"><br>" );

    /* Submit form button */
    httpd_resp_sendstr_chunk( req, "<br><input class=\"btn_cfg\" type=\"submit\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" ); /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_SUBMIT );
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );                    /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"configure\">" ); /* 1 space prefix */

    /* End of config form */
    httpd_resp_sendstr_chunk( req, "</form>" );

    /* Reboot button form */
    httpd_resp_sendstr_chunk( req, "<form method=\"post\" action=\"/reboot\">" );
    httpd_resp_sendstr_chunk( req, "<input class=\"btn_rst\" type=\"submit\"" );
    httpd_resp_sendstr_chunk( req, " name=\"" );                                     /* 1 space prefix */
    snprintf( field_name_str, sizeof field_name_str, "%s", FORM_FIELD_NAME_SUBMIT ); /* 1 space prefix */
    httpd_resp_sendstr_chunk( req, field_name_str );
    httpd_resp_sendstr_chunk( req, "\"" );                 /* close string */
    httpd_resp_sendstr_chunk( req, " value=\"reboot\">" ); /* 1 space prefix */
    httpd_resp_sendstr_chunk( req, "</form><br>" );

    /* End div */
    httpd_resp_sendstr_chunk( req, "</div>" );

    /* Send remaining chunk of HTML file to complete it */
    httpd_resp_sendstr_chunk( req, "</body></html>" );

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk( req, NULL );

    return ESP_OK;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void web_form_to_json( char* dest_json_str, char* src_form_data )
{
    dest_json_str[0] = '\0';

    // Start building the JSON string
    strcpy( dest_json_str, "{" );

    // Tokenize the form data
    char* token;
    char* saveptr;
    token = strtok_r( src_form_data, "&", &saveptr );
    while( token != NULL )
    {
        char* key;
        char* value;
        // Extract key and value
        key   = strtok( token, "=" );
        value = strtok( NULL, "=" );
        // Append key-value pair to JSON string
        strcat( dest_json_str, "\"" );
        strcat( dest_json_str, key );
        strcat( dest_json_str, "\":\"" );
        strcat( dest_json_str, value );
        strcat( dest_json_str, "\"," );
        token = strtok_r( NULL, "&", &saveptr );
    }

    // Remove the trailing comma if it exists
    if( dest_json_str[strlen( dest_json_str ) - 1] == ',' )
    {
        dest_json_str[strlen( dest_json_str ) - 1] = '\0';
    }

    // Add closing brace
    strcat( dest_json_str, "}" );
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* POSTMAN:
POST http://xxx.xxx.xxx.xxxx:8000/api/v1/set_config
{"lns_addr":"eu1.cloud.thethings.network","lns_port":1700,"chan_freq":868.1,"chan_dr":7,"chan_bw":125,"sntp_addr":"pool.ntp.org"}
*/

static esp_err_t set_config_post_handler( httpd_req_t* req )
{
    http_post_src_t post_src = ( http_post_src_t )( intptr_t ) req->user_ctx;
    JSON_Value*     root_val = NULL;
    JSON_Object*    root_obj = NULL;
    JSON_Value*     val      = NULL;
    char*           recv_buffer_ptr;
    size_t          recv_buffer_size;
    const char*     str;

    if( post_src == HTTP_POST_SRC_WEB_FORM )
    {
        ESP_LOGI( TAG_WEB, "%s: received POST set config from web form", __FUNCTION__ );
        recv_buffer_ptr  = post_content_form;
        recv_buffer_size = sizeof( post_content_form );
    }
    else
    {  // HTTP_POST_SRC_API
        ESP_LOGI( TAG_WEB, "%s: received POST set config from API", __FUNCTION__ );
        recv_buffer_ptr  = post_content_json;
        recv_buffer_size = sizeof( post_content_json );
    }

    ESP_LOGI( TAG_WEB, "%s: req->uri=%s", __FUNCTION__, req->uri );
    ESP_LOGI( TAG_WEB, "%s: content length %d (max:%d)", __FUNCTION__, req->content_len, recv_buffer_size );

    /* Get the HTTP request data */
    int total_len = req->content_len;
    int cur_len   = 0;
    int received  = 0;
    if( total_len >= recv_buffer_size )
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long" );
        return ESP_FAIL;
    }
    while( cur_len < total_len )
    {
        received = httpd_req_recv( req, recv_buffer_ptr + cur_len, total_len );
        if( received <= 0 )
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value" );
            return ESP_FAIL;
        }
        cur_len += received;
    }
    recv_buffer_ptr[total_len] = '\0';
    ESP_LOGI( TAG_WEB, "%s: content %s", __FUNCTION__, recv_buffer_ptr );

    /* Convert data to JSON if coming from WEB FORM */
    if( post_src == HTTP_POST_SRC_WEB_FORM )
    {
        if( sizeof( post_content_json ) > total_len )
        {
            ESP_LOGI( TAG_WEB, "%s: converting web form data to json string", __FUNCTION__ );
            web_form_to_json( post_content_json, recv_buffer_ptr );
            ESP_LOGI( TAG_WEB, "%s: content length %d (max:%d)", __FUNCTION__, strlen( post_content_json ),
                      sizeof( post_content_json ) );
            ESP_LOGI( TAG_WEB, "%s: content %s", __FUNCTION__, post_content_json );
        }
        else
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long to be converted to json" );
            return ESP_FAIL;
        }
    }

    /* Get current loaded config */
    const lgw_nvs_cfg_t* current_config;
    lgw_nvs_get_config( &current_config );
    lgw_nvs_cfg_t updated_config = *current_config; /* local alloc for update */

    /* Parse JSON */
    root_val = json_parse_string_with_comments( ( const char* ) ( post_content_json ) );
    if( root_val == NULL )
    {
        ESP_LOGW( TAG_WEB, "WARNING: invalid JSON, configuration failed" );
        httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, "Post configuration failed" );
        return ESP_FAIL;
    }
    else
    {
        if( json_value_get_type( root_val ) == JSONObject )
        {
            esp_err_t err = ESP_OK;

            root_obj = json_value_get_object( root_val );

            /* Get channel frequency (MHz) */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_CHAN_FREQ );
            if( val != NULL )
            {
                double          web_cfg_chan_freq_mhz;
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONNumber )
                {
                    web_cfg_chan_freq_mhz = json_value_get_number( val );
                }
                else if( val_type == JSONString )
                {
                    web_cfg_chan_freq_mhz = atof( json_value_get_string( val ) );
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed", FORM_FIELD_NAME_CHAN_FREQ,
                              val_type );
                    err = ESP_FAIL;
                }
                /* sanity check */
                if( err == ESP_OK )
                {
                    printf( "%s:%.6f\n", FORM_FIELD_NAME_CHAN_FREQ, web_cfg_chan_freq_mhz );
#if defined( CONFIG_RADIO_TYPE_LR1121 )
                    if( ( web_cfg_chan_freq_mhz < 150.0 ) || ( web_cfg_chan_freq_mhz > 2500.0 ) )
#else
                    if( ( web_cfg_chan_freq_mhz < 150.0 ) || ( web_cfg_chan_freq_mhz > 960.0 ) )
#endif
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - out of range, configuration failed",
                                  FORM_FIELD_NAME_CHAN_FREQ );
                        err = ESP_FAIL;
                    }
                    else
                    {
                        /* Update context to be stored in NVS */
                        updated_config.chan_freq_hz = ( uint32_t )( ( double ) ( 1.0e6 ) * web_cfg_chan_freq_mhz );
                    }
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_CHAN_FREQ );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }

            /* Get channel datarate 1 */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_CHAN_DR_1 );
            if( val != NULL )
            {
                double          val_num;
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONNumber )
                {
                    val_num = json_value_get_number( val );
                }
                else if( val_type == JSONString )
                {
                    val_num = atof( json_value_get_string( val ) );
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed", FORM_FIELD_NAME_CHAN_DR_1,
                              val_type );
                    err = ESP_FAIL;
                }
                /* sanity check */
                if( err == ESP_OK )
                {
                    printf( "%s:%.0f\n", FORM_FIELD_NAME_CHAN_DR_1, val_num );
                    if( ( val_num < 5 ) || ( val_num > 12 ) )
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - out of range, configuration failed",
                                  FORM_FIELD_NAME_CHAN_DR_1 );
                        err = ESP_FAIL;
                    }
                    else
                    {
                        updated_config.chan_datarate_1 = ( uint32_t ) val_num;
                    }
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_CHAN_DR_1 );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }

#if defined( CONFIG_RADIO_TYPE_LR1121 )
            /* Get channel datarate 2 */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_CHAN_DR_2 );
            if( val != NULL )
            {
                double          val_num;
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONNumber )
                {
                    val_num = json_value_get_number( val );
                }
                else if( val_type == JSONString )
                {
                    val_num = atof( json_value_get_string( val ) );
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed", FORM_FIELD_NAME_CHAN_DR_2,
                              val_type );
                    err = ESP_FAIL;
                }
                /* sanity check */
                if( err == ESP_OK )
                {
                    printf( "%s:%.0f\n", FORM_FIELD_NAME_CHAN_DR_2, val_num );
                    if( ( val_num != 0 ) && ( ( val_num < 5 ) || ( val_num > 12 ) ) )
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - out of range, configuration failed",
                                  FORM_FIELD_NAME_CHAN_DR_2 );
                        err = ESP_FAIL;
                    }
                    else
                    {
                        updated_config.chan_datarate_2 = ( uint32_t ) val_num;
                    }
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_CHAN_DR_2 );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }
#endif

            /* Get channel bandwidth */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_CHAN_BW );
            if( val != NULL )
            {
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONNumber )
                {
                    updated_config.chan_bandwidth_khz = ( uint16_t ) json_value_get_number( val );
                }
                else if( val_type == JSONString )
                {
                    updated_config.chan_bandwidth_khz = ( uint16_t ) strtoul( json_value_get_string( val ), NULL, 10 );
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed", FORM_FIELD_NAME_CHAN_BW,
                              val_type );
                    err = ESP_FAIL;
                }
                /* sanity check */
                if( err == ESP_OK )
                {
                    printf( "%s:%u\n", FORM_FIELD_NAME_CHAN_BW, updated_config.chan_bandwidth_khz );
#if defined( CONFIG_RADIO_TYPE_LR1121 )
                    if( ( updated_config.chan_bandwidth_khz != 125 ) && ( updated_config.chan_bandwidth_khz != 250 ) &&
                        ( updated_config.chan_bandwidth_khz != 500 ) && ( updated_config.chan_bandwidth_khz != 200 ) &&
                        ( updated_config.chan_bandwidth_khz != 400 ) && ( updated_config.chan_bandwidth_khz != 800 ) )
#else
                    if( ( updated_config.chan_bandwidth_khz != 125 ) && ( updated_config.chan_bandwidth_khz != 250 ) &&
                        ( updated_config.chan_bandwidth_khz != 500 ) )
#endif
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - out of range, configuration failed", FORM_FIELD_NAME_CHAN_BW );
                        err = ESP_FAIL;
                    }
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_CHAN_BW );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }

            /* Get LNS address */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_LNS_ADDRESS );
            if( val != NULL )
            {
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONString )
                {
                    str = json_value_get_string( val );
                    if( strlen( str ) < sizeof( updated_config.lns_address ) )
                    {
                        strcpy( updated_config.lns_address, str );
                        printf( "%s:%s\n", FORM_FIELD_NAME_LNS_ADDRESS, updated_config.lns_address );
                    }
                    else
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - too long", FORM_FIELD_NAME_LNS_ADDRESS );
                        err = ESP_FAIL;
                    }
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed",
                              FORM_FIELD_NAME_LNS_ADDRESS, val_type );
                    err = ESP_FAIL;
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_LNS_ADDRESS );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }

            /* Get LNS port */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_LNS_PORT );
            if( val != NULL )
            {
                double          val_num;
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONNumber )
                {
                    val_num = json_value_get_number( val );
                }
                else if( val_type == JSONString )
                {
                    val_num = atof( json_value_get_string( val ) );
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed", FORM_FIELD_NAME_LNS_PORT,
                              val_type );
                    err = ESP_FAIL;
                }
                /* sanity check */
                if( err == ESP_OK )
                {
                    printf( "%s:%.0f\n", FORM_FIELD_NAME_LNS_PORT, val_num );
                    if( ( val_num < 0 ) || ( val_num > 65535 ) )
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - out of range, configuration failed", FORM_FIELD_NAME_LNS_PORT );
                        err = ESP_FAIL;
                    }
                    else
                    {
                        updated_config.lns_port = ( uint16_t ) val_num;
                    }
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_LNS_PORT );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }

            /* Get SNTP server address */
            val = json_object_get_value( root_obj, FORM_FIELD_NAME_SNTP_ADDRESS );
            if( val != NULL )
            {
                JSON_Value_Type val_type = json_value_get_type( val );
                if( val_type == JSONString )
                {
                    str = json_value_get_string( val );
                    if( strlen( str ) < sizeof( updated_config.sntp_address ) )
                    {
                        strcpy( updated_config.sntp_address, str );
                        printf( "%s:%s\n", FORM_FIELD_NAME_SNTP_ADDRESS, updated_config.sntp_address );
                    }
                    else
                    {
                        ESP_LOGE( TAG_WEB, "ERROR: %s - too long", FORM_FIELD_NAME_SNTP_ADDRESS );
                        err = ESP_FAIL;
                    }
                }
                else
                {
                    ESP_LOGE( TAG_WEB, "ERROR: %s - invalid format %d, configuration failed",
                              FORM_FIELD_NAME_SNTP_ADDRESS, val_type );
                    err = ESP_FAIL;
                }
                /* response on error */
                if( err != ESP_OK )
                {
                    httpd_resp_send_err( req, HTTPD_400_BAD_REQUEST, FORM_FIELD_NAME_SNTP_ADDRESS );
                    json_value_free( root_val );
                    return ESP_FAIL;
                }
            }
        }
    }

    /* free the JSON parse tree from memory */
    json_value_free( root_val );

    /* Check if dual-sf configuration is valid */
    uint8_t bw;
    switch( updated_config.chan_bandwidth_khz )
    {
        /* sub-ghz bandwidths */
    case 125:
        bw = BW_125KHZ;
        break;
    case 250:
        bw = BW_250KHZ;
        break;
    case 500:
        bw = BW_500KHZ;
        break;
        /* 2.4 ghz bandwidths */
    case 200:
        bw = BW_200KHZ;
        break;
    case 400:
        bw = BW_400KHZ;
        break;
    case 800:
        bw = BW_800KHZ;
        break;
    default:
        bw = BW_UNDEFINED;
        break;
    }
    int lgw_err = lgw_check_lora_dualsf_conf( bw, updated_config.chan_datarate_1, updated_config.chan_datarate_2 );
    if( lgw_err != LGW_HAL_SUCCESS )
    {
        ESP_LOGE( TAG_WEB, "ERROR: wrong channel configuration" );
        httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "wrong channel configuration" );
        return ESP_FAIL;
    }

    /* store configuration to flash memory */
    lgw_nvs_set_config( &updated_config );
    if( lgw_nvs_save_config( ) != ESP_OK )
    {
        ESP_LOGE( TAG_WEB, "ERROR: failed to write configuration to NVS" );
        httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to store config to NVS" );
        return ESP_FAIL;
    }

    if( post_src == HTTP_POST_SRC_WEB_FORM )
    {
        /* Redirect back to root page */
        httpd_resp_set_status( req, "302 Found" );
        httpd_resp_set_hdr( req, "Location", "/" );
        httpd_resp_send( req, NULL, 0 );
    }
    else
    {  // HTTP_POST_SRC_API
        httpd_resp_sendstr( req, "set config request ok" );
    }

    return ESP_OK;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* POSTMAN:
POST http://xxx.xxx.xxx.xxxx:8000/api/v1/reboot
{}
*/

static esp_err_t reboot_post_handler( httpd_req_t* req )
{
    http_post_src_t post_src = ( http_post_src_t )( intptr_t ) req->user_ctx;

    ESP_LOGI( TAG_WEB, "%s: req->uri=%s", __FUNCTION__, req->uri );
    ESP_LOGI( TAG_WEB, "%s: content length %d", __FUNCTION__, req->content_len );

    if( post_src == HTTP_POST_SRC_WEB_FORM )
    {
        ESP_LOGI( TAG_WEB, "%s: received POST reboot from web form", __FUNCTION__ );
        /* Redirect back to root page */
        httpd_resp_set_status( req, "302 Found" );
        httpd_resp_set_hdr( req, "Location", "/" );
        httpd_resp_send( req, NULL, 0 );
    }
    else
    {  // HTTP_POST_SRC_API
        ESP_LOGI( TAG_WEB, "%s: received POST reboot from API", __FUNCTION__ );
        httpd_resp_sendstr( req, "reboot request ok" );
    }

    vTaskDelay( 2000 / portTICK_PERIOD_MS );
    esp_restart( );

    return ESP_OK;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* POSTMAN:
GET http://xxx.xxx.xxx.xxxx:8000/api/v1/get_config
*/

esp_err_t get_config_get_handler( httpd_req_t* req )
{
    ESP_LOGI( TAG_WEB, "%s: req->uri=%s", __FUNCTION__, req->uri );
    ESP_LOGI( TAG_WEB, "%s: content length %d", __FUNCTION__, req->content_len );

    /* Get current loaded config */
    const lgw_nvs_cfg_t* nvs_cfg;
    lgw_nvs_get_config( &nvs_cfg );

    /* Generate the JSON string */
    snprintf( post_content_json, JSON_FULL_CONTENT_MAX_SIZE,
              "{\"%s\":\"%s\",\"%s\":%u,\"%s\":%.6f,\"%s\":%u,\"%s\":%u,\"%s\":%u,"
              "\"%s\":\"%s\"}",
              CFG_NVS_KEY_LNS_ADDRESS, nvs_cfg->lns_address, CFG_NVS_KEY_LNS_PORT, nvs_cfg->lns_port,
              CFG_NVS_KEY_CHAN_FREQ, ( double ) nvs_cfg->chan_freq_hz / 1e6, CFG_NVS_KEY_CHAN_DR_1,
              nvs_cfg->chan_datarate_1, CFG_NVS_KEY_CHAN_DR_2, nvs_cfg->chan_datarate_2, CFG_NVS_KEY_CHAN_BW,
              nvs_cfg->chan_bandwidth_khz, CFG_NVS_KEY_SNTP_ADDRESS, nvs_cfg->sntp_address );

    /* Send response */
    httpd_resp_set_type( req, "application/json" );
    httpd_resp_sendstr( req, post_content_json );

    return ESP_OK;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* POSTMAN:
GET http://xxx.xxx.xxx.xxxx:8000/api/v1/get_info
*/

esp_err_t get_info_get_handler( httpd_req_t* req )
{
    ESP_LOGI( TAG_WEB, "%s: req->uri=%s", __FUNCTION__, req->uri );
    ESP_LOGI( TAG_WEB, "%s: content length %d", __FUNCTION__, req->content_len );

    /* Generate the JSON string */
    snprintf( post_content_json, JSON_FULL_CONTENT_MAX_SIZE, "{\"fw_version\":\"%s\",\"radio_type\":\"%s\"}",
              LORAHUB_FW_VERSION_STR, radio_type );

    /* Send response */
    httpd_resp_set_type( req, "application/json" );
    httpd_resp_sendstr( req, post_content_json );

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

void http_server_init( void )
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG( );
    config.server_port    = 8000;  // TODO: make it configurable

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI( TAG_WEB, "Starting HTTP Server on port: '%d'", config.server_port );
    if( httpd_start( &server, &config ) != ESP_OK )
    {
        ESP_LOGE( TAG_WEB, "ERROR: Failed to start file server!" );
        return;
    }

    /* URI handler for GET */
    httpd_uri_t _root_get_handler = {
        .uri = "/", .method = HTTP_GET, .handler = http_root_get_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler( server, &_root_get_handler );

    /* URI handler for submit configuration POST from web form */
    httpd_uri_t submit_post_handler = { .uri      = "/submit",
                                        .method   = HTTP_POST,
                                        .handler  = set_config_post_handler,
                                        .user_ctx = ( void* ) HTTP_POST_SRC_WEB_FORM };
    httpd_register_uri_handler( server, &submit_post_handler );

    /* URI handler for reboot POST from web form */
    httpd_uri_t reboot_post_handler_uri = { .uri      = "/reboot",
                                            .method   = HTTP_POST,
                                            .handler  = reboot_post_handler,
                                            .user_ctx = ( void* ) HTTP_POST_SRC_WEB_FORM };
    httpd_register_uri_handler( server, &reboot_post_handler_uri );

    /* URI handler for submit configuration POST from API */
    httpd_uri_t api_set_config_post_uri = { .uri      = "/api/v1/set_config",
                                            .method   = HTTP_POST,
                                            .handler  = set_config_post_handler,
                                            .user_ctx = ( void* ) HTTP_POST_SRC_API };
    httpd_register_uri_handler( server, &api_set_config_post_uri );

    /* URI handler for reboot POST from API */
    httpd_uri_t api_reboot_post_uri = { .uri      = "/api/v1/reboot",
                                        .method   = HTTP_POST,
                                        .handler  = reboot_post_handler,
                                        .user_ctx = ( void* ) HTTP_POST_SRC_API };
    httpd_register_uri_handler( server, &api_reboot_post_uri );

    /* URI handler got get_config GET from API */
    httpd_uri_t api_get_config_get_uri = {
        .uri = "/api/v1/get_config", .method = HTTP_GET, .handler = get_config_get_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler( server, &api_get_config_get_uri );

    /* URI handler got get_info GET from API */
    httpd_uri_t api_get_info_get_uri = {
        .uri = "/api/v1/get_info", .method = HTTP_GET, .handler = get_info_get_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler( server, &api_get_info_get_uri );
}