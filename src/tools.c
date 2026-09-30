#include "config.h"
#include "tools.h"

led_strip_handle_t led;

static void change_led_colour(led_strip_handle_t led, uint8_t r, uint8_t g, uint8_t b, uint32_t delay) {
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

void post_program_state(program_state_t state) {
    xQueueSend(program_state_queue, &state, 0);
    if (state == DONE)
        xTimerReset(program_state_timer, 0);
    else
        xTimerStop(program_state_timer, 0);
}

void program_state_callback(TimerHandle_t xTimer) {
    program_state_t state = IDLE;
    xQueueSend(program_state_queue, &state, 0);
}

void led_blink_task(void* pvParameters) {
    led_strip_handle_t led = (led_strip_handle_t) pvParameters;
    program_state_t program_state = IDLE;

    while (1) {
        xQueueReceive(program_state_queue, &program_state, 0);
        
        switch (program_state) {
        case IDLE:
            change_led_colour(led, 0, 0, 20, 0);
            break;
        case DONE:
            change_led_colour(led, 0, 20, 0, 500);
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
