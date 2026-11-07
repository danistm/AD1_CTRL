#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/mainViewBase.hpp>
#include <gui/main_screen/mainPresenter.hpp>
#include <gui/common/ChannelMeter.hpp>

class mainView : public mainViewBase
{
public:
    mainView();
    virtual ~mainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    void applyMeterMode(bool stereo);
    virtual void handleKeyEvent(uint8_t key);
    enum TopStatus { TOP_IDLE, TOP_STREAMING, TOP_ERROR };
    void setTopStatus(TopStatus s, const char* errText = 0);
protected:

private:
    ChannelMeter meters[4];
    bool stereoMode = false;
    TopStatus topStatus = TOP_IDLE;
    //bool errorTest = false;

    bool shownUsb = false;
    bool shownStream = false;
    uint8_t shownRateIdx = 0xFF;
    void refreshLink();
    void showRate(uint8_t idx);

};

#endif // MAINVIEW_HPP
