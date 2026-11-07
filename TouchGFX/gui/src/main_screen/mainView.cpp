#include <gui/main_screen/mainView.hpp>
#include "menu_settings.h"
#include <cmath>
#include "KeypadController.hpp"
#include <BitmapDatabase.hpp>
#include "board_leds.h"
#include <texts/TextKeysAndLanguages.hpp>
#include "sai_metering.h"
#include "cpld_ctrl.h"
#include "xmos_link.h"
#include "rate_ctrl.h"
#include "adc_es9843.h"

mainView::mainView()
{

}

void mainView::setTopStatus(TopStatus s, const char* errText)
{
    topStatus = s;
    imgStream.setVisible(s == TOP_STREAMING);
    txtError.setVisible(s == TOP_ERROR);
    if (s == TOP_ERROR && errText)
    {
        Unicode::strncpy(txtErrorBuffer, errText, TXTERROR_SIZE);
        txtError.invalidate();
    }
    imgStream.invalidate();
    txtError.invalidate();
}

void mainView::showRate(uint8_t idx)
{
	Unicode::snprintf(txtRateBuffer, TXTRATE_SIZE, "%d", RateCtl_IdxToKhz(idx));
	txtRate.invalidate();
	shownRateIdx = idx;
}

void mainView::refreshLink()
{
	const bool usb = Xmos_Ready();
	if (usb != shownUsb)
	{
		shownUsb = usb;
		imgUsb.setVisible(usb);
		imgUsb.invalidate();
	}

	const bool str = Xmos_Streaming();
	if (str != shownStream)
	{
		shownStream = str;
		if (topStatus != TOP_ERROR)
		{
			setTopStatus(str ? TOP_STREAMING : TOP_IDLE);
		}
	}

	const uint8_t li = RateCtl_LiveIdx();
	if (li != shownRateIdx) showRate(li);
}

void mainView::setupScreen()
{
    mainViewBase::setupScreen();
    for (int i = 0; i < 4; i++)
    {
    	add(meters[i]);
    	meters[i].setChannelIndex(i);
    }
    applyMeterMode(g_settings.ch2On);

    shownUsb = Xmos_Ready();
    imgUsb.setVisible(shownUsb);
    imgUsb.invalidate();
    shownStream = Xmos_Streaming();
    setTopStatus(shownStream ? TOP_STREAMING : TOP_IDLE);
    shownRateIdx = 0xFFu;
    refreshLink();
}

void mainView::tearDownScreen()
{
    mainViewBase::tearDownScreen();
}

void mainView::applyMeterMode(bool stereo)
{
    static const int16_t rows4[4] = { 13, 38, 63, 88 };
    static const int16_t rows2[2] = { 14, 66 };

    backgroundImg.setBitmap(touchgfx::Bitmap(stereo
    		? BITMAP_METER_BACKGROUND_2CH_ID
    		: BITMAP_METER_BACKGROUND_4CH_ID));

    stereoMode = stereo;
    for (int i = 0; i < 4; i++)
    {
        const bool used = stereo ? (i < 2) : true;
        meters[i].setVisible(used);
        if (used)
        {
            meters[i].setup(stereo);              /* picks 22/46 bitmaps */
            meters[i].setXY(0, stereo ? rows2[i] : rows4[i]);
            Board_SetClipLed(i, false);			  /* reset clip LEDs on mode change */
        }
    }
    img2ch.setVisible(stereo);          /* amber circles = 2 bars shown  */
    invalidate();
}

void mainView::handleTickEvent()
{
    float db[4];
    SAI_Metering_GetPeaksDb(db);

    const int n = stereoMode ? 2 : 4;
    for (int i = 0; i < n; i++)
    {
        meters[i].setInputDb(db[i]);
        meters[i].tick();
    }

    if (ES9843_ClipConsume())
    {
        es9843_errors_t e;
        if (ES9843_PollErrors(&e) == HAL_OK)
        {
            for (int i = 0; i < n; i++)
                if (e.overload & (1u << i)) meters[i].setClipExternal();
            if (e.clock_anomaly) setTopStatus(TOP_ERROR, "ADC err");
        }
        else
        {
            setTopStatus(TOP_ERROR, "ADC err");       /* comms_fail */
        }
    }

    /* CPLD lock (EXTI1, both edges): loss -> error + mute; re-lock -> clear */
    {
        bool locked;
        if (CPLD_LockChanged(&locked))
        {
            if (locked)
            {
                CPLD_Mute(false);
                if (topStatus == TOP_ERROR)           /* back to icon state */
                    setTopStatus(shownStream ? TOP_STREAMING : TOP_IDLE);
            }
            else
            {
                CPLD_Mute(true);
                setTopStatus(TOP_ERROR, "CPLD err");
            }
        }
    }

    refreshLink();                 /* USB icon, STREAM icon, rate digits */

    SAI_Metering_Service();		   /* stall watchdog, as before          */
}

void mainView::handleKeyEvent(uint8_t key)
{

	mainViewBase::handleKeyEvent(key);

	if (key == Keys::Ok)
	{
	    for (int i = 0; i < 4; i++)
	       meters[i].clearClipLatch();
	}
}
