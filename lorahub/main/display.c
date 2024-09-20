/*______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
(C)2024 Semtech

Description:
    LoRaHub Packet Forwarder Display

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

/* -------------------------------------------------------------------------- */
/* --- DEPENDENCIES --------------------------------------------------------- */

#include <stdint.h>  /* C99 types */
#include <stdbool.h> /* bool type */

#include <esp_log.h>
#include <esp_timer.h>

#include <esp_netif.h>

#include <driver/i2c.h>
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#include "display.h"
#include "wifi.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* I2C/OLED display */
#define I2C_HOST 0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ ( 400 * 1000 )
#define EXAMPLE_PIN_NUM_SDA 17
#define EXAMPLE_PIN_NUM_SCL 18
#define EXAMPLE_PIN_NUM_RST 21
#define EXAMPLE_I2C_HW_ADDR 0x3C
#define EXAMPLE_LCD_CMD_BITS 8  // Bit number used to represent command and parameter
#define EXAMPLE_LCD_H_RES 128
#define EXAMPLE_LCD_V_RES 64

#define DEBUG_REFRESH 0

#define LABEL_STR_MAX_SIZE 64

static const char* TAG_DISP = "display";

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static lv_disp_t* oled_disp                     = NULL;
static lv_obj_t*  label_line1                   = NULL; /* status */
static lv_obj_t*  label_line2                   = NULL; /* IP address, Gateway ID, ...*/
static lv_obj_t*  label_line3                   = NULL; /* Channel configuration */
static lv_obj_t*  label_line4                   = NULL; /* Uplink/Downlink stats */
static lv_obj_t*  label_line5                   = NULL; /* Last packet received info */
static char       label_str[LABEL_STR_MAX_SIZE] = { 0 };

/* flags to indicate if a line should be refreshed */
static bool flag_refresh_status         = true;
static bool flag_refresh_chan_cfg       = false;
static bool flag_refresh_statistics     = false;
static bool flag_refresh_connection     = false;
static bool flag_refresh_last_rx_packet = false;

/* Status to be sent to display */
static display_status_t disp_status = DISPLAY_STATUS_UNKNOWN;

/* Channel configuration to be sent to display */
display_channel_conf_t disp_chan_cfg = { .freq_hz  = CONFIG_CHANNEL_FREQ_HZ,
                                         .datarate = CONFIG_CHANNEL_LORA_DATARATE,
                                         .bw_khz   = CONFIG_CHANNEL_LORA_BANDWIDTH };

/* RX/TX statistics to be sent to display */
static display_stats_t disp_stats = { 0 };

/* Gateway/Hub ID to be sent to display */
static display_connection_info_t disp_connection = { 0 };

/* Last received packet information to be sent to display */
static display_last_rx_packet_t disp_last_rx_pkt = { .devaddr = 0, .rssi = 0, .snr = 0 };

/* Fatal error to be sent to display */
static display_error_t disp_error = { .err = LRHB_ERROR_NONE, .line = 0 };

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

void display_init( void )
{
    ESP_LOGI( TAG_DISP, "Initialize I2C bus for OLED display" );
    i2c_config_t i2c_conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num       = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK( i2c_param_config( I2C_HOST, &i2c_conf ) );
    ESP_ERROR_CHECK( i2c_driver_install( I2C_HOST, I2C_MODE_MASTER, 0, 0, 0 ) );

    ESP_LOGI( TAG_DISP, "Install panel IO" );
    esp_lcd_panel_io_handle_t     io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr            = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,                     // According to SSD1306 datasheet
        .lcd_cmd_bits        = EXAMPLE_LCD_CMD_BITS,  // According to SSD1306 datasheet
        .lcd_param_bits      = EXAMPLE_LCD_CMD_BITS,  // According to SSD1306 datasheet
        .dc_bit_offset       = 6,                     // According to SSD1306 datasheet
    };
    ESP_ERROR_CHECK( esp_lcd_new_panel_io_i2c( ( esp_lcd_i2c_bus_handle_t ) I2C_HOST, &io_config, &io_handle ) );

    ESP_LOGI( TAG_DISP, "Install SSD1306 panel driver" );
    esp_lcd_panel_handle_t     panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
    ESP_ERROR_CHECK( esp_lcd_new_panel_ssd1306( io_handle, &panel_config, &panel_handle ) );
    ESP_ERROR_CHECK( esp_lcd_panel_reset( panel_handle ) );
    ESP_ERROR_CHECK( esp_lcd_panel_init( panel_handle ) );
    ESP_ERROR_CHECK( esp_lcd_panel_disp_on_off( panel_handle, true ) );

    ESP_LOGI( TAG_DISP, "Initialize LVGL" );
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG( );
    lvgl_port_init( &lvgl_cfg );
    const lvgl_port_display_cfg_t disp_cfg = { .io_handle     = io_handle,
                                               .panel_handle  = panel_handle,
                                               .buffer_size   = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
                                               .double_buffer = true,
                                               .hres          = EXAMPLE_LCD_H_RES,
                                               .vres          = EXAMPLE_LCD_V_RES,
                                               .monochrome    = true,
                                               .rotation      = {
                                                   .swap_xy  = false,
                                                   .mirror_x = false,
                                                   .mirror_y = false,
                                               } };
    oled_disp                              = lvgl_port_add_disp( &disp_cfg );

    /* Rotation of the screen */
    lv_disp_set_rotation( oled_disp, LV_DISP_ROT_180 );

    /* LVGL Task lock */
    lvgl_port_lock( 0 );

    /* Create widgets */
    lv_coord_t offset_y    = 0;
    uint16_t   font_height = 12; /* TODO: get from CONFIG ? */
    lv_obj_t*  scr         = lv_disp_get_scr_act( oled_disp );
    if( scr != NULL )
    {
        label_line1 = lv_label_create( scr );
        if( label_line1 != NULL )
        {
            lv_label_set_text( label_line1, "" );
            lv_label_set_long_mode( label_line1, LV_LABEL_LONG_SCROLL_CIRCULAR );
            lv_obj_set_width( label_line1, oled_disp->driver->hor_res );
            lv_obj_align( label_line1, LV_ALIGN_TOP_MID, 0, offset_y );
        }
        else
        {
            ESP_LOGE( TAG_DISP, "ERROR: failed to create label1" );
        }
        offset_y += font_height;

        label_line2 = lv_label_create( scr );
        if( label_line2 != NULL )
        {
            lv_label_set_text( label_line2, "" );
            lv_label_set_long_mode( label_line2, LV_LABEL_LONG_SCROLL_CIRCULAR );
            lv_obj_set_width( label_line2, oled_disp->driver->hor_res );
            lv_obj_align( label_line2, LV_ALIGN_TOP_MID, 0, offset_y );
        }
        else
        {
            ESP_LOGE( TAG_DISP, "ERROR: failed to create label2" );
        }
        offset_y += font_height;

        label_line3 = lv_label_create( scr );
        if( label_line3 != NULL )
        {
            lv_label_set_text( label_line3, "" );
            lv_label_set_long_mode( label_line3, LV_LABEL_LONG_CLIP );
            lv_obj_set_width( label_line3, oled_disp->driver->hor_res );
            lv_obj_align( label_line3, LV_ALIGN_TOP_MID, 0, offset_y );
        }
        else
        {
            ESP_LOGE( TAG_DISP, "ERROR: failed to create label3" );
        }
        offset_y += font_height;

        label_line4 = lv_label_create( scr );
        if( label_line4 != NULL )
        {
            lv_label_set_text( label_line4, "" );
            lv_label_set_long_mode( label_line4, LV_LABEL_LONG_SCROLL_CIRCULAR );
            lv_obj_set_width( label_line4, oled_disp->driver->hor_res );
            lv_obj_align( label_line4, LV_ALIGN_TOP_MID, 0, offset_y );
        }
        else
        {
            ESP_LOGE( TAG_DISP, "ERROR: failed to create label4" );
        }
        offset_y += font_height;

        label_line5 = lv_label_create( scr );
        if( label_line5 != NULL )
        {
            lv_label_set_text( label_line5, "" );
            lv_label_set_long_mode( label_line5, LV_LABEL_LONG_CLIP );
            lv_obj_set_width( label_line5, oled_disp->driver->hor_res );
            lv_obj_align( label_line5, LV_ALIGN_TOP_MID, 0, offset_y );
        }
        else
        {
            ESP_LOGE( TAG_DISP, "ERROR: failed to create label5" );
        }
        offset_y += font_height;
    }
    else
    {
        ESP_LOGE( TAG_DISP, "ERROR: failed to get active screen" );
    }

    /* LVGL Task unlock */
    lvgl_port_unlock( );
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_refresh( void )
{
    esp_netif_ip_info_t ip_info;

#if DEBUG_REFRESH
    int64_t time_start = esp_timer_get_time( );
#endif

    /* LVGL Task lock */
    lvgl_port_lock( 0 );

    /* Get current IP address */
    if( wifi_get_status( ) == WIFI_STATUS_CONNECTED )
    {
        esp_netif_t* netif = NULL;
        netif              = esp_netif_get_handle_from_ifkey( "WIFI_STA_DEF" );
        if( netif != NULL )
        {
            esp_err_t err = esp_netif_get_ip_info( netif, &ip_info );
            if( err != ESP_OK )
            {
                ESP_LOGE( TAG_DISP, "ERROR: esp_netif_get_ip_info failed with %s\n", esp_err_to_name( err ) );
            }
        }
    }

    /* Line 1: status */
    if( label_line1 != NULL && flag_refresh_status == true )
    {
        switch( disp_status )
        {
        case DISPLAY_STATUS_UNKNOWN:
            snprintf( label_str, sizeof label_str, "LoRaHub - ..." );
            break;
        case DISPLAY_STATUS_INITIALIZING:
            snprintf( label_str, sizeof label_str, "LoRaHub - Initializing" );
            break;
        case DISPLAY_STATUS_WIFI_PROVISIONING:
            snprintf( label_str, sizeof label_str,
                      "LoRaHub - WiFi provisioning" );  // TODO: display LoRaHub identifier ? LRHB_xxxxxx
            break;
        case DISPLAY_STATUS_RECEIVING:
            snprintf( label_str, sizeof label_str, "LoRaHub - Receiving" );
            break;
        default:
            break;
        }
        lv_label_set_text( label_line1, label_str );
        flag_refresh_status = false;
    }

    /* Line 2: IP address, Gateway ID, ... */
    if( label_line2 != NULL )
    {
        if( wifi_get_status( ) == WIFI_STATUS_CONNECTED )
        {
            if( flag_refresh_connection == true )
            {
                snprintf( label_str, sizeof label_str, "%016llX - " IPSTR, disp_connection.gateway_id,
                          IP2STR( &ip_info.ip ) );
            }
            else
            {
                snprintf( label_str, sizeof label_str, IPSTR, IP2STR( &ip_info.ip ) );
            }
        }
        else
        {
            snprintf( label_str, sizeof label_str, "not connected" );
        }
        lv_label_set_text( label_line2, label_str );
        /* keep flag_refresh_connection to true for now */
    }

    /* Line 3: channel configuration */
    if( label_line3 != NULL && flag_refresh_chan_cfg == true )
    {
        snprintf( label_str, sizeof label_str, "%.4lf  SF%u %u", ( double ) ( disp_chan_cfg.freq_hz ) / 1e6,
                  ( uint8_t ) disp_chan_cfg.datarate, disp_chan_cfg.bw_khz );
        lv_label_set_text( label_line3, label_str );
        flag_refresh_chan_cfg = false;
    }

    /* Line 4: RX/TX packets stats */
    if( label_line4 != NULL && flag_refresh_statistics == true )
    {
        snprintf( label_str, sizeof label_str, "up %lu  dn %lu", disp_stats.nb_rx, disp_stats.nb_tx );
        lv_label_set_text( label_line4, label_str );
        flag_refresh_statistics = false;
    }

    /* Line 5: Last packet info or FATAL ERROR */
    if( label_line5 != NULL )
    {
        if( disp_error.err == LRHB_ERROR_NONE )
        {
            /* No error : display last packet info */
            if( flag_refresh_last_rx_packet == true )
            {
                snprintf( label_str, sizeof label_str, "%08lX %d %d", disp_last_rx_pkt.devaddr,
                          ( int16_t ) disp_last_rx_pkt.rssi, ( int8_t ) disp_last_rx_pkt.snr );
                lv_label_set_text( label_line5, label_str );
                flag_refresh_last_rx_packet = false;
            }
        }
        else
        {
            /* display fatal error */
            snprintf( label_str, sizeof label_str, "ERROR(%d) - %d", disp_error.err, disp_error.line );
            lv_label_set_text( label_line5, label_str );
        }
    }

    /* LVGL Task unlock */
    lvgl_port_unlock( );

#if DEBUG_REFRESH
    int64_t time_stop = esp_timer_get_time( );
    ESP_LOGE( TAG_DISP, "display_refresh %lldus\n", time_stop - time_start );
#endif
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_status( display_status_t status )
{
    disp_status = status;

    flag_refresh_status = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_statistics( const display_stats_t* stats )
{
    disp_stats.nb_rx += stats->nb_rx;
    disp_stats.nb_tx += stats->nb_tx;

    flag_refresh_statistics = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_last_rx_packet( const display_last_rx_packet_t* last_pkt )
{
    disp_last_rx_pkt.devaddr = last_pkt->devaddr;
    disp_last_rx_pkt.rssi    = last_pkt->rssi;
    disp_last_rx_pkt.snr     = last_pkt->snr;

    flag_refresh_last_rx_packet = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_connection_info( const display_connection_info_t* info )
{
    disp_connection.gateway_id = info->gateway_id;

    flag_refresh_connection = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_channel_config( const display_channel_conf_t* chan_cfg )
{
    disp_chan_cfg.freq_hz  = chan_cfg->freq_hz;
    disp_chan_cfg.datarate = chan_cfg->datarate;
    disp_chan_cfg.bw_khz   = chan_cfg->bw_khz;

    flag_refresh_chan_cfg = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update_error( const display_error_t* error )
{
    disp_error.err  = error->err;
    disp_error.line = error->line;
}