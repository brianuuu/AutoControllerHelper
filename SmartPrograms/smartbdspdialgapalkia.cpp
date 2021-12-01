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
    m_encounter = 0;
    m_noShinyTimer = 0.0;
}

void SmartBDSPDialgaPalkia::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Restart;
        setState_runCommand(C_Restart);

        m_parameters.vlcWrapper->clearCaptures();
        m_parameters.vlcWrapper->setAreas({A_Battle, A_Pokemon});

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
                m_encounter++;
                m_parameters.settings->setStreamCounterCount(m_encounter);
                emit printLog("Current Encounter: " + QString::number(m_encounter));

                if (m_noShinyTimer == 0.0)
                {
                    m_noShinyTimer = m_elapsedTimer.elapsed();
                    emit printLog("Calibrated time = " + QString::number(m_noShinyTimer) + "ms");

                    m_substage = SS_Restart;
                    setState_runCommand(C_Restart);
                }
                else
                {
                    double elapsed = m_elapsedTimer.elapsed();
                    if (elapsed > m_noShinyTimer + 1000)
                    {
                        // shiny found!
                        m_substage = SS_Finish;
                        setState_runCommand(C_Capture);
                        emit printLog("Time taken: " + QString::number(elapsed) + "ms, SHINY FOUND!", LOG_SUCCESS);
                    }
                    else
                    {
                        // update timer in case delay has shifted
                        m_noShinyTimer = elapsed;

                        // reset
                        m_substage = SS_Restart;
                        setState_runCommand(C_Restart);
                        emit printLog("Time taken: " + QString::number(elapsed) + "ms, not shiny, restarting...");
                    }
                }
            }
            else if (m_noShinyTimer > 0.0 && m_elapsedTimer.elapsed() > m_noShinyTimer + 10000)
            {
                emit printLog("Unable to detect battle UI for too long, restarting sequence...", LOG_ERROR);
                m_substage = SS_Restart;
                setState_runCommand(C_Restart);
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
