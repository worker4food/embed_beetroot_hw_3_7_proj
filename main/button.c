
#include <driver/gpio.h>
#include <iot_button.h>
#include <button_gpio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include "app_event.h"

#define SEND_TIMEOUT pdMS_TO_TICKS(500)

void post_msg(QueueHandle_t q, const app_event_t e) {
    xQueueSend(q, &e, SEND_TIMEOUT);
}

void on_click(void *button_handle, void *usr_data) {
    post_msg(usr_data, (app_event_t) {.name = BTN_CLICKED, .data = (int) button_handle});
}

void on_hold(void *button_handle, void *usr_data) {
    post_msg(usr_data, (app_event_t) {.name = BTN_HOLD_STARTED, .data = (int) button_handle});
}

button_handle_t setup_button(QueueHandle_t q, gpio_num_t pin) {
    button_handle_t btn_h;

    const button_gpio_config_t gpio_cfg = {
        .gpio_num = pin,
        .active_level = 0,
        .disable_pull = true
    };
    const button_config_t cfg = {0};
    ESP_ERROR_CHECK(
        iot_button_new_gpio_device(&cfg, &gpio_cfg, &btn_h)
    );
    ESP_ERROR_CHECK(
        iot_button_register_cb(btn_h, BUTTON_SINGLE_CLICK, NULL, on_click, q)
    );
    ESP_ERROR_CHECK(
        iot_button_register_cb(btn_h, BUTTON_LONG_PRESS_START, NULL, on_hold, q)
    );

    return btn_h;
}
