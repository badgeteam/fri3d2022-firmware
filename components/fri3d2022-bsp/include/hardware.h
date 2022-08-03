#pragma once

#include <driver/spi_master.h>
#include <esp_err.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "st7789v.h"
#include "pax_gfx.h"
#include "fri3d_badge.h"

typedef struct _input_message {
    uint8_t input;
    bool    state;
} input_message_t;

/** \brief Initialize basic board support
 *
 * \details This function installs the GPIO ISR (interrupt service routine) service
 *          which allows for per-pin GPIO interrupt handlers. Then it initializes the
 *          I2C and SPI communication busses and the LCD display driver. Returns ESP_OK
 *          on success and a
 *
 * \retval ESP_OK   The function succesfully executed
 * \retval ESP_FAIL The function failed, possibly indicating hardware failure
 *
 * Check the esp_err header file from the ESP-IDF for a complete list of error codes
 * returned by SDK functions.
 */

esp_err_t bsp_init();

ST7789V* get_st7789v();

esp_err_t display_flush();

pax_buf_t* get_pax_buffer();

xQueueHandle get_input_queue();
