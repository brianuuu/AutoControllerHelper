#include "smartpladistortionwaiter.h"

SmartPLADistortionWaiter::SmartPLADistortionWaiter(SmartProgramParameter parameter) : SmartProgramBase(parameter)
{
    init();
}

void SmartPLADistortionWaiter::init()
{
    SmartProgramBase::init();
}

void SmartPLADistortionWaiter::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartPLADistortionWaiter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_parameters.vlcWrapper->clearAreas();
        m_parameters.vlcWrapper->setAreas({A_Text});

        m_timer.restart();
        m_substage = SS_Capture;
        setState_ocrRequest(A_Text.m_rect, C_Color_Text);
        break;
    }
    case SS_Capture:
    {
        if (state == S_OCRReady)
        {
            GameLanguage const gameLanguage = m_parameters.settings->getGameLanguage();
            PokemonDatabase::OCREntries const& entries = PokemonDatabase::instance().getEntries_PLADistortion(gameLanguage);

            int result = matchStringDatabase(entries);
            if (result >= 0)
            {
                static QMap<QString, NotificationType> map =
                {
                    {"DistortionForming",   NT_Forming},
                    {"DistortionAppeared",  NT_Appeared},
                    {"DistortionFaded",     NT_Faded},
                };

                auto iter = map.find(entries[result].first);
                if (iter != map.end())
                {
                    if (iter.value() == NT_Appeared || iter.value() == NT_Forming)
                    {
                        m_substage = SS_Found;
                        setState_runCommand("Minus,1,Nothing,20");
                        break;
                    }
                    else if (iter.value() == NT_Faded)
                    {
                        emit printLog("You have missed a distortion...", LOG_WARNING);
                    }
                }
                else
                {
                    emit printLog("Invalid notification type", LOG_ERROR);
                }
            }

            // run OCR every 2 seconds
            m_substage = SS_Init;
            qint64 elpsed = m_timer.elapsed();
            if (2000 - elpsed < 0)
            {
                runNextStateContinue();
            }
            else
            {
                runNextStateDelay(2000 - static_cast<int>(elpsed));
            }
        }
        break;
    }
    case SS_Found:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Distortion found!", LOG_SUCCESS);
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
