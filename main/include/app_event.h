#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _event_name {
    SET_ANGLE,
    OVERFLOW,
    BTN_CLICKED,
    BTN_HOLD_STARTED
} event_name_t;

typedef struct _app_event {
    event_name_t name;
    int data;
} app_event_t;

#ifdef __cplusplus
}
#endif
