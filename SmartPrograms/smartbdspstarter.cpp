#include "smartbdspstarter.h"

SmartBDSPStarter::SmartBDSPStarter
(
    int starterIndex,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_starterIndex(starterIndex)
{
    init();
}

void SmartBDSPStarter::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPStarter::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_encounter = 0;
    m_dialogWasFound = false;
}

void SmartBDSPStarter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Restart;
        runRestartCommand();

        if (m_parameters.settings->isStreamCounterEnabled())
        {
            m_encounter = m_parameters.settings->getStreamCounterCount();
            emit printLog("Stream Counter: ON, encounter starting at " + QString::number(m_encounter));
        }
        else
        {
            emit printLog("Stream Counter: OFF");
        }
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
        // Now pick the starter
        if (state == S_CommandFinished)
        {
            m_substage = SS_Select;
            switch (m_starterIndex)
            {
            case 1: setState_runCommand("LRight,1"); break;
            case 2: setState_runCommand("LLeft,1"); break;
            default: setState_runCommand("Nothing,1"); break;
            }
            emit printLog("Picking starter and starting battle...");
        }
        break;
    }
    case SS_Select:
    {
        // Confirm choosing starter
        if (state == S_CommandFinished)
        {
            m_substage = SS_Choose;
            setState_runCommand(C_Choose);
        }
        break;
    }
    case SS_Choose:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_substage = SS_Detect1;

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Dialog, A_DialogFalse});

            m_dialogWasFound = false;
            m_elapsedTimer.restart();
        }
        break;
    }

    case SS_Detect1: // wild starly appeared
    case SS_Detect2: // Go! Starter!
    case SS_Detect3: // Battle UI
    {
        if (state == S_CaptureReady)
        {
            double elapsed = m_elapsedTimer.elapsed();
            if (elapsed > 15000)
            {
                emit printLog("Unable to detect dialog or battle UI for too long, restarting sequence...", LOG_ERROR);
                m_substage = SS_Restart;
                runRestartCommand();

                break;
            }
            else if (m_substage == SS_Detect1 || m_substage == SS_Detect2)
            {
                bool foundDialog = checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 230) && !checkBrightnessMeanTarget(A_DialogFalse.m_rect, C_Color_Dialog, 100);
                if (m_dialogWasFound && !foundDialog)
                {
                    // dialog closed, goto next state
                    m_elapsedTimer.restart();

                    if (m_substage == SS_Detect1)
                    {
                        m_substage = SS_Detect2;
                    }
                    else
                    {
                        m_parameters.vlcWrapper->clearCaptures();
                        m_parameters.vlcWrapper->setAreas({A_Battle});
                        m_substage = SS_Detect3;
                    }
                }
                else if (!m_dialogWasFound && foundDialog)
                {
                    if (m_substage == SS_Detect1)
                    {
                        emit printLog("\"You encountered wild Starly!\" dialog detected");
                    }
                    else
                    {
                        emit printLog("\"Go! Starter!\" dialog detected, time taken: " + QString::number(elapsed) + "ms");
                        if (elapsed > 2000)
                        {
                            emit printLog("A SHINY Starly was found.....", LOG_WARNING);
                        }
                    }
                }

                m_dialogWasFound = foundDialog;
            }
            else if (m_substage == SS_Detect3)
            {
                if (checkBrightnessMeanTarget(A_Battle.m_rect, C_Color_Battle, 160))
                {
                    // increment counter
                    m_encounter++;
                    m_parameters.settings->setStreamCounterCount(m_encounter);
                    emit printLog("Current Encounter: " + QString::number(m_encounter));

                    if (elapsed > 2000)
                    {
                        // shiny found!
                        m_substage = SS_Finish;
                        setState_runCommand(C_Capture);
                        emit printLog("Battle UI detected, time taken: " + QString::number(elapsed) + "ms, SHINY FOUND!", LOG_SUCCESS);
                    }
                    else
                    {
                        // reset
                        m_substage = SS_Restart;
                        runRestartCommand();
                        emit printLog("Battle UI detected, time taken: " + QString::number(elapsed) + "ms, not shiny, restarting...");
                    }

                    m_parameters.vlcWrapper->clearCaptures();
                    break;
                }
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

void SmartBDSPStarter::runRestartCommand()
{
    setState_runCommand(m_parameters.settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);
}
