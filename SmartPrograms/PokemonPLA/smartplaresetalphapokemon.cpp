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
    setImageMatchFromResource("PLA_Shiny", m_imageMatch_Shiny);

    m_shinyFound = false;
    m_alphaFound = false;
    m_enteredFirstPerson = false;
    m_locateAlphaAttempt = 0;
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
        initStat(m_statAlphaFound, "Alpha Found");

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

                    m_shinyFound = false;
                    m_alphaFound = false;
                    m_enteredFirstPerson = false;
                    m_locateAlphaAttempt = 0;

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
    case SS_FindAlpha:
    {
        if (state == S_CommandFinished)
        {
            if (m_shinyFound)
            {
                m_substage = SS_Capture;
                setState_runCommand("Capture,22,Minus,1");
            }
            else if (!m_alphaFound && m_locateAlphaAttempt < 5)
            {
                // Retry
                if (m_substage == SS_Walk)
                {
                    m_substage = SS_FindAlpha;
                    m_videoManager->setAreas({A_Alpha, A_UI, A_Shiny});
                }

                m_locateAlphaAttempt++;
                setState_runCommand("ZL,20,LUp,20", true);

                m_shinyFound = false;
                m_alphaFound = false;
                m_enteredFirstPerson = false;
            }
            else
            {
                if (m_locateAlphaAttempt >= 5)
                {
                    incrementStat(m_statError);
                    emit printLog("Unable to locate Alpha Pokemon after 5 attempts, restarting...", LOG_ERROR);
                }
                else
                {
                    emit printLog("No shiny found, restarting...", LOG_WARNING);
                }
                m_substage = SS_Restart;
                setState_runCommand(C_Restart);

                m_videoManager->clearCaptures();
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_enteredFirstPerson || m_shinyFound) break;

            if (!checkBrightnessMeanTarget(A_UI.m_rect, C_Color_UI, 150))
            {
                emit printLog("Pokemon not located, retrying...");
                m_enteredFirstPerson = true;
            }
            else if (!m_alphaFound && checkBrightnessMeanTarget(A_Alpha.m_rect, C_Color_Alpha, 70))
            {
                // Alpha pokemon might be found, check if it's shiny
                incrementStat(m_statAlphaFound);
                m_alphaFound = true;

                emit printLog("Alpha pokemon (" + QString::number(m_statAlphaFound.first) + ") located, checking shininess");
                setState_frameAnalyzeRequest();
            }
            else if (m_alphaFound && checkImageMatchTarget(A_Shiny.m_rect, C_Color_Shiny, m_imageMatch_Shiny, 0.3))
            {
                emit printLog("SHINY ALPHA FOUND!", LOG_SUCCESS);
                m_shinyFound = true;
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
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
