#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

#include "hardware.h"

static const char *TAG = "Touch pad";

#define TOUCH_PAD_AMOUNT (3)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

static uint32_t touch_pads[TOUCH_PAD_AMOUNT] = {7, 6, 4};
static bool s_pad_activated[TOUCH_PAD_AMOUNT];
static uint8_t touch_pad_mapping[TOUCH_PAD_AMOUNT] = {INPUT_TOUCH0, INPUT_TOUCH1, INPUT_TOUCH2};

static xQueueHandle queue = NULL;

static void set_thresholds(void) {
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_AMOUNT; i++) {
        touch_pad_read_filtered(touch_pads[i], &touch_value);
        ESP_LOGI(TAG, "touchpad init: touch pad [%d] val is %d", i, touch_value);
        ESP_ERROR_CHECK(touch_pad_set_thresh(touch_pads[i], touch_value * 2 / 3));
    }
}

static void rtc_intr(void *arg) {
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = 0; i < TOUCH_PAD_AMOUNT; i++) {
        if ((pad_intr >> touch_pads[i]) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}

/*
  Check if any of touch pads has been activated
  by reading a table updated by rtc_intr()
  If so, then print it out on a serial monitor.
  Clear related entry in the table afterwards
  In interrupt mode, the table is updated in touch ISR.
*/
static void tp_example_read_task(void *pvParameter)
{
    while (1) {
        for (int i = 0; i < TOUCH_PAD_AMOUNT; i++) {
            if (s_pad_activated[i] == true) {
                ESP_LOGI(TAG, "T%d activated!", i);

                input_message_t message;
                message.input = touch_pad_mapping[i];
                message.state = true;
                xQueueSend(queue, &message, 0);

                // Wait a while for the pad being released
                vTaskDelay(300 / portTICK_PERIOD_MS);
                // Clear information on pad activation
                s_pad_activated[i] = false;

            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void init_touch(xQueueHandle output_queue) {
    queue = output_queue;
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

    for (int i = 0; i < TOUCH_PAD_AMOUNT; i++) {
        touch_pad_config(touch_pads[i], TOUCH_THRESH_NO_USE);
    }

    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    set_thresholds();
    touch_pad_isr_register(rtc_intr, NULL);
    touch_pad_intr_enable();

    // Start a task to show what pads have been touched
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL);
}
