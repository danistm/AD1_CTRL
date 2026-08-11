#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleKeyEvent(uint8_t key);
    virtual void handleTickEvent();
protected:

private:
    static const uint8_t SHOW_TICKS = 20;
    touchgfx::Box* boxes[5];
    uint8_t timers[5];
};

#endif // SCREENVIEW_HPP
