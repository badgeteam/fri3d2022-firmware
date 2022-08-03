#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

void init_touch(xQueueHandle output_queue);
