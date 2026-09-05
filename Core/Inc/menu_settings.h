/*
 * menu_settings.h — AD-1 CTL: settings shared between C (store, boot)
 * and C++ (TouchGFX views). Location: Core/Inc (add Core/Inc to the
 * TouchGFX gui include path if not already visible — it is, via main.h).
 */
#ifndef MENU_SETTINGS_H
#define MENU_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t rateIdx;      /* 0=48, 1=96, 2=192 kHz                      */
    bool    ch2On;        /* 2-channel (stereo parallel) mode           */
    uint8_t keyBright;    /* 0..10                                      */
    uint8_t dispBright;   /* 0..10                                      */
    uint8_t filtIdx;      /* 0..7 = es9843_filter_t                     */
} MenuSettings;

/* live settings instance (defined in settings_store.c)                 */
extern MenuSettings g_settings;

#ifdef __cplusplus
}
#endif
#endif
