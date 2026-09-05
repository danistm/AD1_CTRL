/*
 * menu_settings.h — AD-1 CTL: settings shared between menu and main UI
 * Location: TouchGFX/gui/include/gui/common/
 */
#ifndef MENU_SETTINGS_H
#define MENU_SETTINGS_H

#include <stdint.h>

struct MenuSettings
{
    uint8_t rateIdx;      /* 0=48, 1=96, 2=192 kHz                      */
    bool    ch2On;        /* 2-channel (stereo parallel) mode           */
    uint8_t keyBright;    /* 0..10                                      */
    uint8_t dispBright;   /* 0..10                                      */
    uint8_t filtIdx;      /* 0..7 = es9843_filter_t                     */
};

/* live settings instance (defined in menuView.cpp) */
extern MenuSettings g_settings;

#endif
