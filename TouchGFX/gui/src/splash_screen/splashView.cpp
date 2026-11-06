/*
 * splashView.cpp — AD-1 CTL
 * Location: TouchGFX/gui/src/splash_screen/
 */
#include <gui/splash_screen/splashView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include "boot_seq.h"

splashView::splashView() : finished(false) {}

void splashView::setupScreen()
{
    splashViewBase::setupScreen();
    finished = false;

    barBoot.setValue(0);
    barBoot.setVisible(true);
    txtBootError.setVisible(false);
    imgLogo.setVisible(true);
    imgLogoError.setVisible(false);

    Boot_Begin();
}

void splashView::tearDownScreen()
{
    splashViewBase::tearDownScreen();
}

void splashView::handleTickEvent()
{
    if (finished) return;

    const boot_state_t st = Boot_Tick();

    switch (st)
    {
    case BOOT_RUNNING:
        barBoot.setValue(Boot_ProgressPct());
        break;

    case BOOT_DONE:
        barBoot.setValue(100);
        finished = true;
        application().gotomainScreenNoTransition();
        break;

    case BOOT_FAILED:
        /* red message replaces the bar, red logo replaces the normal
         * one; frozen here on purpose                                 */
        barBoot.setVisible(false);
        barBoot.invalidate();
        Unicode::strncpy(txtBootErrorBuffer, Boot_ErrorText(), TXTBOOTERROR_SIZE);
        txtBootError.setVisible(true);
        txtBootError.invalidate();

        imgLogo.setVisible(false);
        imgLogoError.setVisible(true);
        imgLogo.invalidate();           /* same rect: either invalidate */
        imgLogoError.invalidate();      /* covers the swap              */
        finished = true;
        break;
    }
}
