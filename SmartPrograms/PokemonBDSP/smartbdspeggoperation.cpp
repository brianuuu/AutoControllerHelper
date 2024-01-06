#include "smartbdspeggoperation.h"

SmartBDSPEggOperation::SmartBDSPEggOperation
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(setting)
{
    init();
}

void SmartBDSPEggOperation::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPEggOperation::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;

    m_resetCountdown = 0;
    m_watchColor = QColor(0,0,0);

    m_dialogDetected = false;
    m_eggsCollected = 0;
}

void SmartBDSPEggOperation::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_error, "Errors");
        initStat(m_statEggCollected, "Eggs Collected");
        initStat(m_statEggHatched, "Eggs Hatched");
        initStat(m_statShinyHatched, "Shinies");

        m_substage = SS_Start;
        setState_runCommand("Nothing,1");
        break;
    }
    case SS_Start:
    {
        if (state == S_CommandFinished)
        {
            if (m_resetCountdown == 0)
            {
                m_resetCountdown = 10;
                m_eggsCollected = 0;

                m_substage = SS_Restart;
                setState_runCommand(m_settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
            }
            else if (m_watchColor == QColor(0,0,0))
            {
                emit printLog("Calibrating Poketch color...");
                m_substage = SS_Watch;
                setState_runCommand("R,1,Nothing,30");

                m_videoManager->setAreas({A_Watch, A_Dialog});
            }
            else
            {
                if (m_programSettings.m_operation == EggOperationType::EOT_Collector)
                {
                    if (m_eggsCollected >= m_programSettings.m_targetEggCount)
                    {
                        m_substage = SS_Finished;
                        setState_runCommand("Home,2");
                    }
                    else
                    {
                        // more eggs to collect
                        m_substage = SS_CycleCollect;
                        setState_runCommand(C_CycleCollect);

                        m_videoManager->setAreas({A_Watch, A_Dialog});
                    }
                }
                else if (m_programSettings.m_operation == EggOperationType::EOT_Hatcher)
                {
                    // TODO:
                }
                else
                {
                    // TODO: reached shiny target?
                    if (m_eggsCollected >= 5)
                    {
                        // TODO: start hatching one column
                        m_eggsCollected = 0;
                        m_programSettings.m_columnsToHatch = 1;
                    }
                    else
                    {
                        // more eggs to collect
                        m_substage = SS_CycleCollect;
                        setState_runCommand(C_CycleCollect);

                        m_videoManager->setAreas({A_Watch, A_Dialog});
                    }
                }
            }
        }
        break;
    }
    case SS_Watch:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            m_watchColor = getAverageColor(A_Watch.m_rect);
            emit printLog("Poketch Color = {" + QString::number(m_watchColor.red()) + "," + QString::number(m_watchColor.green()) + "," + QString::number(m_watchColor.blue()) + "}");

            m_substage = SS_Start;
            setState_runCommand("Nothing,1");
        }
        break;
    }
    case SS_CycleCollect:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Collect;
            setState_runCommand(C_Collect, true);

            m_dialogDetected = false;
        }
        break;
    }
    case SS_Collect:
    {
        if (state == S_CommandFinished)
        {
            incrementStat(m_error);
            setState_error("Unable to detect dialog when attempting to collect egg");
        }
        else if (state == S_CaptureReady)
        {
            bool dialogDetected = checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 150);
            if (m_dialogDetected)
            {
                if (!dialogDetected)
                {
                    emit printLog("No eggs to collect");
                    m_substage = SS_Start;
                    setState_runCommand(C_CycleReturn);

                    m_videoManager->clearCaptures();
                    break;
                }
                else if (!checkAverageColorMatch(A_Watch.m_rect, m_watchColor))
                {
                    m_substage = SS_CollectSuccess;
                    setState_runCommand("ASpam,60,Loop,1,BSpam,2,Loop,0", true);

                    m_eggsCollected++;
                    incrementStat(m_statEggCollected);
                    m_timer.restart();

                    emit printLog("Egg collected! (" + QString::number(m_programSettings.m_targetEggCount - m_eggsCollected) + " remaining)");
                    m_videoManager->setAreas({A_Watch});
                    break;
                }
            }
            else
            {
                // waiting for dialog to show up
                if (dialogDetected)
                {
                    m_dialogDetected = true;
                }
            }

            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_CollectSuccess:
    {
        if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                incrementStat(m_error);
                setState_error("Unable to detect finish collecting egg");
            }
            else if (checkAverageColorMatch(A_Watch.m_rect, m_watchColor))
            {
                m_substage = SS_Start;
                setState_runCommand(C_CycleReturn);
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }

        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_substage = SS_Intro;

            m_videoManager->setAreas({A_Title});
            m_timer.restart();
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
            if (m_timer.elapsed() > 30000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect game start after title screen, the game might have froze or crashed. restarting...", LOG_ERROR);
                m_substage = SS_Restart;
                setState_runCommand(m_settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_GameStart)
                {
                    m_substage = SS_Start;
                    setState_runCommand("Nothing,1");
                }
                else
                {
                    setState_runCommand("ASpam,30,Nothing,20");
                    m_timer.restart();
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
