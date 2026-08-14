/*
 * ChannelMeter.hpp — one horizontal dBFS bar meter (AD-1 CTL)
 *
 * Composition (all coords relative to this container):
 *   dim bar image  @ x=18  (full 250 px, always visible)
 *   lit bar image  @ x=18, clipped by litClip container width = level px
 *   peak-hold      2 px white Box
 *   clip latch     13x13 Box @ x=272, dark until latched red
 *
 * Data flow per frame:
 *   setInputDb(latest block peak in dBFS)   <- Model / test variable
 *   tick()                                  <- View::handleTickEvent()
 *
 * Ballistics per IEC 60268-18 at ~57 fps:
 *   attack instant, release 20 dB / 1.7 s, peak hold ~1.5 s then release.
 *
 * Location: TouchGFX/gui/include/gui/common/  (user code, safe)
 */

#ifndef GUI_INCLUDE_GUI_COMMON_CHANNELMETER_HPP_
#define GUI_INCLUDE_GUI_COMMON_CHANNELMETER_HPP_

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/Box.hpp>

class ChannelMeter : public touchgfx::Container
{
public:
    static const int16_t BAR_X    = 18;
    static const int16_t BAR_LEN  = 250;
    static const int16_t CLIP_X   = 272;
    static const int16_t CLIP_SZ  = 13;
    static const int16_t METER_W  = 290;   /* container width           */

    ChannelMeter();

    /* tall=false: 22 px bars (4-ch), tall=true: 46 px bars (stereo)    */
    void setup(bool tall);

    void setInputDb(float dbfs);   /* latest block peak, any rate       */
    void tick();                   /* once per frame                    */
    void clearClipLatch();
    bool isClipLatched() const { return clipLatched; }

private:
    static int16_t dbToPx(float db);

    touchgfx::Image     dimImg;
    touchgfx::Image     litImg;
    touchgfx::Container litClip;
    touchgfx::Box       peakBox;
    touchgfx::Box       clipBox;

    float    inputDb;
    float    dispDb;       /* bar level after ballistics                */
    float    peakDb;       /* peak-hold level                           */
    uint16_t peakHoldCnt;
    bool     clipLatched;
    int16_t  lastLevelPx;
    int16_t  lastPeakPx;
};





#endif /* GUI_INCLUDE_GUI_COMMON_CHANNELMETER_HPP_ */
