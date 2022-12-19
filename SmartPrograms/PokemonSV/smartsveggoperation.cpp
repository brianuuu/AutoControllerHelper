#include "smartsveggoperation.h"

SmartSVEggOperation::SmartSVEggOperation
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(setting)
{
    init();
}

void SmartSVEggOperation::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSVEggOperation::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_sandwichCount = 0;
    m_eggsCollected = 0;
    m_eggCollectCount = 0;
    m_eggCollectDialog = false;
    m_eggCollectDetected = false;
}

void SmartSVEggOperation::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statError, "Errors");
        initStat(m_statEggCollected, "Eggs Collected");
        initStat(m_statEggHatched, "Eggs Hatched");
        initStat(m_statShinyHatched, "Shinies");

        m_substage = SS_Restart;
        setState_runCommand(C_Restart);
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();

            m_sandwichCount = 0;
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect title screen, attempting restart again...", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);
            }
            else if (checkAverageColorMatch(A_Title.m_rect, QColor(253,253,253)))
            {
                emit printLog("Title detected!");
                m_substage = SS_Title;
                setState_runCommand("ASpam,270");

                m_videoManager->clearCaptures();
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
                incrementStat(m_statError);
                emit printLog("Unable to detect game start, attempting restart again...", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_programSettings.m_operation == EOT_Hatcher)
                {
                    // TODO:
                    setState_error("TODO");
                }
                else
                {
                    m_substage = SS_Picnic;
                    setState_runCommand(C_Picnic);
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
    case SS_Picnic:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Sandwich});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                incrementStat(m_statError);
                if (m_programSettings.m_operation == EOT_Collector && m_sandwichCount > 0)
                {
                    setState_error("Unable to detect sandwich menu, but there are already eggs collected, stopping program");
                }
                else
                {
                    emit printLog("Unable to detect sandwich menu, forcing restart", LOG_ERROR);
                    m_substage = SS_Restart;
                    setState_runCommand(C_Restart);
                }
            }
            else if (checkAverageColorMatch(A_Sandwich.m_rect, C_Color_Sandwich))
            {
                QString command = m_programSettings.m_sandwichPosX > 0 ? "DRight,2,Nothing,3,Loop,1" : "Nothing,2,Loop,1";
                if (m_programSettings.m_sandwichPosY > 0)
                {
                    command += ",DDown,2,Nothing,3,Loop," + QString::number(m_programSettings.m_sandwichPosY);
                }

                m_substage = SS_ToSandwich;
                setState_runCommand(command);
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ToSandwich:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Making the Great Peanut Butter Sandwich...");
            m_substage = SS_MakeSandwich;
            setState_runCommand(C_Sandwich);

            if (m_programSettings.m_operation == EOT_Collector)
            {
                m_sandwichCount++;
            }
        }
        break;
    }
    case SS_MakeSandwich:
    {
        if (state == S_CommandFinished)
        {
            if (m_programSettings.m_operation == EOT_Hatcher)
            {
                // TODO:
                setState_error("TODO");
            }
            else
            {
                m_substage = SS_CollectEggs;
                m_eggCollectCount = 0;
                m_eggCollectDialog = false;
                m_eggCollectDetected = false;
                m_timer.restart();
                m_videoManager->setAreas({A_Dialog,A_Yes});
                runNextStateContinue();
            }
        }
        break;
    }
    case SS_CollectEggs:
    {
        if (state == S_CommandFinished)
        {
            qint64 timeElapsedExpected = (m_eggCollectCount + 1) * 120000;
            qint64 timeElapsed = m_timer.elapsed();

            if (m_eggCollectCount > 0 && !m_eggCollectDialog)
            {
                incrementStat(m_statError);
                if (m_programSettings.m_operation == EOT_Collector && m_sandwichCount > 0)
                {
                    setState_error("Unable to talk to egg basket, but there are already eggs collected, stopping program");
                }
                else
                {
                    emit printLog("Unable to talk to egg basket, forcing restart", LOG_ERROR);
                    m_substage = SS_Restart;
                    setState_runCommand(C_Restart);
                }
            }
            else if (m_eggCollectCount >= 15)
            {
                emit printLog("Total eggs collected: " + QString::number(m_eggsCollected));
                if (m_programSettings.m_operation == EOT_Collector)
                {
                    if (m_sandwichCount < m_programSettings.m_sandwichCount)
                    {
                        // make more sandwich!
                        m_substage = SS_Picnic;
                        setState_runCommand("LUp,8,ASpam,40");
                        m_videoManager->clearCaptures();
                    }
                    else
                    {
                        // completed
                        setState_completed();
                    }
                }
                else
                {
                    // TODO:
                    setState_completed();
                }
            }
            else if (timeElapsed < timeElapsedExpected)
            {
                emit printLog("Waiting until " + QString::number(timeElapsedExpected / 60000) + " minutes (Eggs collected: " + QString::number(m_eggsCollected) + ")");
                runNextStateDelay(int(timeElapsedExpected - timeElapsed + 500));
            }
            else
            {
                setState_runCommand("ASpam,280,BSpam,60", true);
                m_eggCollectCount++;
            }
        }
        else if (state == S_CaptureReady)
        {
            // check if we can talk to egg basket
            if (!m_eggCollectDialog && checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 220))
            {
                emit printLog("Successfully talked to egg basket!");
                m_eggCollectDialog = true;
                m_videoManager->setAreas({A_Yes});
            }

            // detect how many eggs collected
            bool isYesDetected = checkBrightnessMeanTarget(A_Yes.m_rect, C_Color_Yellow, 200);
            if (!m_eggCollectDetected && isYesDetected)
            {
                incrementStat(m_statEggCollected);
                m_eggsCollected++;
                m_eggCollectDetected = true;
            }
            else if (m_eggCollectDetected && !isYesDetected)
            {
                m_eggCollectDetected = false;
            }

            setState_frameAnalyzeRequest();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
