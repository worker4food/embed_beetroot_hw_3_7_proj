#include <stdio.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include "servo.h"

#define LEDC_DUTY_RESOLUTION 14
#define LEDC_FREQ_HZ 50
#define LEDC_MAX_PULSE_US (1'000'000.0 / LEDC_FREQ_HZ)
#define LEDC_MAX_DUTY ((1 << LEDC_DUTY_RESOLUTION) - 1)

#define SERVO_MIN_PULSE_US 500.0
#define SERVO_MAX_PULSE_US 2500.0
#define SERVO_MAX_DEGREE 180
#define SERVO_STEP_US ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)/SERVO_MAX_DEGREE)

uint32_t degree_to_ledc_duty(int angle) {
    if (angle < 0) {
        angle = 0;
    } else if (angle > SERVO_MAX_DEGREE) {
        angle = SERVO_MAX_DEGREE;
    }

    double pulse_us = SERVO_MIN_PULSE_US + angle * SERVO_STEP_US;
    double duty = (pulse_us / LEDC_MAX_PULSE_US) * LEDC_MAX_DUTY;
    return (uint32_t) duty;
}

ledc_channel_t setup_servo(const app_servo_cfg cfg, int start_angle) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = cfg.ledc_timer,
        .duty_resolution  = (ledc_timer_bit_t) LEDC_DUTY_RESOLUTION,
        .freq_hz          = LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(
        ledc_timer_config(&ledc_timer)
    );

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = cfg.ledc_chan,
        .timer_sel      = cfg.ledc_timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = cfg.ctrl_pin,
        .duty           = degree_to_ledc_duty(start_angle),
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(
        ledc_channel_config(&ledc_channel)
    );

    return cfg.ledc_chan;
}

void set_servo_angle(ledc_channel_t chan, int angle) {
    uint32_t duty_ticks = degree_to_ledc_duty(angle);

    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, chan, duty_ticks)
    );
    ESP_ERROR_CHECK(
        ledc_update_duty(LEDC_LOW_SPEED_MODE, chan)
    );
}