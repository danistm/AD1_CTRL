/*
 * settings_store.h — persistent settings in the 24AA02E64
 *
 * EEPROM map (user area 0x00..0xF7):
 *   0x00  SettingsBlock (16 B)  rate/mode/brightness/filter
 *   0x40  CalBlock      (reserved: THD compensation, future)
 *   0xF8  EUI-64 (factory, read-only) -> unit serial number
 */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "menu_settings.h"
#include <stdbool.h>

#define SETTINGS_EEPROM_ADDR   (0x00u)
#define CAL_EEPROM_ADDR        (0x40u)

/* Load into g_settings. Falls back to defaults (and writes them) if the
 * block is missing/corrupt. Returns true if a valid block was loaded. */
bool Settings_Load(void);

/* Persist g_settings. Skips the write if nothing changed since the last
 * load/save (wear + time). Returns false on EEPROM error.              */
bool Settings_Save(void);

void Settings_Defaults(MenuSettings *s);

#ifdef __cplusplus
}
#endif
#endif
