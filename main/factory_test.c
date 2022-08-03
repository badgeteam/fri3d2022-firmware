#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hardware.h"
#include "pax_gfx.h"
#include "settings.h"
#include "test_common.h"
#include "wifi_defaults.h"
#include "ws2812.h"

static const char* TAG = "factory";

/* Test routines */

bool run_basic_tests() {
    pax_buf_t*        pax_buffer = get_pax_buffer();
    const pax_font_t* font;
    int               line = 0;
    bool              ok   = true;

    /* Screen init */
    font = pax_font_sky_mono;

    pax_noclip(pax_buffer);
    pax_background(pax_buffer, 0x8060f0);
    display_flush();

    /* Run mandatory tests */
    // None yet

    /* Run tests */
    // None yet

//error:
    /* Fail result on screen */
    if (!ok) pax_draw_text(pax_buffer, 0xffff0000, font, 36, 0, 20 * line, "FAIL");
    display_flush();
    return ok;
}

const uint8_t led_green[15] = {50, 0, 0, 50, 0, 0, 50, 0, 0, 50, 0, 0, 50, 0, 0};
const uint8_t led_red[15]   = {0, 50, 0, 0, 50, 0, 0, 50, 0, 0, 50, 0, 0, 50, 0};
const uint8_t led_blue[15]  = {0, 0, 50, 0, 0, 50, 0, 0, 50, 0, 0, 50, 0, 0, 50};

void factory_test() {
    pax_buf_t* pax_buffer        = get_pax_buffer();
    uint8_t    factory_test_done = nvs_get_u8_default("system", "factory_test", 0);
    if (!factory_test_done) {
        bool result;

        ESP_LOGI(TAG, "Factory test start");

        result = run_basic_tests();

        ws2812_init(GPIO_LED_DATA);
        if (result) {
            ws2812_send_data(led_blue, sizeof(led_blue));
        } else {
            ws2812_send_data(led_red, sizeof(led_red));
        }

        if (!result) goto test_end;

    // Wait for the operator to unplug the badge
    test_end:

        if (result) {
            esp_err_t res = nvs_set_u8_fixed("system", "factory_test", 0x01);
            if (res != ESP_OK) {
                ESP_LOGE(TAG, "Failed to store test result %d\n", res);
                result = false;
                ws2812_send_data(led_red, sizeof(led_red));
                pax_noclip(pax_buffer);
                pax_background(pax_buffer, 0xa85a32);
                display_flush();
            }
            nvs_set_u8_fixed("system", "force_sponsors", 0x01);  // Force showing sponsors on first boot
            wifi_set_defaults();
            pax_noclip(pax_buffer);
            pax_background(pax_buffer, 0x00FF00);
            display_flush();
            ws2812_send_data(led_green, sizeof(led_green));
        }

        while (true) {
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}
