#include <stdatomic.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/gptimer.h>
#include <esp_check.h>

#include "buzzer.h"

#define SPEED_MODE LEDC_LOW_SPEED_MODE

#define NOTE_DUTY 768

#define VM_TIMER_HZ 1'000'000
#define VM_TICK_MS  5
#define VM_TICK_CNT (VM_TICK_MS*1000)

#define E4 330
#define A3 220

static const note_t wrong[] = {
    {E4, 220},
    { 0,  45},
    {A3, 450},
    { 0, 120},
    { 0,   0}
};

esp_err_t init_ledc(gpio_num_t ctrl_pin, ledc_timer_t timer, ledc_channel_t chan);
esp_err_t init_vm_timer(app_buzzer_t* player);

static bool on_buzzer_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

void setup_buzzer(const app_buzzer_cfg config, app_buzzer_t* player) {
    ESP_ERROR_CHECK(
        init_ledc(config.ctrl_pin, config.ledc_timer, config.ledc_channel)
    );

    player->note = NULL;
    player->now_playing = false;
    player->chan = config.ledc_channel;
    player->tim = config.ledc_timer;

    ESP_ERROR_CHECK(
        init_vm_timer(player)
    );
}

bool play_wrong(app_buzzer_t* player) {
    note_t* no_note = NULL;

    return atomic_compare_exchange_strong(&player->note, &no_note, (note_t *) wrong);
}

esp_err_t init_ledc(gpio_num_t ctrl_pin, ledc_timer_t timer, ledc_channel_t chan) {
    esp_err_t ret;

    const ledc_timer_config_t timer_config = {
        .speed_mode      = SPEED_MODE,
        .timer_num       = timer,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ret = ledc_timer_config(&timer_config);
    ESP_RETURN_ON_ERROR(ret, __func__, "ledc_timer_config failed: %s", esp_err_to_name(ret));

    const ledc_channel_config_t channel_config = {
        .gpio_num   = ctrl_pin,
        .speed_mode = SPEED_MODE,
        .channel    = chan,
        .timer_sel  = timer,
        .duty       = 0,
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    return ledc_channel_config(&channel_config);
}

esp_err_t init_vm_timer(app_buzzer_t* player) {
    esp_err_t r;

    gptimer_handle_t timer_h;
    const gptimer_config_t tm_conf = {
        .resolution_hz = VM_TIMER_HZ,
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
    };
    r = gptimer_new_timer(&tm_conf, &timer_h);
    ESP_RETURN_ON_ERROR(r, __func__, "gptimer_new_timer failed: %s", esp_err_to_name(r));

    const gptimer_alarm_config_t tick_conf = {
        .alarm_count = VM_TICK_CNT,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    r = gptimer_set_alarm_action(timer_h, &tick_conf);
    ESP_RETURN_ON_ERROR(r, __func__, "gptimer_set_alarm_action failed: %s", esp_err_to_name(r));

    const gptimer_event_callbacks_t cb = {
        .on_alarm = on_buzzer_tick
    };
    r = gptimer_register_event_callbacks(timer_h, &cb, player);
    ESP_RETURN_ON_ERROR(r, __func__, "gptimer_register_event_callbacks failed: %s", esp_err_to_name(r));

    r = gptimer_enable(timer_h);
    ESP_RETURN_ON_ERROR(r, __func__, "gptimer_enable failed: %s", esp_err_to_name(r));

    return gptimer_start(timer_h);
}

void play_note(int freq, ledc_timer_t tim, ledc_channel_t chan) {
    if (freq == 0) {
        ESP_ERROR_CHECK(
            ledc_set_duty(SPEED_MODE, chan, 0)
        );
    } else {
        ESP_ERROR_CHECK(
            ledc_set_freq(SPEED_MODE, tim, freq)
        );
        ESP_ERROR_CHECK(
            ledc_set_duty(SPEED_MODE, chan, NOTE_DUTY)
        );
    }

    ESP_ERROR_CHECK(
        ledc_update_duty(SPEED_MODE, chan)
    );
}

bool on_buzzer_tick(gptimer_handle_t, const gptimer_alarm_event_data_t *, void *user_ctx) {
    app_buzzer_t* player = user_ctx;

    if (!player->note) {
        return false;
    }

    note_t* next_note = NULL;
    
    if (!player->now_playing) {
        player->now_playing = true;
        next_note = player->note;
    } else if (player->millis_left <= 0) {
        next_note = player->note + 1;
    } else {
        player->millis_left -= VM_TICK_MS;
    }

    if (next_note && next_note->duration_ms > 0) {
        player->note = next_note;
        player->millis_left = next_note->duration_ms;

        play_note(next_note->frequency, player->tim, player->chan);
    } else if (next_note) {
        player->note = NULL;
        player->now_playing = false;
    }

    return false;
}
