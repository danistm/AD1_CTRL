/*
 * splashView.hpp — AD-1 CTL startup screen (set as Startup screen in
 * Designer). Drives the boot sequencer, shows a time-based progress
 * bar, freezes with a red message on a failed check, otherwise goes to
 * the main screen after BOOT_SETTLE_MS.
 * Location: TouchGFX/gui/include/gui/splash_screen/
 */
#ifndef SPLASHVIEW_HPP
#define SPLASHVIEW_HPP

#include <gui_generated/splash_screen/splashViewBase.hpp>
#include <gui/splash_screen/splashPresenter.hpp>

class splashView : public splashViewBase
{
public:
    splashView();
    virtual ~splashView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:

private:
    bool finished;
};

#endif
