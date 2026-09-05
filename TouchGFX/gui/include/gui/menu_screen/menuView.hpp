/*
 * menuView.hpp — AD-1 CTL settings menu (5-key navigation)
 * Location: TouchGFX/gui/include/gui/menu_screen/
 * (adjust include of the generated base to your screen name)
 */
#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/menuViewBase.hpp>
#include <gui/menu_screen/menuPresenter.hpp>
#include <gui/common/menu_settings.h>

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

    void moveSelection(int dir);
    void changeValue(int dir);
    void applyAndExit();
    void updateVisuals();

    /* brightness stubs — wire the PWM CCR writes here later            */
    static void applyKeyBrightness(uint8_t level10);
    static void applyDisplayBrightness(uint8_t level10);
};

#endif
