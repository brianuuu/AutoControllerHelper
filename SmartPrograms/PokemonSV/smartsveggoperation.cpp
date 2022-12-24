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

    m_missedInputRetryCount = 0;

    m_sandwichCount = 0;
    m_eggsCollected = 0;
    m_eggCollectCount = 0;
    m_eggCollectDialog = false;
    m_eggCollectDetected = false;

    m_eggColumnsHatched = 0;
    m_eggsToHatch = 0;
    m_eggsHatched = 0;
    m_checkShinyCount = 0;
    m_flyAttempts = 0;
    m_flySuccess = true;
    m_shinyPositions.clear();

    m_shinySoundID = -1;
    m_shinySoundDetected = false;
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

        m_substage = SS_Restart;
        setState_runCommand(C_Restart);

        m_shinySoundDetected = false;
        if (m_programSettings.m_isShinyDetection)
        {
            // Setup sound detection
            m_shinySoundID = m_audioManager->addDetection("PokemonSV/ShinySFXHatch", 0.2f, 5000);
            connect(m_audioManager, &AudioManager::soundDetected, this, &SmartSVEggOperation::soundDetected);
        }
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();

            m_missedInputRetryCount = 0;
            m_sandwichCount = 0;
            m_eggsCollected = 0;
            m_eggsRejected = 0;
            m_eggColumnsHatched = 0;
            m_eggsHatched = 0;
            m_shinyPositions.clear();
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
                        emit printLog("Heading up to Poco Path Lighthouse");
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
                    emit printLog("Unable to detect sandwich menu, but there are already eggs collected, stopping program", LOG_ERROR);
                    m_substage = SS_Finished;
                    setState_runCommand("Home,2,Nothing,20");
                }
                else
                {
                    runRestartCommand("Unable to detect sandwich menu, forcing restart", m_programSettings.m_isErrorCapture);
                }
            }
            else if (checkBrightnessMeanTarget(A_Sandwich.m_rect, C_Color_Sandwich, 220))
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
            if (m_programSettings.m_operation == EOT_Hatcher
            || (m_programSettings.m_operation == EOT_Shiny && m_sandwichCount >= 2))
            {
                // if shiny mode has made 2 sandwiches, it must start hatching eggs
                m_sandwichCount = qMax(m_sandwichCount, 10);

                emit printLog("Heading up to Poco Path Lighthouse");
                m_substage = SS_PackUp;
                setState_runCommand(C_PackUp);
            }
            else
            {
                m_substage = SS_CollectEggs;
                m_eggCollectCount = 0;
                m_eggCollectDialog = false;
                m_eggCollectDetected = false;
                m_eggRejectDetected = false;
                m_eggsRejected = 0;
                m_timer.restart();
                m_videoManager->setAreas({A_Dialog,A_Yes,A_No});
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
                    emit printLog("Unable to talk to egg basket, but there are already eggs collected, stopping program", LOG_ERROR);
                    m_substage = SS_Finished;
                    setState_runCommand("Home,2,Nothing,20");
                }
                else
                {
                    runRestartCommand("Unable to talk to egg basket, forcing restart", m_programSettings.m_isErrorCapture);
                }
            }
            else if (m_eggCollectCount >= 15)
            {
                if (m_eggsCollected == 0)
                {
                    // no eggs? most likely color issue with Yes/No box, don't restart
                    emit printLog("No eggs has been collected, something went really wrong", LOG_ERROR);
                    m_substage = SS_Finished;
                    setState_runCommand("Home,2,Nothing,20");
                    break;
                }

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
                    // must be shiny mode
                    // calculate how many columns of eggs are there
                    m_programSettings.m_columnsToHatch = (m_eggsCollected / 30) * 6 + qMin(m_eggsCollected % 30, 6);
                    emit printLog("No. of columns to hatch: " + QString::number(m_programSettings.m_columnsToHatch));

                    if (m_programSettings.m_isHatchWithSandwich && m_sandwichCount == 1)
                    {
                        // make more sandwich!
                        m_substage = SS_Picnic;
                        setState_runCommand("LUp,5,ASpam,40");
                        m_videoManager->clearCaptures();
                    }
                    else
                    {
                        // magic number for swapping team with Flame Body pokemon
                        // but don't set to 10 if it has already done so
                        m_sandwichCount = qMax(m_sandwichCount, 10);

                        // this must be shiny mode
                        emit printLog("Heading up to Poco Path Lighthouse");
                        m_substage = SS_PackUp;
                        setState_runCommand(C_PackUp);
                    }
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
                m_videoManager->setAreas({A_Yes,A_No});
            }

            // detect rejecting egg, but refuse to send to academy
            bool isNoDetected = checkBrightnessMeanTarget(A_Yes.m_rect, C_Color_Yellow, 200);
            if (!m_eggRejectDetected && isNoDetected)
            {
                m_eggsRejected++;
                m_eggRejectDetected = true;

                if (m_eggsRejected % 2 == 0)
                {
                    emit printLog("Two 'No' has been detected when collecting eggs, meaning we got 1 egg instead of 2", LOG_WARNING);
                    incrementStat(m_statEggCollected, -1);
                    m_eggsCollected--;
                }
            }
            else if (m_eggRejectDetected && !isNoDetected)
            {
                 m_eggRejectDetected = false;
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
                runRestartCommand("Unable to detect screen fade to black when going on Poco Path Lighthouse, forcing restart", m_programSettings.m_isErrorCapture);
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

            m_flyAttempts = 0;
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                runRestartCommand("Unable to detect being in Box menu, forcing restart", m_programSettings.m_isErrorCapture);
            }
            else if (checkBrightnessMeanTarget(GetBoxCaptureAreaOfPos(1,1).m_rect, C_Color_Yellow, 130))
            {
                if (m_programSettings.m_operation == EOT_Shiny && m_sandwichCount == 10)
                {
                    // fake more sandwich so we don't repeat this
                    m_sandwichCount++;

                    // swap 2 pokemon in party with flame body pokemon in box
                    runToBoxCommand(m_commands[C_SwapToHatch]);
                    m_videoManager->clearCaptures();
                }
                else if (m_eggColumnsHatched > 0)
                {
                    m_missedInputRetryCount = 0;
                    m_hasPokemonCount = 0;
                    m_substage = SS_CheckShiny;
                    setState_runCommand("LLeft,3,DDown,3,Minus,3,Nothing,20");
                    m_checkShinyCount = 0;
                    m_videoManager->setAreas(
                                {
                                    A_Pokemon,
                                    A_Shiny,
                                    GetPartyCaptureAreaOfPos(2),
                                    GetPartyCaptureAreaOfPos(3),
                                    GetPartyCaptureAreaOfPos(4),
                                    GetPartyCaptureAreaOfPos(5),
                                    GetPartyCaptureAreaOfPos(6)
                                });
                }
                else
                {
                    //m_missedInputRetryCount = 0; // now set in runToBoxCommand()
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
                    m_eggsToHatchColumn = m_eggsToHatch;
                    m_substage = SS_ExitBox;
                    setState_runCommand("B,5,Nothing,5");
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
                if (m_missedInputRetryCount < 3)
                {
                    m_missedInputRetryCount++;
                    if (m_eggsToHatch == 5)
                    {
                        // right after puting eggs to party
                        emit printLog("Unable to detect cursor on current party, input might be missed, exiting Box and retrying...", LOG_WARNING);

                        // don't call runToBoxCommand() so it doesn't reset m_missedInputRetryCount
                        m_substage = SS_MainMenu;
                        setState_runCommand("B,5,Nothing,5,Loop,2");
                        m_videoManager->clearCaptures();

                        m_substageAfterMainMenu = SS_ToBox;
                        m_commandAfterMainMenu = "ASpam,10,Nothing,20";
                        m_isToBoxAfterMainMenu = true;
                    }
                    else
                    {
                        // when checking if there's egg in party
                        emit printLog("Unable to detect cursor on party slot " + QString::number(m_eggsToHatch + 1) + ", input might be missed, retrying...", LOG_WARNING);
                        setState_runCommand("DUp,3,Nothing,20");
                    }
                }
                else
                {
                    runRestartCommand("Unable to detect cursor on current party, forcing restart", m_programSettings.m_isErrorCapture);
                }
            }
        }
        break;
    }
    case SS_ExitBox:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Box, A_Pokemon});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                if (m_missedInputRetryCount < 3 && checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Yellow, 200))
                {
                    m_missedInputRetryCount++;
                    emit printLog("Detected still inside Box, input might be missed when putting eggs to party, exiting Box and retrying...", LOG_WARNING);

                    // don't call runToBoxCommand() so it doesn't reset m_missedInputRetryCount
                    m_substage = SS_MainMenu;
                    setState_runCommand("B,5,Nothing,5");
                    m_videoManager->clearCaptures();

                    m_substageAfterMainMenu = SS_ToBox;
                    m_commandAfterMainMenu = "ASpam,10,Nothing,20";
                    m_isToBoxAfterMainMenu = true;

                    // this can go really wrong if it was able to put eggs to party but just missed B input when exiting Box...
                    // but should be able to just force restart after
                }
                else
                {
                    runRestartCommand("Unable to detect exiting Box, forcing restart", m_programSettings.m_isErrorCapture);
                }
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
                // check screen again after 0.2s in case it was a false positive
                m_substage = SS_ConfirmHatching;
                setState_runCommand("LRight,5");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ConfirmHatching:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 220))
            {
                --m_eggsToHatch;
                m_eggsHatched++;
                incrementStat(m_statEggHatched);
                emit printLog("Oh? Egg is hatching! (" + QString::number(m_eggsToHatch) + " remaining)");
                m_substage = SS_Hatching;
                setState_runCommand("ASpam,450");
                m_videoManager->clearCaptures();

                // sound detection
                if (m_programSettings.m_isShinyDetection)
                {
                    m_audioManager->startDetection(m_shinySoundID);
                }
            }
            else
            {
                // false positive
                emit printLog("Dialog box did not stay as expected, might have been a false positive detection", LOG_WARNING);
                setState_runCommand("LRight,3750", true);
                m_substage = SS_HatchEggs;
            }
        }
        break;
    }
    case SS_Battle:
    case SS_Hatching:
    {
        if (state == S_CommandFinished)
        {
            // sound detection
            if (m_programSettings.m_isShinyDetection && m_audioManager->hasDetection(m_shinySoundID))
            {
                m_audioManager->stopDetection(m_shinySoundID);
            }

            if (m_shinySoundDetected)
            {
                incrementStat(m_statShinyHatched);
                setState_runCommand("Capture,22");
                m_shinySoundDetected = false;

                QPoint pos(m_eggColumnsHatched + 1, m_eggsToHatchColumn - m_eggsToHatch);
                QString log = "Column " + QString::number(pos.x()) + " row " + QString::number(pos.y());
                emit printLog(log + " is SHINY!!! Capturing video!", LOG_SUCCESS);
                m_shinyPositions.push_back(pos);
            }
            else if (m_eggsToHatch <= 0)
            {
                m_eggColumnsHatched++;
                if (m_programSettings.m_isHatchWithSandwich && m_sandwichTimer.elapsed() > 30 * 60 * 1000)
                {
                    if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch - 2)
                    {
                        emit printLog("Fewer than 2 columns of eggs remaining, not making sandwich!");
                    }
                    else
                    {
                        // 30 minutes passed, try to make another sandwich
                        if (m_flyAttempts == 0)
                        {
                            // don't print log if we're retrying
                            emit printLog("30 minutes passed, attempt making another sandwich");
                        }
                        m_substage = SS_Fly;
                        setState_runCommand(C_Fly, true);
                        m_flySuccess = false;
                        m_videoManager->setAreas({A_Title});
                        break;
                    }
                }

                if (m_programSettings.m_isHatchWithSandwich)
                {
                    qint64 timeRemain = 30 - (m_sandwichTimer.elapsed() / 60000);
                    timeRemain = qMax(timeRemain, 0ll);
                    emit printLog("Sandwich time remaining: " + QString::number(timeRemain) + " min");
                }
                runToBoxCommand();
            }
            else
            {
                setState_runCommand((m_substage == SS_Battle) ? "LUpRight,30,LRight,3750" : "LRight,3750", true);
                m_substage = SS_HatchEggs;
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
            // m_checkShinyCount = [1,5]
            if (!checkBrightnessMeanTarget(GetPartyCaptureAreaOfPos(m_checkShinyCount + 1).m_rect, C_Color_Yellow, 200))
            {
                if (m_missedInputRetryCount < 3)
                {
                    m_checkShinyCount--;
                    m_missedInputRetryCount++;
                    emit printLog("Unable to detect cursor on current party, input might be missed, retrying...", LOG_WARNING);
                    setState_runCommand("DDown,3,Nothing,20");
                }
                else
                {
                    runRestartCommand("Unable to detect cursor on current party, forcing restart", m_programSettings.m_isErrorCapture);
                }
                break;
            }

            m_missedInputRetryCount = 0;
            QString log = "Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(m_checkShinyCount);
            if (!checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Yellow, 200))
            {
                emit printLog(log + " has no pokemon");
            }
            else
            {
                m_hasPokemonCount++;
                if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
                {
                    QPoint pos(m_eggColumnsHatched, m_checkShinyCount);
                    if (m_shinyPositions.contains(pos))
                    {
                        emit printLog(log + " is SHINY!!! But we already know that ;)");
                    }
                    else
                    {
                        // SHINY FOUND!
                        m_shinyPositions.push_back(pos);
                        incrementStat(m_statShinyHatched);
                        emit printLog(log + " is SHINY!!!", LOG_SUCCESS);
                    }
                }
                else
                {
                    emit printLog(log + " is not shiny...");
                }
            }

            if (m_checkShinyCount >= 5)
            {
                // m_eggColumnsHatched = [1,6]
                if (m_hasPokemonCount == 0)
                {
                    // this happens when we have checked shiny of the previous column but unable to put down the next column of eggs
                    emit printLog("No pokemon is detected in party, attempting to put eggs to party again", LOG_WARNING);
                    QString command = "A,3,Nothing,3,Loop,1,LUp,3,DUp,3,Loop,4,LRight,3,Nothing,3,Loop," + QString::number((m_eggColumnsHatched % 6) + 1);

                    m_substage = SS_NextColumn;
                    setState_runCommand(command);
                    break;
                }

                QString command = "A,3,Nothing,3,LUp,3,Loop,1,LRight,3,Nothing,3,Loop," + QString::number(((m_eggColumnsHatched - 1) % 6) + 1) + ",A,3,Nothing,3";
                if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
                {
                    if (!m_shinyPositions.empty())
                    {
                        emit printLog(QString::number(m_shinyPositions.size()) + " SHINY POKEMON IS FOUND!!!", LOG_SUCCESS);
                        for (QPoint const& pos : m_shinyPositions)
                        {
                            emit printLog("Shiny at column " + QString::number(pos.x()) + " row " + QString::number(pos.y()), LOG_SUCCESS);
                        }
                    }
                    else if (m_programSettings.m_operation == EOT_Shiny)
                    {
                        emit printLog("No shiny pokemon is found...restarting game");
                        m_substage = SS_Restart;
                        m_videoManager->clearCaptures();
                        setState_runCommand(C_Restart);
                        break;
                    }
                    else
                    {
                        emit printLog("No shiny pokemon is found...");
                    }
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
    case SS_Fly:
    {
        if (state == S_CommandFinished)
        {
            if (m_flySuccess)
            {
                // go make another sandwich!
                runPicnicCommand();
            }
            else
            {
                m_flyAttempts++;
                if (m_flyAttempts >= 3)
                {
                    emit printLog("Failed 3 flying attempts, will try again next cycle...", LOG_WARNING);
                    runToBoxCommand("B,2,Nothing,20,X,2");
                }
                else
                {
                    emit printLog("Unable to fly to Poco Path Lighthouse, retrying...", LOG_WARNING);
                    m_substage = SS_Hatching;
                    setState_runCommand("B,2,Nothing,20");

                    // SS_Hatching state will add one to this again
                    m_eggColumnsHatched--;
                }
            }
        }
        else if (state == S_CaptureReady)
        {
            if (!m_flySuccess)
            {
                if (checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
                {
                    m_flySuccess = true;
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

    if (!m_shinyPositions.empty())
    {
        error = "An error has occurred but " + QString::number(m_shinyPositions.size()) + " SHINY Pokemon is found, preventing restart";
        if (capture)
        {
            error += ", a capture has been taken";
        }
        emit printLog(error);
        m_substage = SS_Finished;
        setState_runCommand(capture ? "Capture,22,Nothing,200" : "Nothing,10");
        return;
    }

    if (m_programSettings.m_operation == EOT_Hatcher && m_eggColumnsHatched > 0)
    {
        error = "An error has occurred but " + QString::number(m_eggColumnsHatched) + " column(s) of eggs has been hatched, preventing restart";
        if (capture)
        {
            error += ", a capture has been taken";
        }
        emit printLog(error);
        m_substage = SS_Finished;
        setState_runCommand(capture ? "Capture,22,Nothing,200" : "Nothing,10");
        return;
    }

    if (m_programSettings.m_operation == EOT_Hatcher)
    {
        // removed number of hatched eggs, since we are hatching them again...
        incrementStat(m_statEggHatched, -m_eggsHatched);
    }
    else
    {
        // remove those eggs collected that weren't able to hatch
        int collectedButNotHatched = m_eggsCollected - m_eggsHatched;
        incrementStat(m_statEggCollected, -collectedButNotHatched);
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

void SmartSVEggOperation::runToBoxCommand(QString command)
{
    m_missedInputRetryCount = 0;

    m_substage = SS_MainMenu;
    setState_runCommand(command.isEmpty() ? "X,3" : command);
    m_videoManager->clearCaptures();

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
    return CaptureArea(29, 138 + y * 84, 10, 70);
}

void SmartSVEggOperation::soundDetected(int id)
{
    if (id != m_shinySoundID) return;

    if (m_substage == SS_Hatching)
    {
        m_shinySoundDetected = true;
    }
}
