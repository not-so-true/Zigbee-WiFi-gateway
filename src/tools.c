#include "config.h"
#include "tools.h"

void change_led_colour(led_strip_handle_t led, uint8_t r, uint8_t g, uint8_t b, uint32_t delay) {
    if (delay == 0) {
        led_strip_set_pixel(led, 0, r, g, b);
        led_strip_refresh(led);
    }
    else {
        led_strip_set_pixel(led, 0, r, g, b);
        led_strip_refresh(led);
        vTaskDelay(pdMS_TO_TICKS(delay));

        led_strip_clear(led);
        led_strip_refresh(led);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void blink_led_task(void* pvParameters) {
    led_strip_handle_t led = (led_strip_handle_t) pvParameters;
    program_state_t program_state;

    while (1) {
        xQueueReceive(program_state_queue, &program_state, 0);
        
        switch (program_state) {
        case IDLE:
            change_led_colour(led, 0, 0, 20, 0);
            break;
        case DONE:
            change_led_colour(led, 0, 20, 0, 800);
            break;
        case PROCESSING:
            change_led_colour(led, 20, 10, 0, 100);
            break;
        case ERROR:
            change_led_colour(led, 20, 0, 0, 500);
            break;
        }
    }
}