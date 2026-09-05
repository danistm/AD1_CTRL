/*
 * menuView.cpp — AD-1 CTL settings menu
 * Location: TouchGFX/gui/src/menu_screen/
 *
 * Focus model:
 *   ITEM level : Up/Down move selection (wraps), highlight = inverted
 *                label. Right enters VALUE level (except on EXIT).
 *                OK on EXIT applies settings and returns to main.
 *   VALUE level: highlight moves to the value widget. Up/Down change
 *                the value (brightness applies IMMEDIATELY). LEFT returns
 *                to ITEM level.
 */
#include <gui/menu_screen/menuView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>
#include "KeypadController.hpp"
#include "adc_es9843.h"
#include "backlight.h"
#include "settings_store.h"

static const char* const rateStr[3] = { "48 kHz", "96 kHz", "192 kHz" };
static const char* const ch2Str[2]  = { "OFF", "ON" };
static const char* const filtStr[8] =
{
    "MIN PHASE", "LIN APODIZ", "LIN FAST", "LIN FST LR",
    "LIN SLOW",  "MIN FAST",   "MIN SLOW", "MIN SLW LD"
};

static const touchgfx::colortype COL_WHITE =
        touchgfx::Color::getColorFromRGB(255, 255, 255);
static const touchgfx::colortype COL_BLACK =
        touchgfx::Color::getColorFromRGB(0, 0, 0);
static const touchgfx::colortype COL_BLUE =
		touchgfx::Color::getColorFromRGB(90, 170, 255);
static const touchgfx::colortype COL_ORANGE =
        touchgfx::Color::getColorFromRGB(255, 136, 0);
static const touchgfx::colortype COL_GREEN =
		touchgfx::Color::getColorFromRGB(70, 255, 3);

menuView::menuView() : selected(IT_RATE), editing(false) {}

void menuView::setupScreen()
{
    menuViewBase::setupScreen();
    selected      = IT_RATE;
    editing       = false;
    idleTicks     = 0u;
    entrySettings = g_settings;      /* snapshot for cancel-on-timeout  */
    updateVisuals();
}

void menuView::handleTickEvent()
{
    if (++idleTicks >= IDLE_TIMEOUT_TICKS)
    {
        /* cancel: discard edits, revert live brightness, leave         */
        g_settings = entrySettings;
        applyKeyBrightness(g_settings.keyBright);
        applyDisplayBrightness(g_settings.dispBright);
        application().gotomainScreenNoTransition();
    }
}

void menuView::tearDownScreen()
{
    menuViewBase::tearDownScreen();
}

void menuView::handleKeyEvent(uint8_t key)
{
    idleTicks = 0u;                  /* any key restarts the timeout    */

    menuViewBase::handleKeyEvent(key);

    if (!editing)
    {
        switch (key)
        {
        case Keys::Up:    moveSelection(-1); break;
        case Keys::Down:  moveSelection(+1); break;
        case Keys::Right:
            if (selected != IT_EXIT)
            {
                editing = true;
                updateVisuals();
            }
            break;
        case Keys::Ok:
            if (selected == IT_EXIT)
            {
                applyAndExit();
            }
            break;
        default: break;
        }
    }
    else
    {
        switch (key)
        {
        case Keys::Up:    changeValue(+1); break;
        case Keys::Down:  changeValue(-1); break;
        case Keys::Left:                 /* backward = leave value level */
            editing = false;
            updateVisuals();
            break;
        default: break;                  /* OK does nothing here         */
        }
    }
}

void menuView::moveSelection(int dir)
{
    selected = (uint8_t)((selected + IT_COUNT + dir) % IT_COUNT);
    updateVisuals();
}

void menuView::changeValue(int dir)
{
    switch (selected)
    {
    case IT_RATE:
        g_settings.rateIdx = (uint8_t)((g_settings.rateIdx + 3 + dir) % 3);
        break;
    case IT_CH2:
        g_settings.ch2On = !g_settings.ch2On;
        break;
    case IT_KEYB:
        if (dir > 0 && g_settings.keyBright < 10) g_settings.keyBright++;
        if (dir < 0 && g_settings.keyBright > 0)  g_settings.keyBright--;
        applyKeyBrightness(g_settings.keyBright);      /* immediate     */
        break;
    case IT_DISPB:
        if (dir > 0 && g_settings.dispBright < 10) g_settings.dispBright++;
        if (dir < 0 && g_settings.dispBright > 0)  g_settings.dispBright--;
        applyDisplayBrightness(g_settings.dispBright); /* immediate     */
        break;
    case IT_FILT:
        g_settings.filtIdx = (uint8_t)((g_settings.filtIdx + 8 + dir) % 8);
        break;
    default: break;
    }
    updateVisuals();
}

void menuView::applyAndExit()
{
    static const es9843_rate_t rateMap[3] =
        { ES9843_RATE_48K, ES9843_RATE_96K, ES9843_RATE_192K };

    /* only touch the ADC for values that actually changed              */
    if (g_settings.rateIdx != entrySettings.rateIdx)
        (void)ES9843_ChangeSampleRate(rateMap[g_settings.rateIdx]);
    if (g_settings.ch2On != entrySettings.ch2On)
        (void)ES9843_SetStereoMode(g_settings.ch2On);
    if (g_settings.filtIdx != entrySettings.filtIdx)
        (void)ES9843_SetFilter((es9843_filter_t)g_settings.filtIdx);

    (void)Settings_Save();           /* skips the write if unchanged     */

    /* generated by the forced interaction (see designer spec)         */
    application().gotomainScreenNoTransition();
}

void menuView::updateVisuals()
{
    /* ---- item label inversion (ITEM level highlight) ---- */
    struct Row { touchgfx::Box* sel; touchgfx::TextArea* lbl; };
    Row rows[IT_COUNT] =
    {
        { &boxSelRate,  &lblRate  },
        { &boxSelCh2,   &lblCh2   },
        { &boxSelKeyB,  &lblKeyB  },
        { &boxSelDispB, &lblDispB },
        { &boxSelFilt,  &lblFilt  },
        { &boxSelExit,  &lblExit  },
    };
    for (uint8_t i = 0; i < IT_COUNT; i++)
    {
        const bool hi = (i == selected) && !editing;
        rows[i].sel->setVisible(hi);
        rows[i].lbl->setColor(hi ? COL_BLACK : COL_WHITE);
    }

    /* ---- value highlight (VALUE level) ---- */
    const bool edRate = editing && selected == IT_RATE;
    const bool edCh2  = editing && selected == IT_CH2;
    const bool edFilt = editing && selected == IT_FILT;
    const bool edKeyB = editing && selected == IT_KEYB;
    const bool edDisB = editing && selected == IT_DISPB;

    boxValRate.setVisible(edRate);
    valRate.setColor(edRate ? COL_BLACK : COL_BLUE);
    boxValCh2.setVisible(edCh2);
    valCh2.setColor(edCh2 ? COL_BLACK : COL_ORANGE);
    boxValFilt.setVisible(edFilt);
    valFilt.setColor(edFilt ? COL_BLACK : COL_GREEN);
    boxValKeyB.setVisible(edKeyB);      /* thick frame around the bar   */
    boxValDispB.setVisible(edDisB);

    /* ---- values ---- */
    Unicode::strncpy(valRateBuffer, rateStr[g_settings.rateIdx],
                     VALRATE_SIZE);
    Unicode::strncpy(valCh2Buffer, ch2Str[g_settings.ch2On ? 1 : 0],
                     VALCH2_SIZE);
    Unicode::strncpy(valFiltBuffer, filtStr[g_settings.filtIdx],
                     VALFILT_SIZE);
    valRate.invalidate();
    valCh2.invalidate();
    valFilt.invalidate();

    /* ---- bars: Box Progress indicators, range 0..10 ---- */
    barKey.setValue(g_settings.keyBright);
    barDisp.setValue(g_settings.dispBright);

    invalidate();   /* menu interaction is sparse: full redraw is fine  */
}

void menuView::applyKeyBrightness(uint8_t level10)
{
    Backlight_SetKey(level10);
}

void menuView::applyDisplayBrightness(uint8_t level10)
{
    Backlight_SetDisplay(level10);
}
