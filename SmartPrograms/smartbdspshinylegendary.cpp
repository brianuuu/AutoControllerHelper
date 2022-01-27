#include "smartbdspshinylegendary.h"

SmartBDSPShinyLegendary::SmartBDSPShinyLegendary
(
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();
}

void SmartBDSPShinyLegendary::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPShinyLegendary::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_noShinyTimer = 0.0;
}

void SmartBDSPShinyLegendary::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_encounter, "Encounters");
        initStat(m_error, "Errors");

        m_substage = SS_Restart;
        runRestartCommand();
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_substage = SS_Intro;

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Title});

            m_elapsedTimer.restart();
        }
        break;
    }
    case SS_Intro:
    case SS_Title:
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 30000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect game start after title screen, the game might have froze or crashed. restarting...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_GameStart)
                {
                    m_substage = SS_Talk;
                    setState_runCommand(C_Talk);
                    m_parameters.vlcWrapper->clearCaptures();
                }
                else
                {
                    setState_runCommand("Nothing,21,A,1,Nothing,50");
                    m_elapsedTimer.restart();
                    m_substage = (m_substage == SS_Intro) ? SS_Title : SS_GameStart;
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Talk:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_substage = SS_Detect1;

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Dialog});

            m_dialogWasFound = false;
            m_elapsedTimer.restart();
        }
        break;
    }
    case SS_Detect1: // wild starly appeared
    case SS_Detect2: // Go! <POKEMON>!
    {
        if (state == S_CaptureReady)
        {
            double elapsed = m_elapsedTimer.elapsed();
            if (elapsed > 15000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect dialog for too long, restarting sequence...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();

                break;
            }
            else
            {
                bool foundDialog = checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 230);
                if (m_dialogWasFound && !foundDialog)
                {
                    // dialog closed, goto next state
                    m_elapsedTimer.restart();
                    m_substage = SS_Detect2;
                }
                else if (!m_dialogWasFound && foundDialog)
                {
                    if (m_substage == SS_Detect1)
                    {
                        emit printLog("\"Legendary appeared!\" dialog detected");
                    }
                    else
                    {
                        // increment counter
                        incrementStat(m_encounter);
                        emit printLog("Current Encounter: " + QString::number(m_encounter.first));

                        QString str = "\"Go! Your Pokemon!\" dialog detected, time taken: " + QString::number(elapsed) + "ms, ";
                        if (elapsed > 2000)
                        {
                            // shiny found!
                            m_substage = SS_Finish;
                            setState_runCommand(C_Capture);
                            emit printLog(str + "SHINY FOUND!", LOG_SUCCESS);
                        }
                        else
                        {
                            // reset
                            m_substage = SS_Restart;
                            runRestartCommand();
                            emit printLog(str + "not shiny, restarting...");
                        }

                        m_parameters.vlcWrapper->clearCaptures();
                        break;
                    }
                }
                m_dialogWasFound = foundDialog;
            }
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_Finish:
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

void SmartBDSPShinyLegendary::runRestartCommand()
{
    setState_runCommand(m_parameters.settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
}
