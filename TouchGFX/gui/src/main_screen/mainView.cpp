#include <gui/main_screen/mainView.hpp>
#include <cmath>
#include "Keypadcontroller.hpp"
#include <BitmapDatabase.hpp>

#define METER_TEST_AUTOSWEEP 1

volatile float g_testDb[4] = { -60.0f, -60.0f, -60.0f, -60.0f };

mainView::mainView()
{

}

void mainView::setupScreen()
{
    mainViewBase::setupScreen();
    for (int i = 0; i < 4; i++)
    	{
            add(meters[i]);
        }
        applyMeterMode(false);            /* start in 4-channel mode        */
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
        }
    }
    invalidate();
}

void mainView::handleTickEvent()
{
#if METER_TEST_AUTOSWEEP
    static uint32_t t = 0;
    t++;
    g_testDb[0] = -30.0f + 30.0f * sinf(t * 0.045f);        /* full swing */
    g_testDb[1] = -24.0f + 10.0f * sinf(t * 0.020f + 1.f);  /* gentle     */
    g_testDb[2] = ((t / 120) % 2) ? -3.0f : -50.0f;         /* steps+clip
                                                               (hits -3;
                                                               raise to
                                                               0 to test
                                                               latch)    */
    g_testDb[3] = -18.0f;                                   /* static     */
#endif

    const int n = stereoMode ? 2 : 4;
    for (int i = 0; i < n; i++)
    {
        meters[i].setInputDb(g_testDb[i]);
        meters[i].tick();
    }

    /* keep your existing key-test box timer code here if still present */
}

void mainView::handleKeyEvent(uint8_t key)
{
	switch(key)
	{
	case Keys::Left:
		applyMeterMode(!stereoMode);
		break;

	case Keys::Ok:
		for (int i = 0 ; i < 4 ; i++)
		{
			meters[i].clearClipLatch();
		}
		break;

	default:
		break;
	}
}
