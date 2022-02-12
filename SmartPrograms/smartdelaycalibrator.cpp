#include "smartdelaycalibrator.h"

SmartDelayCalibrator::SmartDelayCalibrator(SmartProgramParameter parameter) : SmartProgramBase(parameter)
{
    init();
}

void SmartDelayCalibrator::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    A_TestButtons.clear();
    for (int i = 0; i < COUNT_MAX; i++)
    {
        A_TestButtons.push_back(CaptureArea(257 + i * 80, 295, 48, 48));
    }
}

void SmartDelayCalibrator::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;

    m_delay = 0;
    m_count = 0;
}

void SmartDelayCalibrator::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_parameters.vlcWrapper->setPoints({P_Home});
        m_substage = SS_GotoHome;
        setState_runCommand(C_HomePress);
        emit printLog("Going to HOME menu...");
        break;
    }
    case SS_GotoHome:
    {
        if (state == S_CaptureReady)
        {
            // Check pixel color if we are at home
            bool atHome = checkPixelColorMatch(P_Home.m_point, C_Color_Home);
            bool atHome2 = checkPixelColorMatch(P_Home.m_point, C_Color_Home2);
            if (atHome || atHome2)
            {
                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas(A_TestButtons);

                // Goto "Test Controller Buttons"
                m_substage = SS_GoToTestButton;
                setState_runCommand(C_ToTestButton);
                emit printLog("Going to \"Test Controller Buttons\"...");
            }
            else
            {
                // Not home, try again
                m_substage = SS_GotoHome;
                setState_runCommand(C_HomePress);
                emit printLog("Not at HOME menu, trying again...");
            }
        }
        else if (state == S_CommandFinished)
        {
            // Grab frame to check if we are at home
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_GoToTestButton:
    {
        if (state == S_CommandFinished)
        {
            m_timer.start();
            m_substage = SS_CalibrateDelay;

            // emit command without calling set state, we need to do frame analyze immediately
            qDebug() << "A,1,Nothing,200";
            emit runSequence(m_commands[C_APress]);
            emit printLog("Press A to test delay...");

            // Grab frame to check "A" appears
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_CalibrateDelay:
    {
        if (state == S_CaptureReady)
        {
            bool buttonShowUp = checkBrightnessMeanTarget(A_TestButtons[m_count].m_rect, C_Color_TestButton, 40);
            bool buttonShowUp2 = checkBrightnessMeanTarget(A_TestButtons[m_count].m_rect, C_Color_TestButton2, 40);
            if (buttonShowUp || buttonShowUp2)
            {
                // A button shows up!
                m_delay += static_cast<int>(m_timer.restart());

                m_count++;
                if (m_count == COUNT_MAX)
                {
                    m_delay /= m_count;
                    emit printLog("Average delay calibrated " + QString::number(m_delay) + "ms");
                    m_substage = SS_BackToGame;
                    setState_runCommand(C_BackToGame);
                }
                else
                {
                    setState_runCommand(C_APress, true);
                }
            }
            else
            {
                // Not show up yet...
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_BackToGame:
    {
        if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
