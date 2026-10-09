#pragma once

#include <driver/gpio.h>
#include <driver/pulse_cnt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _app_counter {
    int lo;
    int hi;
    int v;
} app_counter_t;

typedef struct _app_rotary_enc {
    pcnt_unit_handle_t unit_h;
    pcnt_channel_handle_t chan_a_h, chan_b_h;
    app_counter_t* p_counter;
} app_rotary_enc_t;

typedef struct _app_rotary_enc_cfg {
    gpio_num_t chan_a;
    gpio_num_t chan_b;
    app_counter_t cnt;
} app_rotary_enc_cfg;

app_rotary_enc_t setup_rotary_enc(QueueHandle_t queue, const app_rotary_enc_cfg conf);
void rotary_enc_switch_mode(const app_rotary_enc_t* e, bool enable_x4);
int rotary_enc_set_val(app_rotary_enc_t* e, int val);

#ifdef __cplusplus
}
#endif
