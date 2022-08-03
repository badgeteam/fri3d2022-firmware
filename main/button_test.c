#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <stdio.h>

#include "hardware.h"
#include "pax_gfx.h"

void test_buttons() {
    pax_buf_t*        pax_buffer = get_pax_buffer();
    const pax_font_t* font       = pax_font_saira_regular;

    bool render = true;
    bool quit   = false;

    bool btn_touch0 = false;
    bool btn_touch1 = false;
    bool btn_touch2 = false;

    bool btn_touch0_green = false;
    bool btn_touch1_green = false;
    bool btn_touch2_green = false;

    while (!quit) {
        input_message_t button_message = {0};
        if (xQueueReceive(get_input_queue(), &button_message, 16 / portTICK_PERIOD_MS) == pdTRUE) {
            bool value = button_message.state;
            render = true;
            switch (button_message.input) {
                case INPUT_TOUCH0:
                    btn_touch0 = value;
                    if (value) btn_touch0_green = true;
                    break;
                case INPUT_TOUCH1:
                    btn_touch1 = value;
                    if (value) btn_touch1_green = true;
                    break;
                case INPUT_TOUCH2:
                    btn_touch2 = value;
                    if (value) btn_touch2_green = true;
                    break;
                default:
                    break;
            }
        }

        if (render) {
            pax_noclip(pax_buffer);
            pax_background(pax_buffer, 0x325aa8);
            pax_draw_text(pax_buffer, 0xFFFFFFFF, font, 18, 0, 20 * 0, "Press 0 + 2 to exit");
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "TOUCH 0 %s", btn_touch0 ? "PRESSED" : "released");
            pax_draw_text(pax_buffer, btn_touch0_green ? 0xFF00FF00 : 0xFFFFFFFF, font, 18, 0, 20 * 1, buffer);
            snprintf(buffer, sizeof(buffer), "TOUCH 1 %s", btn_touch1 ? "PRESSED" : "released");
            pax_draw_text(pax_buffer, btn_touch1_green ? 0xFF00FF00 : 0xFFFFFFFF, font, 18, 0, 20 * 2, buffer);
            snprintf(buffer, sizeof(buffer), "TOUCH 2 %s", btn_touch2 ? "PRESSED" : "released");
            pax_draw_text(pax_buffer, btn_touch2_green ? 0xFF00FF00 : 0xFFFFFFFF, font, 18, 0, 20 * 3, buffer);
            display_flush();
            render = false;
        }

        if (btn_touch0 && btn_touch1) {
            quit = true;
        }
    }
}
