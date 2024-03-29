#include "smartbdspshinylegendary.h"

SmartBDSPShinyLegendary::SmartBDSPShinyLegendary
(
    LegendaryType type,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_type(type)
{
    init();

    if (m_type >= LT_COUNT)
    {
        setState_error("Invalid legendary type!");
    }
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

        if (m_type == LT_Shaymin)
        {
            // No restart on Shaymin
            m_substage = SS_Talk;
            setState_runCommand(C_TalkShaymin);
        }
        else
        {
            m_substage = SS_Restart;
            runRestartCommand();
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
                    switch (m_type)
                    {
                        case LT_DialgaPalkia: setState_runCommand(C_TalkDialgaPalkia); break;
                        case LT_Regigigas: setState_runCommand(C_TalkRegigigas); break;
                        case LT_Shaymin: m_substage = SS_RestartShaymin; setState_runCommand("Nothing,21"); break;
                        case LT_Arceus: setState_runCommand(C_TalkArceus); break;
                        case LT_Darkrai: setState_runCommand(C_TalkDarkrai); break;
                        case LT_Others: setState_runCommand(C_Talk); break;
                        default: setState_error("Invalid legendary type"); break;
                    }
                    m_videoManager->clearCaptures();
                }
                else
                {
                    setState_runCommand("ASpam,30,Nothing,20");
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

            m_videoManager->setAreas({A_Dialog});
            m_dialogWasFound = false;
            m_elapsedTimer.restart();
        }
        break;
    }
    case SS_Detect1: // Legendary appeared!
    case SS_Detect2: // Go! <POKEMON>!
    {
        if (state == S_CaptureReady)
        {
            double elapsed = m_elapsedTimer.elapsed();
            if (elapsed > 15000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect dialog for too long, restarting game...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();

                m_videoManager->clearCaptures();
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
                            QImage frame;
                            m_videoManager->getFrame(frame);
                            sendDiscordMessage("Shiny Found!", true, QColor(255,255,0), &frame);

                            // shiny found!
                            m_substage = SS_Finish;
                            setState_runCommand(C_Capture);
                            emit printLog(str + "SHINY FOUND!", LOG_SUCCESS);
                        }
                        else if (m_type == LT_Shaymin)
                        {
                            // for Shaymin, just run from battle
                            emit printLog(str + "not shiny, running from battle...");
                            m_elapsedTimer.restart();
                            m_substage = SS_DetectBattle;
                            setState_frameAnalyzeRequest();

                            m_videoManager->setAreas({A_Battle});
                            break;
                        }
                        else
                        {
                            // reset
                            m_substage = SS_Restart;
                            runRestartCommand();
                            emit printLog(str + "not shiny, restarting...");
                        }

                        m_videoManager->clearCaptures();
                        break;
                    }
                }
                m_dialogWasFound = foundDialog;
            }
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_DetectBattle:
    {
        if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 15000)
            {
                incrementStat(m_error);
                emit printLog("Unable to detect battle UI for too long, restarting game...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();

                m_videoManager->clearCaptures();
            }
            else if (checkBrightnessMeanTarget(A_Battle.m_rect, C_Color_Battle, 160))
            {
                // Walk down and up to respawn shaymin
                emit printLog("Battle UI detected, respawning legendary...");
                m_substage = SS_RestartShaymin;
                setState_runCommand(C_RestartShaymin);

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_RestartShaymin:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Talk;
            setState_runCommand(C_TalkShaymin);
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
    setState_runCommand(m_settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
}
