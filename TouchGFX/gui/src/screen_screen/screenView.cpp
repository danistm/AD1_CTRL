#include <gui/screen_screen/screenView.hpp>
#include "Keypadcontroller.hpp"

screenView::screenView()
{

}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();
    touchgfx::Box* map[5] = { &boxOK, &boxUP, &boxPR, &boxFW, &boxDN };
    for (int i = 0; i < 5; i++) { boxes[i] = map[i]; timers[i] = 0; }
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}

void screenView::handleKeyEvent(uint8_t key)
{
    int idx = -1;
    switch (key)
    {
    case Keys::Ok:    	idx = 0; break;
    case Keys::Up:  	idx = 1; break;
    case Keys::Left:  	idx = 2; break;
    case Keys::Right: 	idx = 3; break;
    case Keys::Down:    idx = 4; break;
    }
    if (idx >= 0)
    {
        boxes[idx]->setVisible(true);
        boxes[idx]->invalidate();
        timers[idx] = SHOW_TICKS;
    }
}

void screenView::handleTickEvent()
{
    for (int i = 0; i < 5; i++)
    {
        if (timers[i] != 0 && --timers[i] == 0)
        {
            boxes[i]->setVisible(false);
            boxes[i]->invalidate();
        }
    }
}
