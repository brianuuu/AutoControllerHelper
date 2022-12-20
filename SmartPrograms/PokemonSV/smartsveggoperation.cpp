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
    m_substageAfterMainMenu = SS_Finished;
    m_commandAfterMainMenu = "Nothing,10";
    m_isToBoxAfterMainMenu = false;
    m_sandwichCount = 0;
    m_eggsCollected = 0;
    m_eggCollectCount = 0;
    m_eggCollectDialog = false;
    m_eggCollectDetected = false;
    m_eggColumnsHatched = 0;
    m_eggsToHatch = 0;
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

        //runToBoxCommand();
        //m_eggColumnsHatched = 1;

        //m_substage = SS_ToBox;
        //setState_runCommand("ASpam,10,Nothing,20");

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
            m_eggColumnsHatched = 0;
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                runRestartCommand("Unable to detect black screen after restarting, attempting restart again...");
            }
            else if (checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_Title;
                setState_frameAnalyzeRequest();
                m_timer.restart();
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
        if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                runRestartCommand("Unable to detect title screen, attempting restart again...");
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                emit printLog("Title detected!");
                m_substage = SS_GameStart;
                setState_runCommand("ASpam,310");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_GameStart:
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
                runRestartCommand("Unable to detect game start, attempting restart again...");
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_programSettings.m_operation == EOT_Hatcher)
                {
                    if (m_programSettings.m_isHatchWithSandwich)
                    {
                        runPicnicCommand();
                    }
                    else
                    {
                        // go up to lighthouse immediately
                        emit printLog("Heading up to Poco Path lighthouse");
                        m_substage = SS_PackUp;
                        setState_runCommand("LDown,40");
                        m_videoManager->clearCaptures();
                    }
                }
                else
                {
                    runPicnicCommand();
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_MainMenu:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_PartyFirst,A_Box,A_Picnic});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                runRestartCommand("Unable to detect main menu, forcing restart", m_programSettings.m_isErrorCapture);
            }
            else
            {
                if (checkBrightnessMeanTarget(A_PartyFirst.m_rect, C_Color_Yellow, 200))
                {
                    m_substage = m_substageAfterMainMenu;
                    QString command = m_isToBoxAfterMainMenu ? "LRight,3,DDown,3,Loop,1," : "LRight,3,DDown,3,LDown,3,Loop,1,";
                    setState_runCommand(command + m_commandAfterMainMenu);
                    m_videoManager->clearCaptures();
                }
                else if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Yellow, 200))
                {
                    m_substage = m_substageAfterMainMenu;
                    QString command = m_isToBoxAfterMainMenu ? "" : "LDown,3,Loop,1,";
                    setState_runCommand(command + m_commandAfterMainMenu);
                    m_videoManager->clearCaptures();
                }
                else if (checkBrightnessMeanTarget(A_Picnic.m_rect, C_Color_Yellow, 200))
                {
                    m_substage = m_substageAfterMainMenu;
                    QString command = m_isToBoxAfterMainMenu ? "LUp,3,Loop,1," : "";
                    setState_runCommand(command + m_commandAfterMainMenu);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    setState_frameAnalyzeRequest();
                }
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
                if (m_programSettings.m_operation == EOT_Collector && m_sandwichCount > 0)
                {
                    incrementStat(m_statError);
                    setState_error("Unable to detect sandwich menu, but there are already eggs collected, stopping program");
                }
                else
                {
                    runRestartCommand("Unable to detect sandwich menu, forcing restart", m_programSettings.m_isErrorCapture);
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
            emit printLog("Making the Great Peanut Butter Sandwich");
            m_substage = SS_MakeSandwich;
            setState_runCommand(C_Sandwich);

            m_sandwichCount++;
        }
        break;
    }
    case SS_MakeSandwich:
    {
        if (state == S_CommandFinished)
        {
            m_sandwichTimer.restart();
            if (m_programSettings.m_operation == EOT_Hatcher)
            {
                emit printLog("Heading up to Poco Path lighthouse");
                m_substage = SS_PackUp;
                setState_runCommand(C_PackUp);
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
                if (m_programSettings.m_operation == EOT_Collector && m_sandwichCount > 0)
                {
                    incrementStat(m_statError);
                    setState_error("Unable to talk to egg basket, but there are already eggs collected, stopping program");
                }
                else
                {
                    runRestartCommand("Unable to talk to egg basket, forcing restart", m_programSettings.m_isErrorCapture);
                }
            }
            else if (m_eggCollectCount >= 15)
            {
                // TODO: error if no eggs collected
                emit printLog("Total eggs collected: " + QString::number(m_eggsCollected));
                if (m_programSettings.m_operation == EOT_Collector)
                {
                    if (m_sandwichCount < m_programSettings.m_sandwichCount)
                    {
                        // make more sandwich!
                        m_substage = SS_Picnic;
                        setState_runCommand("LUp,5,ASpam,40");
                        m_videoManager->clearCaptures();
                    }
                    else
                    {
                        // completed
                        m_substage = SS_Finished;
                        setState_runCommand("Home,2,Nothing,20");
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
    case SS_PackUp:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                runRestartCommand("Unable to detect screen fade to black when going on Poco Path lighthouse, forcing restart", m_programSettings.m_isErrorCapture);
            }
            else if (checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_HatchInit;
                setState_runCommand(C_HatchInit);
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_HatchInit:
    {
        if (state == S_CommandFinished)
        {
            runToBoxCommand();
        }
        break;
    }
    case SS_ToBox:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({GetBoxCaptureAreaOfPos(1,1)});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                runRestartCommand("Unable to detect being in Box menu, forcing restart", m_programSettings.m_isErrorCapture);
            }
            else if (checkBrightnessMeanTarget(GetBoxCaptureAreaOfPos(1,1).m_rect, C_Color_Yellow, 130))
            {
                if (m_eggColumnsHatched > 0)
                {
                    m_substage = SS_CheckShiny;
                    setState_runCommand("LLeft,3,DDown,3,Minus,3,Nothing,20");
                    m_checkShinyCount = 0;
                    m_videoManager->setAreas({A_Shiny});
                }
                else
                {
                    m_eggsToHatch = 5;
                    m_substage = SS_CheckHasEgg;
                    setState_runCommand(m_commands[C_MultiSelect] + ",LLeft,3,LDown,3,A,3,Nothing,3,Loop,1,LDown,3,DDown,3,Loop,4,Nothing,20");
                    m_videoManager->setAreas(
                                {
                                    A_Pokemon,
                                    GetPartyCaptureAreaOfPos(2),
                                    GetPartyCaptureAreaOfPos(3),
                                    GetPartyCaptureAreaOfPos(4),
                                    GetPartyCaptureAreaOfPos(5),
                                    GetPartyCaptureAreaOfPos(6)
                                });
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CheckHasEgg:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(GetPartyCaptureAreaOfPos(m_eggsToHatch + 1).m_rect, C_Color_Yellow, 200))
            {
                if (checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Yellow, 200))
                {
                    emit printLog("Hatching column " + QString::number(m_eggColumnsHatched + 1) + " with " + QString::number(m_eggsToHatch) + " eggs");
                    m_substage = SS_ExitBox;
                    setState_runCommand("BSpam,10");
                }
                else if (m_eggsToHatch <= 1)
                {
                    runRestartCommand("Current party has no eggs, something went really wrong", m_programSettings.m_isErrorCapture);
                }
                else
                {
                    // current party position doesn't have an egg
                    m_eggsToHatch--;
                    setState_runCommand("DUp,3,Nothing,20");
                }
            }
            else
            {
                runRestartCommand("Unable to detect cursor on current party, forcing restart", m_programSettings.m_isErrorCapture);
            }
        }
        break;
    }
    case SS_ExitBox:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Box});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                runRestartCommand("Unable to detect exiting Box, forcing restart", m_programSettings.m_isErrorCapture);
            }
            else if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Yellow, 200))
            {
                // run right for 3 minutes max
                m_substage = SS_HatchEggs;
                setState_runCommand("B,20,LRight,3750",true);
                m_videoManager->setAreas({A_Health,A_Battle,A_Dialog});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_HatchEggs:
    {
        if (state == S_CommandFinished)
        {
            runRestartCommand("Did not hatch any eggs for 3 minutes, forcing restart");
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Battle.m_rect, C_Color_Yellow, 180) && checkBrightnessMeanTarget(A_Health.m_rect, C_Color_Green, 200))
            {
                // go into a battle
                emit printLog("Encountered wild pokemon, running", LOG_WARNING);
                m_substage = SS_Battle;
                setState_runCommand("DUp,3,ASpam,100");
                m_videoManager->clearCaptures();
            }
            else if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 220))
            {
                --m_eggsToHatch;
                incrementStat(m_statEggHatched);
                emit printLog("Oh? Egg is hatching! (" + QString::number(m_eggsToHatch) + " remaining)");
                m_substage = SS_Hatching;
                setState_runCommand("ASpam,450");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Battle:
    case SS_Hatching:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggsToHatch <= 0)
            {
                m_eggColumnsHatched++;
                runToBoxCommand();
                // TODO: check sandwich running out of time
            }
            else
            {
                m_substage = SS_HatchEggs;
                setState_runCommand("LRight,3750",true);
                m_videoManager->setAreas({A_Health,A_Battle,A_Dialog});
            }
        }
        break;
    }
    case SS_CheckShiny:
    {
        if (state == S_CommandFinished)
        {
            m_checkShinyCount++;
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            bool isShiny = false;
            if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                // SHINY FOUND!
                isShiny = true;
                incrementStat(m_statShinyHatched);
                emit printLog("Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(m_checkShinyCount) + " is SHINY!!!", LOG_SUCCESS);
            }

            if (!isShiny)
            {
                emit printLog("Not shiny");
            }

            if (m_checkShinyCount >= 5)
            {
                // m_eggColumnsHatched = [1,6]
                QString command = "A,3,Nothing,3,LUp,3,Loop,1,LRight,3,Nothing,3,Loop," + QString::number(((m_eggColumnsHatched - 1) % 6) + 1) + ",A,3,Nothing,3";
                if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
                {
                    // TODO: shiny mode
                    // we are done
                    m_substage = SS_Finished;
                    setState_runCommand(command + ",Home,2");
                }
                else
                {
                    // go to next column
                    m_substage = SS_NextColumn;
                    if (m_eggColumnsHatched % 6 == 0)
                    {
                        // next box
                        command += ",LUp,3,DRight,3,LDown,3,DRight,3";
                    }
                    command += ",LRight,3";
                    setState_runCommand(command);
                }
            }
            else
            {
                // current party position is not shiny
                setState_runCommand("DDown,3,Nothing,20");
            }
        }
        break;
    }
    case SS_NextColumn:
    {
        if (state == S_CommandFinished)
        {
            m_eggsToHatch = 5;
            m_substage = SS_CheckHasEgg;
            setState_runCommand(m_commands[C_MultiSelect] + ",Loop,1,LLeft,3,Nothing,3,Loop," + QString::number((m_eggColumnsHatched % 6) + 1) + ",LDown,3,A,3,Nothing,3,Loop,1,LDown,3,DDown,3,Loop,4,Nothing,20");
            m_videoManager->setAreas(
                        {
                            A_Pokemon,
                            GetPartyCaptureAreaOfPos(2),
                            GetPartyCaptureAreaOfPos(3),
                            GetPartyCaptureAreaOfPos(4),
                            GetPartyCaptureAreaOfPos(5),
                            GetPartyCaptureAreaOfPos(6)
                        });
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

void SmartSVEggOperation::runRestartCommand(QString error, bool capture)
{
    incrementStat(m_statError);
    if (!error.isEmpty())
    {
        emit printLog(error, LOG_ERROR);
    }
    m_substage = SS_Restart;
    m_videoManager->clearCaptures();
    if (capture)
    {
        setState_runCommand("Capture,22,Nothing,200," + m_commands[C_Restart]);
    }
    else
    {

        setState_runCommand(C_Restart);
    }
}

void SmartSVEggOperation::runPicnicCommand()
{
    // go picnic!
    m_substage = SS_MainMenu;
    setState_runCommand("Nothing,30,L,3,X,3");
    m_videoManager->clearCaptures();

    // make sandwich before hatching
    m_substageAfterMainMenu = SS_Picnic;
    m_commandAfterMainMenu = m_commands[C_Picnic];
    m_isToBoxAfterMainMenu = false;
}

void SmartSVEggOperation::runToBoxCommand()
{
    m_substage = SS_MainMenu;
    setState_runCommand("X,3");
    m_videoManager->clearCaptures();

    // make sandwich before hatching
    m_substageAfterMainMenu = SS_ToBox;
    m_commandAfterMainMenu = "ASpam,10,Nothing,20";
    m_isToBoxAfterMainMenu = true;
}

const CaptureArea SmartSVEggOperation::GetBoxCaptureAreaOfPos(int x, int y)
{
    // return capture area of location in box
    if (x < 1 || y < 1 || x > 6 || y > 5)
    {
        x = 1;
        y = 1;
    }

    x--;
    y--;
    return CaptureArea(305 + x * 84, 139 + y * 84, 70, 70);
}

const CaptureArea SmartSVEggOperation::GetPartyCaptureAreaOfPos(int y)
{
    if (y < 1 || y > 6)
    {
        y = 1;
    }

    y--;
    return CaptureArea(36, 142 + y * 84, 100, 30);
}
