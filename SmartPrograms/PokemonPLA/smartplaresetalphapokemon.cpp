#include "smartplaresetalphapokemon.h"

SmartPLAResetAlphaPokemon::SmartPLAResetAlphaPokemon
(
    AlphaType type,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_type(type)
{
    init();

    if (m_type >= AT_COUNT)
    {
        setState_error("Invalid alpha pokemon type!");
    }
}

void SmartPLAResetAlphaPokemon::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartPLAResetAlphaPokemon::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartPLAResetAlphaPokemon::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statError, "Errors");
        initStat(m_statAttempts, "Attempts");
        initStat(m_statShiny, "Shinies");

        // Setup sound detection
        m_shinySoundID = m_audioManager->addDetection("PokemonLA/ShinySFX", 0.19f, 5000);
        m_shinyDetected = false;
        connect(m_audioManager, &AudioManager::soundDetected, this, &SmartPLAResetAlphaPokemon::soundDetected);

        m_substage = SS_Restart;
        setState_runCommand(C_Restart);
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
                setState_runCommand(C_Restart);
            }
            else
            {
                setState_frameAnalyzeRequest();

                m_videoManager->clearCaptures();
                m_videoManager->setAreas({A_Title});

                m_substage = SS_Title;
            }
        }
        break;
    }
    case SS_Title:
    case SS_GameStart:
    case SS_EnterCave:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage != SS_EnterCave)
                {
                    emit printLog((m_substage == SS_Title) ? "Title detected!" : "Entering Cave/Temple...");
                    setState_runCommand("ASpam,60,Nothing,30");
                    m_substage = (m_substage == SS_Title) ? SS_GameStart : SS_EnterCave;
                }
                else
                {
                    incrementStat(m_statAttempts);
                    m_substage = SS_Walk;
                    setState_runCommand(m_type == AT_Gallade ? C_WalkGallade : C_WalkCrobat);

                    m_audioManager->startDetection(m_shinySoundID);
                    m_videoManager->clearCaptures();
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Walk:
    {
        if (m_shinyDetected)
        {
            emit printLog("SHINY POKEMON FOUND!", LOG_SUCCESS);
            m_substage = SS_Capture;
            setState_runCommand("Capture,22,Minus,1");
        }
        else if (state == S_CommandFinished)
        {
            emit printLog("No shiny found, restarting...", LOG_WARNING);
            m_substage = SS_Restart;
            setState_runCommand(C_Restart);

            m_audioManager->stopDetection(m_shinySoundID);
            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_Capture:
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

void SmartPLAResetAlphaPokemon::soundDetected(int id)
{
    if (id != m_shinySoundID) return;

    if (m_substage == SS_Walk)
    {
        incrementStat(m_statShiny);
        m_shinyDetected = true;
        runNextStateContinue();
    }
    else
    {
        incrementStat(m_statError);
        emit printLog("Shiny sound detected at unexpected state, false positive?", LOG_ERROR);
    }
}
