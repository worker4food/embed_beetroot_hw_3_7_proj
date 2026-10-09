#pragma once

#include <iot_button.h>

#ifdef __cplusplus
extern "C" {
#endif

button_handle_t setup_button(QueueHandle_t q, gpio_num_t pin);

#ifdef __cplusplus
}
#endif