/*
 * FRI3D badge
 *
 * This file describes the connections from the ESP32 to the other
 * components on the FRI3D badge.
 *
 */

#pragma once

#define GPIO_BTN_BOOT      0
#define GPIO_UART_TX       1
#define GPIO_LED_DATA      2
#define GPIO_UART_RX       3
#define GPIO_BADGELINK     4
#define GPIO_SPI_CS_LCD    5
#define GPIO_LCD_BACKLIGHT 12
#define GPIO_SPI_CLK       18
#define GPIO_SPI_MISO      19
#define GPIO_I2C_SDA       21
#define GPIO_I2C_SCL       22
#define GPIO_SPI_MOSI      23
#define GPIO_BUZZER        32
#define GPIO_SPI_DC_LCD    33

// I2C bus
#define I2C_BUS        0
#define I2C_SPEED      400000  // 400 kHz
#define I2C_TIMEOUT    250 // us

// SPI bus
#define SPI_BUS               VSPI_HOST
#define SPI_MAX_TRANSFER_SIZE 4094
#define SPI_DMA_CHANNEL       2

// Buttons
#define INPUT_TOUCH0 0
#define INPUT_TOUCH1 1
#define INPUT_TOUCH2 2
