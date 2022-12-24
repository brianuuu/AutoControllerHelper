#include "smartsvgimmighoulfarmer.h"

SmartSVGimmighoulFarmer::SmartSVGimmighoulFarmer
(
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();
}

void SmartSVGimmighoulFarmer::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSVGimmighoulFarmer::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartSVGimmighoulFarmer::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_ToTime;
        setState_runCommand(C_ToTime);
        break;
    }
    case SS_ToTime:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_SkipDay;
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: setState_runCommand(C_SkipJPDay); break;
                case DA_EU: setState_runCommand(C_SkipEUDay); break;
                case DA_US: setState_runCommand(C_SkipUSDay); break;
            }
        }
        break;
    }
    case SS_SkipDay:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Restart;
            setState_runCommand(C_Restart);
        }
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                emit printLog("Unable to detect black screen after restarting, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_Title;
                setState_frameAnalyzeRequest();
                m_timer.restart();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Title:
    {
        if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                emit printLog("Unable to detect title screen, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                emit printLog("Title detected!");
                m_substage = SS_GameStart;
                setState_runCommand("ASpam,310");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                emit printLog("Unable to detect game start, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                // talk to gimmighoul if it exist
                emit printLog("Attempting to collect Gimmighoul coin...");
                m_substage = SS_MainMenu;
                setState_runCommand(C_Farm);
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_MainMenu:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_PartyFirst});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                emit printLog("Unable to detect main menu, forcing restart", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else
            {
                if (checkBrightnessMeanTarget(A_PartyFirst.m_rect, C_Color_Yellow, 200))
                {
                    // save game
                    m_substage = SS_Save;
                    setState_runCommand(C_Save);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    setState_frameAnalyzeRequest();
                }
            }
        }
        break;
    }
    case SS_Save:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_ToTime;
            setState_runCommand(C_ToTime);
            break;
        }
    }
    }

    SmartProgramBase::runNextState();
}
