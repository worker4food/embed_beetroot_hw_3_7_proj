#include <stdio.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "app_event.h"
#include "encoder.h"
#include "servo.h"
#include "buzzer.h"
#include "button.h"

#define ENC_CLK_PIN GPIO_NUM_2
#define ENC_DT_PIN GPIO_NUM_1
#define ENC_BTN_PIN GPIO_NUM_42
#define BUZZER_CTRL_PIN GPIO_NUM_41
#define SERVO_CTRL_PIN GPIO_NUM_15

#define RECV_TIMEOUT pdMS_TO_TICKS(100)

void app_main(void)
{
    QueueHandle_t app_q = xQueueCreate(10, sizeof(app_event_t));

    const int start_angle = 90;
    const app_rotary_enc_cfg enc_conf = {
        .chan_a = ENC_CLK_PIN,
        .chan_b = ENC_DT_PIN,
        .cnt = {
            .lo = 0,
            .hi = 180,
            .v = start_angle
        }
    };
    app_rotary_enc_t enc = setup_rotary_enc(app_q, enc_conf);

    const app_servo_cfg servo_conf = {
        .ctrl_pin = SERVO_CTRL_PIN,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_chan = LEDC_CHANNEL_0,
    };
    const ledc_channel_t servo_chan = setup_servo(servo_conf, start_angle);

    const app_buzzer_cfg buzzer_conf = {
        .ctrl_pin = BUZZER_CTRL_PIN,
        .ledc_timer = LEDC_TIMER_1,
        .ledc_channel = LEDC_CHANNEL_1
    };
    app_buzzer_t player;
    setup_buzzer(buzzer_conf, &player);

    button_handle_t enc_bnt_h = setup_button(app_q, ENC_BTN_PIN);

    app_event_t ev;
    BaseType_t ret = pdFALSE;
    bool is_x4_speed = false;
    for(;;) {
        ret = xQueueReceive(app_q, &ev, RECV_TIMEOUT);
        if (ret == pdFALSE) {
            continue;
        }

        switch(ev.name) {
            case OVERFLOW:
                bool r = play_wrong(&player);
                if (!r) {
                    ESP_LOGW(__func__, "buzzer busy, buzzing boldly");
                }
                break;
            case SET_ANGLE:
                set_servo_angle(servo_chan, ev.data);
                break;
            case BTN_HOLD_STARTED:
                if (ev.data == (int) enc_bnt_h) {
                    int angle = rotary_enc_set_val(&enc, start_angle);
                    set_servo_angle(servo_chan, angle);
                }
                break;
            case BTN_CLICKED:
                if (ev.data == (int) enc_bnt_h) {
                    is_x4_speed = !is_x4_speed;
                    rotary_enc_switch_mode(&enc, is_x4_speed);
                }
                break;
            default:
                ESP_LOGW("unhandled app_event", "%d", ev.name);
        }
    }
}
