#include "config.h"
#include "tools.h"

led_strip_handle_t led;

static void led_change_colour(led_strip_handle_t led, uint8_t r, uint8_t g, uint8_t b, uint32_t delay) {
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

void led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO_PIN,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 0 //default value
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led));

    program_state_queue = xQueueCreate(3, sizeof(program_state_t));
    program_state_timer = xTimerCreate(
        "program_state_timer",
        pdMS_TO_TICKS(5000),
        pdFALSE,
        NULL,
        program_state_callback
    );

    ESP_LOGI(TAG_LED, "Initialized successfully");
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
    BaseType_t check = pdFALSE;
    TickType_t timeout = 0;

    while (1) {
        switch (program_state) {
        case IDLE:
            led_change_colour(led, 0, 0, 20, 0);
            timeout = portMAX_DELAY;
            break;
        case DONE:
            led_change_colour(led, 0, 20, 0, 500);
            break;
        case PROCESSING:
            led_change_colour(led, 20, 10, 0, 100);
            break;
        case ERROR:
            led_change_colour(led, 20, 0, 0, 500);
            break;
        }

        check = xQueueReceive(program_state_queue, &program_state, timeout);
        if (check == pdTRUE) timeout = 0;
    }
}
