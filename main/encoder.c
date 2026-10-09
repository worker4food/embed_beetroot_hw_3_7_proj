
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>

#include "encoder.h"
#include "app_event.h"

#define GLITCH_FILTER_NS 12'000

#define USELESS_PCNT_LIMIT 100

typedef struct _cnt_ctx {
    QueueHandle_t q;
    app_counter_t cnt;
} cnt_ctx_t;

app_rotary_enc_t init_pcnt(gpio_num_t clk_pin, gpio_num_t dt_pin, cnt_ctx_t* ctx);

app_rotary_enc_t setup_rotary_enc(QueueHandle_t queue, const app_rotary_enc_cfg conf) {

    cnt_ctx_t* ctx = malloc(sizeof(cnt_ctx_t));
    ctx->cnt = conf.cnt;
    ctx->q = queue;

    app_rotary_enc_t e = init_pcnt(conf.chan_a, conf.chan_b, ctx);

    return e;
}

bool IRAM_ATTR pcnt_on_reach(pcnt_unit_handle_t unit_h, const pcnt_watch_event_data_t *e, void *arg) {

    cnt_ctx_t* ctx = arg;
    int old_v = ctx->cnt.v;
    bool overflow = false, changed = false;

    pcnt_unit_clear_count(unit_h);

    ctx->cnt.v += e->watch_point_value;
    if (ctx->cnt.v >= ctx->cnt.hi) {
        overflow = true;
        ctx->cnt.v = ctx->cnt.hi;
    } else if (ctx->cnt.v <= ctx->cnt.lo) {
        overflow = true;
        ctx->cnt.v = ctx->cnt.lo;
    }

    changed = old_v != ctx->cnt.v;

    BaseType_t smth_woken1 = pdFALSE;
    if (changed && overflow) {
        app_event_t ev = {.name = OVERFLOW};
        xQueueSendFromISR(ctx->q, &ev, &smth_woken1);
    }

    BaseType_t smth_woken2 = pdFALSE;
    if (changed) {
        app_event_t ev = {.name = SET_ANGLE, .data = ctx->cnt.v};
        xQueueSendFromISR(ctx->q, &ev, &smth_woken2);
    }
    
    return smth_woken1 || smth_woken2;
}

pcnt_channel_handle_t mk_pcnt_chan(pcnt_unit_handle_t unit_h, gpio_num_t edge_pin, gpio_num_t level_pin) {
    pcnt_channel_handle_t chan_h;

    const pcnt_chan_config_t conf = {
        .edge_gpio_num = edge_pin,
        .level_gpio_num = level_pin
    };
    ESP_ERROR_CHECK(
        pcnt_new_channel(unit_h, &conf, &chan_h)
    );

    return chan_h;
}

esp_err_t enable_pcnt_level_action(pcnt_channel_handle_t chan_h) {
    return pcnt_channel_set_level_action(
        chan_h,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    );
}

esp_err_t disable_pcnt_level_action(pcnt_channel_handle_t chan_h) {
    return pcnt_channel_set_level_action(
        chan_h,
        PCNT_CHANNEL_LEVEL_ACTION_HOLD,
        PCNT_CHANNEL_LEVEL_ACTION_HOLD
    );
}

app_rotary_enc_t init_pcnt(gpio_num_t clk_pin, gpio_num_t dt_pin, cnt_ctx_t* cb_ctx) {
    app_rotary_enc_t encoder = {
        .p_counter = &cb_ctx->cnt
    };

    const pcnt_unit_config_t unit_conf = {
        .low_limit = -USELESS_PCNT_LIMIT,
        .high_limit = USELESS_PCNT_LIMIT,
        .clk_src = PCNT_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(
        pcnt_new_unit(&unit_conf, &encoder.unit_h)
    );

    const pcnt_glitch_filter_config_t glitch_cfg = {.max_glitch_ns = GLITCH_FILTER_NS};
    ESP_ERROR_CHECK(
        pcnt_unit_set_glitch_filter(encoder.unit_h, &glitch_cfg)
    );

    encoder.chan_a_h = mk_pcnt_chan(encoder.unit_h, clk_pin, dt_pin);
    encoder.chan_b_h = mk_pcnt_chan(encoder.unit_h, dt_pin, clk_pin);

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        encoder.chan_a_h,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        encoder.chan_b_h,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(
        enable_pcnt_level_action(encoder.chan_a_h)
    );
    ESP_ERROR_CHECK(
        disable_pcnt_level_action(encoder.chan_b_h)
    );

    const pcnt_event_callbacks_t cb = {.on_reach = pcnt_on_reach};
    ESP_ERROR_CHECK(
        pcnt_unit_register_event_callbacks(encoder.unit_h, &cb, cb_ctx)
    );
    ESP_ERROR_CHECK(
        pcnt_unit_add_watch_point(encoder.unit_h, -2)
    );
    ESP_ERROR_CHECK(
        pcnt_unit_add_watch_point(encoder.unit_h, 2)
    );

    ESP_ERROR_CHECK(
        pcnt_unit_enable(encoder.unit_h)
    );
    ESP_ERROR_CHECK(
        pcnt_unit_clear_count(encoder.unit_h)
    );
    ESP_ERROR_CHECK(
        pcnt_unit_start(encoder.unit_h)
    );

    return encoder;
}

void rotary_enc_switch_mode(const app_rotary_enc_t* e, bool enable_x4) {
    if (enable_x4) {
        enable_pcnt_level_action(e->chan_b_h);
    } else {
        disable_pcnt_level_action(e->chan_b_h);
    }
}

int rotary_enc_set_val(app_rotary_enc_t* e, int val) {
    app_counter_t* pc = e->p_counter;

    if (val < pc->lo) {
        val = pc->lo;
    } else if (val > pc->hi) {
        val = pc->hi;
    }

    pc->v = val;

    return val;
}
