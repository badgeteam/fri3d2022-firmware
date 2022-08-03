/**
 * Copyright (c) 2022 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include <sdkconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/gpio_struct.h>
#include <soc/spi_reg.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "include/st7789v.h"

static const char *TAG = "st7789v";

static void st7789v_spi_pre_transfer_callback(spi_transaction_t *t) {
    ST7789V* device = ((ST7789V*) t->user);
    gpio_set_level(device->pin_dcx, device->dc_level);
}

static esp_err_t st7789v_send(ST7789V* device, const uint8_t *data, const int len, const bool dc_level) {
    if (len == 0) return ESP_OK;
    if (device->spi_device == NULL) return ESP_FAIL;
    device->dc_level = dc_level;
    spi_transaction_t transaction = {
        .length = len * 8,  // transaction length is in bits
        .tx_buffer = data,
        .user = (void*) device,
    };
    return spi_device_transmit(device->spi_device, &transaction);
}

static esp_err_t st7789v_write_init_data(ST7789V* device, const uint8_t * data) {
    if (device->spi_device == NULL) return ESP_FAIL;
    esp_err_t res = ESP_OK;
    uint8_t cmd, len;
    while(true) {
        cmd = *data++;
        if(!cmd) return ESP_OK; //END
        len = *data++;
        res = st7789v_send(device, &cmd, 1, false);
        if (res != ESP_OK) break;
        res = st7789v_send(device, data, len, true);
        if (res != ESP_OK) break;
        data+=len;
    }
    return res;
}

static esp_err_t st7789v_send_command(ST7789V* device, const uint8_t cmd) {
    return st7789v_send(device, &cmd, 1, false);
}

static esp_err_t st7789v_send_data(ST7789V* device, const uint8_t* data, const uint16_t length) {
    return st7789v_send(device, data, length, true);
}

static esp_err_t st7789v_send_u32(ST7789V* device, const uint32_t data) {
    uint8_t buffer[4];
    buffer[0] = (data>>24)&0xFF;
    buffer[1] = (data>>16)&0xFF;
    buffer[2] = (data>> 8)&0xFF;
    buffer[3] = data      &0xFF;
    return st7789v_send(device, buffer, 4, true);
}

esp_err_t st7789v_set_addr_window(ST7789V* device, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
    uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
    esp_err_t res;
    res = st7789v_send_command(device, ST7789V_CASET);
    if (res != ESP_OK) return res;
    res = st7789v_send_u32(device, xa);
    if (res != ESP_OK) return res;
    res = st7789v_send_command(device, ST7789V_RASET);
    if (res != ESP_OK) return res;
    res = st7789v_send_u32(device, ya);
    if (res != ESP_OK) return res;
    res = st7789v_send_command(device, ST7789V_RAMWR);
    return res;
    return ESP_OK;
}

esp_err_t st7789v_reset(ST7789V* device) {
    if (device->mutex != NULL) xSemaphoreTake(device->mutex, portMAX_DELAY);
    if (device->pin_reset >= 0) {
        if (device->reset_open_drain) {
            esp_err_t res = gpio_set_level(device->pin_reset, false);
            if (res != ESP_OK) return res;
            res = gpio_set_direction(device->pin_reset, GPIO_MODE_OUTPUT);
            if (res != ESP_OK) return res;
            vTaskDelay(50 / portTICK_PERIOD_MS);
            res = gpio_set_direction(device->pin_reset, GPIO_MODE_INPUT);
            if (res != ESP_OK) return res;
            vTaskDelay(50 / portTICK_PERIOD_MS);
        } else {
            esp_err_t res = gpio_set_level(device->pin_reset, false);
            if (res != ESP_OK) return res;
            vTaskDelay(50 / portTICK_PERIOD_MS);
            res = gpio_set_level(device->pin_reset, true);
            if (res != ESP_OK) return res;
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    } else {
        ESP_LOGI(TAG, "(no reset pin available)");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (device->mutex != NULL) xSemaphoreGive(device->mutex);
    return ESP_OK;
}

esp_err_t st7789v_set_sleep(ST7789V* device, bool state) {
    esp_err_t res;
    if (device->mutex != NULL) xSemaphoreTake(device->mutex, portMAX_DELAY);
    if (state) {
        res = st7789v_send_command(device, ST7789V_SLPIN);
        if (res != ESP_OK) return res;
    } else {
        res = st7789v_send_command(device, ST7789V_SLPOUT);
        if (res != ESP_OK) return res;
    }
    if (device->mutex != NULL) xSemaphoreGive(device->mutex);
    return res;
}

esp_err_t st7789v_init(ST7789V* device) {
    esp_err_t res;
    
    if (device->pin_dcx < 0) return ESP_FAIL;
    if (device->pin_cs < 0) return ESP_FAIL;

    //Allocate partial update buffer
    device->internal_buffer = heap_caps_malloc(device->spi_max_transfer_size, MALLOC_CAP_8BIT);
    if (!device->internal_buffer) return ESP_FAIL;

	//Initialize reset GPIO pin
	if (device->pin_reset >= 0) {
		res = gpio_set_direction(device->pin_reset, GPIO_MODE_OUTPUT);
		if (res != ESP_OK) return res;
	}

	//Initialize data/clock select GPIO pin
	res = gpio_set_direction(device->pin_dcx, GPIO_MODE_OUTPUT);
	if (res != ESP_OK) return res;
	
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = device->spi_speed,
		.mode           = 0,  // SPI mode 0
		.spics_io_num   = device->pin_cs,
		.queue_size     = 1,
		.flags          = (SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE),//SPI_DEVICE_HALFDUPLEX,
		.pre_cb         = st7789v_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
	};
	res = spi_bus_add_device(device->spi_bus, &devcfg, &device->spi_device);
	if (res != ESP_OK) return res;

	//Reset the LCD display
	res = st7789v_reset(device);
	if (res != ESP_OK) return res;
	
	//Wake-up display
	res = st7789v_set_sleep(device, false);
	if (res != ESP_OK) return res;
	
	ets_delay_us(100);
	
	if (device->mutex != NULL) xSemaphoreTake(device->mutex, portMAX_DELAY);
	res = st7789v_send_command(device, ST7789V_COLMOD);
	if (res != ESP_OK) return res;
	uint8_t value = 0x55;
	res = st7789v_send(device, &value, 1, true);
	if (res != ESP_OK) return res;
	res = st7789v_send_command(device, ST7789V_MADCTL);
	if (res != ESP_OK) return res;
	res = st7789v_send_u32(device, 0b00000000);
	if (res != ESP_OK) return res;
	res = st7789v_send_command(device, ST7789V_INVON);
	if (res != ESP_OK) return res;
	if (device->mutex != NULL) xSemaphoreGive(device->mutex);

	ets_delay_us(100);
	
	//
	res = st7789v_send_command(device, ST7789V_DISPON);
	if (res != ESP_OK) return res;
	
	ets_delay_us(100);
	
	//
	res = st7789v_send_command(device, ST7789V_NORON);
	if (res != ESP_OK) return res;

	return ESP_OK;
}

esp_err_t st7789v_write(ST7789V* device, const uint8_t *buffer) {
	return st7789v_write_partial(device, buffer, 0, 0, device->width-1, device->height-1);
}

esp_err_t st7789v_write_partial_direct(ST7789V* device, const uint8_t *buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) { //Without conversion
	if (x0 > x1) return ESP_FAIL;
	if (y0 > y1) return ESP_FAIL;
	uint16_t w = x1-x0;
	uint16_t h = y1-y0;
	esp_err_t res = st7789v_set_addr_window(device, x0, y0, w, h);
	if (res != ESP_OK) return res;
	res = st7789v_send(device, buffer, w*h*2, true);
	return res;
}

esp_err_t st7789v_write_partial(ST7789V* device, const uint8_t *frameBuffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) { //With conversion from framebuffer
	esp_err_t res = ESP_OK;
	if (x0 > x1) return ESP_FAIL;
	if (y0 > y1) return ESP_FAIL;
	if (x1 >= device->width)  x1 = device->width-1;
	if (y1 >= device->height) y1 = device->height-1;
	
	uint16_t w = x1-x0+1;
	uint16_t h = y1-y0+1;

	while (w > 0) {
		uint16_t transactionWidth = w;
		if (transactionWidth*2 > device->spi_max_transfer_size) {
			transactionWidth = device->spi_max_transfer_size/2;
		}
		res = st7789v_set_addr_window(device, x0+device->offset_x, y0+device->offset_y, transactionWidth, h);
		if (res != ESP_OK) return res;
		for (uint16_t currentLine = 0; currentLine < h; currentLine++) {
			for (uint16_t i = 0; i<transactionWidth; i++) {
				device->internal_buffer[i*2+0] = frameBuffer[((x0+i)+(y0+currentLine)*device->width)*2+0];
				device->internal_buffer[i*2+1] = frameBuffer[((x0+i)+(y0+currentLine)*device->width)*2+1];
			}
			res = st7789v_send(device, device->internal_buffer, transactionWidth*2, true);
			if (res != ESP_OK) return res;
		}
		w -= transactionWidth;
		x0 += transactionWidth;
	}
	return res;
}
