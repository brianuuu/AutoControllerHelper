#include "smartbdspdialgapalkia.h"

SmartBDSPDialgaPalkia::SmartBDSPDialgaPalkia
(
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();
}

void SmartBDSPDialgaPalkia::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPDialgaPalkia::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_noShinyTimer = 0.0;
}

void SmartBDSPDialgaPalkia::runNextState()
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
            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Battle,A_Pokemon});

            setState_frameAnalyzeRequest();
            if (m_noShinyTimer == 0.0)
            {
                emit printLog("Using first battle to calibrate time until Battle UI shows up...");
                m_substage = SS_Calibrate;
            }
            else
            {
                m_substage = SS_TestShiny;
            }
            m_elapsedTimer.restart();
        }
        break;
    }
    case SS_Calibrate:
    case SS_TestShiny:
    {
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Battle.m_rect, C_Color_Battle, 160) && checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Pokemon, 160))
            {
                // increment counter
                incrementStat(m_encounter);
                emit printLog("Current Encounter: " + QString::number(m_encounter.first));

                if (m_noShinyTimer == 0.0)
                {
                    m_noShinyTimer = m_elapsedTimer.elapsed();
                    emit printLog("Calibrated time = " + QString::number(m_noShinyTimer) + "ms");

                    m_substage = SS_Restart;
                    runRestartCommand();
                }
                else
                {
                    double elapsed = m_elapsedTimer.elapsed();
                    if (elapsed > m_noShinyTimer + 2000)
                    {
                        // shiny found!
                        m_substage = SS_Finish;
                        setState_runCommand(C_Capture);
                        emit printLog("Time taken: " + QString::number(elapsed) + "ms, SHINY FOUND!", LOG_SUCCESS);
                    }
                    else
                    {
                        if (elapsed < 500)
                        {
                            incrementStat(m_error);
                            emit printLog("Time taken: " + QString::number(elapsed) + "ms, unexpected low time, restarting...", LOG_ERROR);
                        }
                        else
                        {
                            // update timer in case delay has shifted
                            m_noShinyTimer = elapsed;
                            emit printLog("Time taken: " + QString::number(elapsed) + "ms, not shiny, restarting...");
                        }

                        // reset
                        m_substage = SS_Restart;
                        runRestartCommand();
                    }
                }

                m_parameters.vlcWrapper->clearCaptures();
            }
            else if (m_noShinyTimer > 0.0 && m_elapsedTimer.elapsed() > m_noShinyTimer + 10000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect battle UI for too long, restarting sequence...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();

                m_parameters.vlcWrapper->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
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

void SmartBDSPDialgaPalkia::runRestartCommand()
{
    setState_runCommand(m_parameters.settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
}
