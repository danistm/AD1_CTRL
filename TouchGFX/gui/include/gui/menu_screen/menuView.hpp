/*
 * menuView.hpp — AD-1 CTL settings menu (5-key navigation)
 * Location: TouchGFX/gui/include/gui/menu_screen/
 *
 * Step 1.2: the RATE item is locked (dimmed, skipped by Up/Down, not
 * enterable) while the USB host owns the sample rate (XMOS_RDY high).
 * The lock follows the line live while the menu is open.
 */
#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/menuViewBase.hpp>
#include <gui/menu_screen/menuPresenter.hpp>
#include "menu_settings.h"

class menuView : public menuViewBase
{
public:
    menuView();
    virtual ~menuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleKeyEvent(uint8_t key);
    virtual void handleTickEvent();

protected:

private:
    enum Item : uint8_t
    {
        IT_RATE = 0, IT_CH2, IT_KEYB, IT_DISPB, IT_FILT, IT_EXIT,
        IT_COUNT
    };

    uint8_t selected;     /* current item                               */
    bool    editing;      /* false = item level, true = value level     */

    /* inactivity timeout: leave without applying (cancel)              */
    static const uint16_t IDLE_TIMEOUT_TICKS = 60u * 57u;  /* ~60 s     */
    uint16_t     idleTicks;
    MenuSettings entrySettings;   /* snapshot for revert-on-timeout     */

    /* step 1.2: rate ownership                                          */
    bool    rateLocked;      /* USB host owns the rate (XMOS_RDY high)   */
    uint8_t shownLiveIdx;    /* live rate last drawn while locked        */
    bool    itemLocked(uint8_t item) const;

    void moveSelection(int dir);
    void changeValue(int dir);
    void applyAndExit();
    void updateVisuals();

    static void applyKeyBrightness(uint8_t level10);
    static void applyDisplayBrightness(uint8_t level10);
};

#endif
