/*
 * ChannelMeter.cpp — AD-1 CTL
 * Location: TouchGFX/gui/src/common/
 *
 * Bitmap names assume the assets were imported as:
 *   meter_bar_lit_22.png / meter_bar_dim_22.png
 *   meter_bar_lit_46.png / meter_bar_dim_46.png
 * -> IDs BITMAP_METER_BAR_LIT_22_ID etc. Adjust if your import
 *    names differ (see BitmapDatabase.hpp after generation).
 */

#include <gui/common/ChannelMeter.hpp>
#include <touchgfx/Color.hpp>
#include <BitmapDatabase.hpp>

/* --- ballistics constants, frames at ~57 fps --- */
static const float    FLOOR_DB         = -60.0f;
static const float    CLIP_THRESH_DB   = -0.1f;
static const float    RELEASE_PER_TICK = 20.0f / (1.7f * 57.0f); /* ~0.21 dB */
static const uint16_t PEAK_HOLD_TICKS  = 85;                     /* ~1.5 s   */

ChannelMeter::ChannelMeter()
    : inputDb(FLOOR_DB), dispDb(FLOOR_DB), peakDb(FLOOR_DB),
      peakHoldCnt(0), clipLatched(false), lastLevelPx(0), lastPeakPx(0)
{
}

int16_t ChannelMeter::dbToPx(float db)
{
    if (db <= FLOOR_DB) return 0;
    if (db >= 0.0f)     return BAR_LEN;
    return (int16_t)(BAR_LEN * (60.0f + db) * (1.0f / 60.0f) + 0.5f);
}

void ChannelMeter::setup(bool tall)
{
	removeAll();
	litClip.removeAll();
    const touchgfx::BitmapId dimId =
        tall ? BITMAP_METER_BAR_DIM_46_ID : BITMAP_METER_BAR_DIM_22_ID;
    const touchgfx::BitmapId litId =
        tall ? BITMAP_METER_BAR_LIT_46_ID : BITMAP_METER_BAR_LIT_22_ID;

    const int16_t h = touchgfx::Bitmap(dimId).getHeight();
    setWidth(METER_W);
    setHeight(h);

    dimImg.setBitmap(touchgfx::Bitmap(dimId));
    dimImg.setXY(BAR_X, 0);

    litImg.setBitmap(touchgfx::Bitmap(litId));
    litImg.setXY(0, 0);                 /* relative to litClip          */
    litClip.setPosition(BAR_X, 0, 0, h);/* width 0 = silent             */
    litClip.add(litImg);

    peakBox.setPosition(BAR_X, 0, 2, h);
    peakBox.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    peakBox.setVisible(false);

    clipBox.setPosition(CLIP_X, (h - CLIP_SZ) / 2, CLIP_SZ, CLIP_SZ);
    clipBox.setColor(touchgfx::Color::getColorFromRGB(30, 30, 30));

    add(dimImg);
    add(litClip);
    add(peakBox);
    add(clipBox);

    lastLevelPx = -1;
    lastPeakPx	= -1;
    clipBox.setColor(clipLatched
    		? touchgfx::Color::getColorFromRGB(255, 0, 0)
    		: touchgfx::Color::getColorFromRGB(30, 30, 30));
}

void ChannelMeter::setInputDb(float dbfs)
{
    inputDb = dbfs;
}

void ChannelMeter::tick()
{
    /* --- bar: instant attack, standard release --- */
    if (inputDb > dispDb)
    {
        dispDb = inputDb;                       /* attack               */
    }
    else
    {
        dispDb -= RELEASE_PER_TICK;             /* release              */
        if (dispDb < FLOOR_DB) dispDb = FLOOR_DB;
    }

    /* --- peak hold --- */
    if (inputDb >= peakDb)
    {
        peakDb      = inputDb;
        peakHoldCnt = PEAK_HOLD_TICKS;
    }
    else if (peakHoldCnt > 0)
    {
        peakHoldCnt--;
    }
    else
    {
        peakDb -= RELEASE_PER_TICK;
    }
    if (peakDb < dispDb) peakDb = dispDb;       /* peak never below bar */

    /* --- clip latch --- */
    if (!clipLatched && inputDb >= CLIP_THRESH_DB)
    {
        clipLatched = true;
        clipBox.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
        clipBox.invalidate();
    }

    /* --- push to widgets only when pixels changed --- */
    const int16_t lp = dbToPx(dispDb);
    const int16_t pp = dbToPx(peakDb);
    if (lp != lastLevelPx || pp != lastPeakPx)
    {
        litClip.setWidth(lp);

        const bool peakVisible = (pp > 2);
        peakBox.setVisible(peakVisible);
        if (peakVisible)
        {
            peakBox.setX(BAR_X + pp - 2);
        }

        /* invalidate the bar strip only (not the clip latch)           */
        touchgfx::Rect r(BAR_X, 0, BAR_LEN + 2, getHeight());
        invalidateRect(r);

        lastLevelPx = lp;
        lastPeakPx  = pp;
    }
}

void ChannelMeter::clearClipLatch()
{
    if (clipLatched)
    {
        clipLatched = false;
        clipBox.setColor(touchgfx::Color::getColorFromRGB(30, 30, 30));
        clipBox.invalidate();
    }
}
