#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>

#include "appfs.h"
#include "appfs_wrapper.h"
#include "bootscreen.h"
#include "filesystems.h"
#include "graphics_wrapper.h"
#include "hardware.h"
#include "menu.h"
#include "nametag.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "system_wrapper.h"
#include "wifi.h"
#include "wifi_connect.h"
#include "wifi_ota.h"

extern const uint8_t settings_png_start[] asm("_binary_settings_png_start");
extern const uint8_t settings_png_end[] asm("_binary_settings_png_end");

typedef enum action {
    ACTION_NONE,
    ACTION_BACK,
    ACTION_WIFI,
    ACTION_OTA,
    ACTION_OTA_NIGHTLY,
    ACTION_NICKNAME,
    ACTION_FORMAT_FAT,
    ACTION_FORMAT_APPFS
} menu_settings_action_t;

void render_settings_help(pax_buf_t* pax_buffer) {
    const pax_font_t* font = pax_font_saira_regular;
    pax_background(pax_buffer, 0xFFFFFF);
    pax_noclip(pax_buffer);
    pax_draw_text(pax_buffer, 0xFF000000, font, 18, 5, 240 - 18, "🅰 accept  🅱 back");
}

void menu_settings(xQueueHandle button_queue) {
    pax_buf_t* pax_buffer = get_pax_buffer();
    menu_t*    menu       = menu_alloc("Settings", 34, 18);

    menu->fgColor           = 0xFF000000;
    menu->bgColor           = 0xFFFFFFFF;
    menu->bgTextColor       = 0xFFFFFFFF;
    menu->selectedItemColor = 0xFF491d88;
    menu->borderColor       = 0xFF43b5a0;
    menu->titleColor        = 0xFF491d88;
    menu->titleBgColor      = 0xFF43b5a0;
    menu->scrollbarBgColor  = 0xFFCCCCCC;
    menu->scrollbarFgColor  = 0xFF555555;

    pax_buf_t icon_settings;
    pax_decode_png_buf(&icon_settings, (void*) settings_png_start, settings_png_end - settings_png_start, PAX_BUF_32_8888ARGB, 0);

    menu_set_icon(menu, &icon_settings);

    menu_insert_item(menu, "Edit nickname", NULL, (void*) ACTION_NICKNAME, -1);
    menu_insert_item(menu, "WiFi settings", NULL, (void*) ACTION_WIFI, -1);
    menu_insert_item(menu, "Firmware update", NULL, (void*) ACTION_OTA, -1);
    menu_insert_item(menu, "Install experimental firmware", NULL, (void*) ACTION_OTA_NIGHTLY, -1);
    menu_insert_item(menu, "Format internal FAT filesystem", NULL, (void*) ACTION_FORMAT_FAT, -1);
    menu_insert_item(menu, "Format internal AppFS filesystem", NULL, (void*) ACTION_FORMAT_APPFS, -1);

    bool                   render = true;
    menu_settings_action_t action = ACTION_NONE;

    render_settings_help(pax_buffer);

    while (1) {
        input_message_t button_message = {0};
        if (xQueueReceive(get_input_queue(), &button_message, 16 / portTICK_PERIOD_MS) == pdTRUE) {
            if (button_message.state) {
                switch (button_message.input) {
                    case INPUT_TOUCH0:
                        action = ACTION_BACK;
                        break;
                    case INPUT_TOUCH1:
                        menu_navigate_next(menu);
                        render = true;
                        break;
                    case INPUT_TOUCH2:
                        action = (menu_settings_action_t) menu_get_callback_args(menu, menu_get_position(menu));
                        break;
                    default:
                        break;
                }
            }
        }

        if (render) {
            menu_render(pax_buffer, menu, 0, 0, pax_buffer->width, 220);
            display_flush();
            render = false;
        }

        if (action != ACTION_NONE) {
            if (action == ACTION_OTA) {
                ota_update(false);
            } else if (action == ACTION_OTA_NIGHTLY) {
                ota_update(true);
            } else if (action == ACTION_WIFI) {
                menu_wifi(button_queue);
            } else if (action == ACTION_BACK) {
                break;
            } else if (action == ACTION_NICKNAME) {
                edit_nickname(button_queue);
            } else if (action == ACTION_FORMAT_FAT) {
                display_boot_screen("Formatting FAT...");
                format_internal_filesystem();
            } else if (action == ACTION_FORMAT_APPFS) {
                display_boot_screen("Formatting AppFS...");
                appfsFormat();
            }

            render = true;
            action = ACTION_NONE;
            render_settings_help(pax_buffer);
        }
    }

    menu_free(menu);
    pax_buf_destroy(&icon_settings);
}
