#include "SmartPLZARespawnReset.h"

SmartPLZARespawnReset::SmartPLZARespawnReset(SmartProgramParameter parameter) : SmartProgramBase(parameter)
{
    init();
}

void SmartPLZARespawnReset::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartPLZARespawnReset::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartPLZARespawnReset::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statAttempts, "Attempts");
        initStat(m_statShiny, "Shinies");
        initStat(m_statError, "Errors");

        // Setup sound detection
        m_shinySoundID = m_audioManager->addDetection("PokemonLA/ShinySFX", 0.19f, 5000);
        m_shinyDetected = false;
        connect(m_audioManager, &AudioManager::soundDetected, this, &SmartPLZARespawnReset::soundDetected);

        runRestartCommand();
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect black screen on restart, HOME button might have missed, retrying...", LOG_ERROR);
                runRestartCommand();
            }
            else
            {
                m_elapsedTimer.restart();
                setState_frameAnalyzeRequest();
                m_videoManager->setAreas({A_Title});

                m_substage = SS_Title;
            }
        }
        break;
    }
    case SS_Title:
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            m_elapsedTimer.restart();
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 30000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect title screen or game starting for too long, the game might have crashed, restarting...", LOG_ERROR);
                runRestartCommand();
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_Title)
                {
                    emit printLog("Title detected!");
                    setState_runCommand("ASpam,20,Nothing,30");
                    m_substage = SS_GameStart;
                }
                else
                {
                    incrementStat(m_statAttempts);
                    emit printLog("Listening for shiny sound...");
                    m_substage = SS_DetectShiny;
                    setState_runCommand("Nothing,100");

                    m_videoManager->clearCaptures();
                    m_audioManager->startDetection(m_shinySoundID);
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_DetectShiny:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("No shiny found, restarting...", LOG_WARNING);
            runRestartCommand();

            m_audioManager->stopDetection(m_shinySoundID);
        }
        break;
    }
    case SS_Capture:
    {
        if (m_shinyDetected)
        {
            setState_runCommand("Capture,22,Plus,1,Nothing,20,Home,1");
            m_shinyDetected = false;
        }
        else if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartPLZARespawnReset::runRestartCommand()
{
    m_substage = SS_Restart;
    setState_runCommand(C_Restart);
}

void SmartPLZARespawnReset::soundDetected(int id)
{
    if (id != m_shinySoundID) return;

    if (m_substage == SS_DetectShiny)
    {
        incrementStat(m_statShiny);
        m_shinyDetected = true;

        QImage frame;
        m_videoManager->getFrame(frame);
        sendDiscordMessage("Shiny Sound Detected!", true, QColor(255,255,0), &frame);

        emit printLog("SHINY POKEMON FOUND!", LOG_SUCCESS);
        m_substage = SS_Capture;
        runNextStateContinue();
    }
}
