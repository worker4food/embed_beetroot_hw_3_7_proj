#pragma once

#include <driver/gpio.h>
#include <driver/ledc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _app_servo_cfg {
    gpio_num_t ctrl_pin;
    ledc_timer_t ledc_timer;
    ledc_channel_t ledc_chan;
} app_servo_cfg;

ledc_channel_t setup_servo(const app_servo_cfg cfg, int start_angle);
void set_servo_angle(ledc_channel_t chan, int angle);

#ifdef __cplusplus
}
#endif
