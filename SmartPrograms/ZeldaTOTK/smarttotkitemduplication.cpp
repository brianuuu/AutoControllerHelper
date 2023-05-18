#include "smarttotkitemduplication.h"

SmartTOTKItemDuplication::SmartTOTKItemDuplication
(
    int loopCount,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_loopLeft(loopCount)
{
    init();
}

void SmartTOTKItemDuplication::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartTOTKItemDuplication::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;

    m_menuDetected = false;
}

void SmartTOTKItemDuplication::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_CheckColor;
        setState_runCommand("LUp,1,Nothing,10,Plus,15,Nothing,10");

        m_videoManager->setAreas({A_Menu});
        break;
    }
    case SS_CheckColor:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                m_substage = SS_Start;
                setState_runCommand("B,1,Nothing,20");
            }
            else
            {
                setState_error("Unable to detect menu color, program will not work properly");
            }
        }
        break;
    }
    case SS_Start:
    {
        if (state == S_CommandFinished)
        {
            if (m_loopLeft <= 0)
            {
                m_substage = SS_Finished;
                setState_runCommand("Home,1");
            }
            else
            {
                emit printLog("Attaching item (Loop left: " + QString::number(m_loopLeft) + ")");
                m_substage = SS_Attach;
                setState_runCommand(C_Attach, true);

                m_timer.restart();
                m_loopLeft--;
            }
        }
        break;
    }
    case SS_Attach:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 3000)
            {
                setState_error("Unable to detect menu color after dropping first arrow");
            }
            else if (checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                emit printLog("Dropping first arrow");
                m_substage = SS_DropFirst;
                setState_runCommand(C_DropFirst);
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_DropFirst:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_QuickMenu;
            setState_runCommand("B,3,Plus,3", true);

            m_menuDetected = true;
            m_timer.restart();
        }
        break;
    }
    case SS_QuickMenu:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 3000)
            {
                if (m_menuDetected)
                {
                    // this should NOT happen
                    setState_error("Error occured doing quick pause");
                    break;
                }

                emit printLog("Unable to detect menu color for too long, retrying", LOG_WARNING);
                setState_runCommand("Plus,3", true);

                m_loopLeft++;
                m_timer.restart();
            }
            else if (m_menuDetected && !checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                // menu closed
                m_menuDetected = false;
                setState_frameAnalyzeRequest();
            }
            else if (!m_menuDetected && checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                // menu reopened
                m_menuDetected = true;

                emit printLog("Dropping and picking up both arrows");
                m_substage = SS_Start;
                setState_runCommand(C_DropSecond);
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Finished:
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

