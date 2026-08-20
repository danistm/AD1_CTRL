/*
 * sai_metering.h — AD-1 CTL: 4-channel peak capture from SAI1 A+B
 *
 * SAI1 Block A (slots 0/1) -> channels 1/2
 * SAI1 Block B (slots 0/1) -> channels 3/4
 *
 * Model: DMA circular capture, per-half-buffer |peak| accumulation with
 * max-hold between UI reads. The UI calls SAI_Metering_GetPeaksDb() once
 * per frame; the call returns the highest sample peak per channel since
 * the previous call (read-and-reset), so no peak between UI frames is
 * ever lost regardless of sample rate (44.1k..192k).
 */
#ifndef SAI_METERING_H
#define SAI_METERING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Start both SAI DMA streams (call once, after MX_DMA/MX_SAI1 init).   */
void SAI_Metering_Start(void);

/* Read-and-reset accumulated peaks, converted to dBFS.
 * Silence (no signal / no clocks) returns -90.0f per channel.          */
void SAI_Metering_GetPeaksDb(float dbfs[4]);

/* Supervisor: recovers capture after clock discontinuities (sample-rate
 * changes, unplugged clocks). Call once per UI frame, e.g. right after
 * SAI_Metering_GetPeaksDb() in handleTickEvent().                      */
void SAI_Metering_Service(void);

/* Diagnostics: count of SAI/DMA error callbacks since boot.            */
uint32_t SAI_Metering_ErrorCount(void);

#ifdef __cplusplus
}
#endif
#endif
