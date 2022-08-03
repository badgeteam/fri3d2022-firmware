#include "hardware.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c.h>
#include <esp_log.h>

#include "managed_i2c.h"

static const char* TAG = "hardware";

static bool bsp_ready    = false;

xSemaphoreHandle i2c_semaphore = NULL;

ST7789V dev_st7789v = {0};

static pax_buf_t pax_buffer;

static xQueueHandle input_queue;

static esp_err_t _bus_init() {
    esp_err_t res;

    // I2C bus
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_io_num = GPIO_I2C_SCL,
        .master.clk_speed = I2C_SPEED,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = 0
    };

    res = i2c_param_config(I2C_BUS, &i2c_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
        return res;
    }

    res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
        return res;
    }

    res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing system I2C bus failed");
        return res;
    }

    i2c_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive( i2c_semaphore );

    // SPI bus
    spi_bus_config_t busConfiguration = {0};
    busConfiguration.mosi_io_num      = GPIO_SPI_MOSI;
    busConfiguration.miso_io_num      = GPIO_SPI_MISO;
    busConfiguration.sclk_io_num      = GPIO_SPI_CLK;
    busConfiguration.quadwp_io_num    = -1;
    busConfiguration.quadhd_io_num    = -1;
    busConfiguration.max_transfer_sz  = SPI_MAX_TRANSFER_SIZE;
    res                               = spi_bus_initialize(SPI_BUS, &busConfiguration, SPI_DMA_CHANNEL);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing SPI bus failed");
        return res;
    }
    
    input_queue = xQueueCreate(8, sizeof(input_message_t));

    return ESP_OK;
}

esp_err_t bsp_init() {
    if (bsp_ready) return ESP_OK;

    esp_err_t res;

    // Interrupts
    res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Installing ISR service failed");
        return res;
    }

    // Communication busses
    res = _bus_init();
    if (res != ESP_OK) return res;

    // LCD display
    dev_st7789v.spi_bus               = SPI_BUS;
    dev_st7789v.pin_cs                = GPIO_SPI_CS_LCD;
    dev_st7789v.pin_dcx               = GPIO_SPI_DC_LCD;
    dev_st7789v.pin_reset             = -1;
    dev_st7789v.spi_speed             = 40000000;  // 40MHz
    dev_st7789v.spi_max_transfer_size = SPI_MAX_TRANSFER_SIZE;
    dev_st7789v.width                 = 240;
    dev_st7789v.height                = 240;

    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1 << 12;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    
    res = gpio_set_direction(GPIO_LCD_BACKLIGHT, GPIO_MODE_OUTPUT);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD backlight GPIO failed");
        return res;
    }

    res = st7789v_init(&dev_st7789v);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD failed");
        return res;
    }
    
    pax_buf_init(&pax_buffer, NULL, 240, 240, PAX_BUF_16_565RGB);

    res = gpio_set_level(GPIO_LCD_BACKLIGHT, true);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Setting LCD backlight GPIO failed");
        return res;
    }

    bsp_ready = true;
    
    ESP_LOGW(TAG, "--- BSP init done ---");
    return ESP_OK;
}


ST7789V* get_st7789v() {
    if (!bsp_ready) return NULL;
    return &dev_st7789v;
}

esp_err_t display_flush() {
    if (!bsp_ready) return ESP_FAIL;
    return st7789v_write(&dev_st7789v, pax_buffer.buf);
}

pax_buf_t* get_pax_buffer() {
    if (!bsp_ready) return NULL;
    return &pax_buffer;
}

xQueueHandle get_input_queue() {
    return input_queue;
}
