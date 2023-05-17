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

    m_menuDetectFailed = false;
}

void SmartTOTKItemDuplication::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_CheckColor;
        setState_runCommand("LUp,1,Nothing,10,Plus,15,Nothing,10,Y,1");

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
                emit printLog("Attaching item");
                m_substage = SS_Attach;
                setState_runCommand("B,20," + m_commands[C_Attach]);
            }
            else
            {
                setState_error("Unable to detect menu color, program will not work properly");
            }
        }
        break;
    }
    case SS_Attach:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Dropping first arrow");
            m_substage = SS_DropFirst;
            setState_runCommand(C_DropFirst);
        }
        break;
    }
    case SS_DropFirst:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                m_substage = SS_QuickMenu;
                setState_runCommand("B,3,Plus,15,Nothing,10");
            }
            else
            {
                setState_error("Unable to detect menu color after dropping first arrow");
            }
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
            if (checkBrightnessMeanTarget(A_Menu.m_rect, C_Color_Menu, 230))
            {
                emit printLog("Dropping and picking up both arrows");
                m_substage = SS_DropSecond;
                setState_runCommand(C_DropSecond);

                m_menuDetectFailed = false;
            }
            else if (m_menuDetectFailed)
            {
                setState_error("Unable to detect menu color again");
            }
            else
            {
                emit printLog("Unable to detect menu color, retrying", LOG_WARNING);
                setState_runCommand("Plus,15,Nothing,10");

                m_loopLeft++;
                m_menuDetectFailed = true;
            }
        }
        break;
    }
    case SS_DropSecond:
    {
        if (state == S_CommandFinished)
        {
            m_loopLeft--;
            if (m_loopLeft == 0)
            {
                m_substage = SS_Finished;
                setState_runCommand("Home,1");
            }
            else
            {
                emit printLog("Attaching item (Loop left: " + QString::number(m_loopLeft) + ")");
                m_substage = SS_Attach;
                setState_runCommand(C_Attach);
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

