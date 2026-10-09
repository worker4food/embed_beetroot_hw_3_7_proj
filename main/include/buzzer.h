#pragma once

#include <driver/ledc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _app_buzzer_cfg {
    int ctrl_pin;
    ledc_timer_t ledc_timer;
    ledc_channel_t ledc_channel;
} app_buzzer_cfg;

typedef struct {
    int16_t frequency;
    int16_t duration_ms;
} note_t;

typedef struct _app_buzzer_t {
    _Atomic(note_t*) note;
    bool now_playing;
    int millis_left;
    ledc_timer_t tim;
    ledc_channel_t chan;
} app_buzzer_t;

void setup_buzzer(const app_buzzer_cfg config, app_buzzer_t* player);
bool play_wrong(app_buzzer_t *player);

#ifdef __cplusplus
}
#endif
