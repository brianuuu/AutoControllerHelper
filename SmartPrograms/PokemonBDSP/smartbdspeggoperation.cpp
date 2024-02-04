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

    m_watchEnabled = false;

    m_dialogDetected = false;
    m_eggsCollected = 0;

    m_eggColumnsHatched = 0;
    m_eggsToHatchCount = 0;
    m_eggsToHatchColumn = 0;
    m_isStatView = false;

    m_shinyWasFound = false;
    m_shinyCount = 0;
    m_shinySoundID = 0;
    m_loopDone = 0;
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

        if (m_programSettings.m_operation != EggOperationType::EOT_Collector && m_programSettings.m_shinyDetection == ShinyDetectionType::SDT_Sound)
        {
            // Setup sound detection
            m_shinySoundID = m_audioManager->addDetection("PokemonBDSP/ShinySFX", 0.2f, 5000);
            connect(m_audioManager, &AudioManager::soundDetected, this, &SmartBDSPEggOperation::soundDetected);
        }
        break;
    }
    case SS_Start:
    {
        if (state == S_CommandFinished)
        {
            if (!m_watchEnabled && m_programSettings.m_operation != EggOperationType::EOT_Hatcher)
            {
                m_substage = SS_Watch;
                setState_frameAnalyzeRequest();

                m_videoManager->setAreas({A_Watch});
            }
            else
            {
                if (m_programSettings.m_operation == EggOperationType::EOT_Collector)
                {
                    if (m_eggsCollected >= m_programSettings.m_targetEggCount)
                    {
                        // enough eggs collected
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
                    // open box
                    if (m_isStatView)
                    {
                        m_substage = SS_ToBox;
                        setState_runCommand(C_ToBox);
                    }
                    else
                    {
                        m_substage = SS_CheckView;
                        setState_runCommand(m_commands[C_ToBox] + ",LLeft,1,Nothing,20");

                        m_videoManager->setAreas({A_Pokemon, A_Stat});
                    }
                }
                else // shiny mode
                {
                    if (m_shinyCount >= m_programSettings.m_statTable->GetTableList()[0].m_target)
                    {
                        // reached shiny target? should not reach here
                        m_substage = SS_Finished;
                        setState_runCommand("Home,2");
                    }
                    else if (m_eggsCollected >= 5)
                    {
                        // start hatching 1st column
                        m_shinyWasFound = false;
                        m_eggsCollected = 0;
                        m_eggColumnsHatched = 0;
                        m_eggsToHatchColumn = 0;
                        m_programSettings.m_columnsToHatch = 1;

                        // open box
                        if (m_isStatView)
                        {
                            m_substage = SS_PartyKeep;
                            setState_runCommand(C_ToBox);

                            m_videoManager->clearCaptures();
                        }
                        else
                        {
                            m_substage = SS_CheckView;
                            setState_runCommand(m_commands[C_ToBox] + ",LLeft,1,Nothing,20");

                            m_videoManager->setAreas({A_Pokemon, A_Stat});
                        }
                    }
                    else
                    {
                        // more eggs to collect
                        m_substage = SS_CycleCollect;
                        if (m_loopDone > 0 && m_eggsCollected == 0)
                        {
                            // immediately talk to nursery, should have an egg by now
                            setState_runCommand("Nothing,5");
                        }
                        else
                        {
                            setState_runCommand(C_CycleCollect);
                        }

                        m_programSettings.m_targetEggCount = 5;
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
            if (!checkAverageColorMatch(A_Watch.m_rect, C_Color_Watch))
            {
                if (m_watchEnabled)
                {
                    incrementStat(m_error);
                    setState_error("Unable to detect Poketch");
                }
                else
                {
                    m_watchEnabled = true;
                    emit printLog("Enabling Poketch...");

                    setState_runCommand("R,1,Nothing,30");
                }
            }
            else
            {
                m_watchEnabled = true;
                emit printLog("Poketch detected");

                m_substage = SS_Start;
                setState_runCommand("Nothing,1");
            }
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
                else if (!checkAverageColorMatch(A_Watch.m_rect, C_Color_Watch))
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
            else if (checkAverageColorMatch(A_Watch.m_rect, C_Color_Watch))
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
    case SS_PartyKeep:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Putting 5 filler Pokemon to Keep Box");
            m_substage = SS_ToBox;
            setState_runCommand(C_PartyKeep);
        }
        break;
    }
    case SS_PartyBack:
    {
        if (state == S_CommandFinished)
        {
            int shinyRemaining = qMax(0, m_programSettings.m_statTable->GetTableList()[0].m_target - m_shinyCount);
            emit printLog(QString::number(m_shinyCount) + " shiny was found (" + QString::number(shinyRemaining) + " left)");
            if (shinyRemaining == 0)
            {
                // reached shiny target, go to keep box and finish
                m_substage = SS_Finished;
                setState_runCommand("L,22,Home,2");
                break;
            }

            m_eggsCollected = 0;

            // hatched 50 eggs, restart game
            m_loopDone++;
            if (m_loopDone >= 10 && !m_shinyWasFound)
            {
                m_watchEnabled = false;
                m_loopDone = 0;

                m_isStatView = false;
                emit printLog("It's been over 50 eggs, restarting game to prevent crash", LOG_WARNING);

                m_substage = SS_Restart;
                setState_runCommand(m_settings->isPreventUpdate() ? C_RestartNoUpdate : C_Restart);

                m_videoManager->clearCaptures();
                break;
            }

            // start from beginning
            if (m_shinyWasFound)
            {
                emit printLog("Saving game as a shiny was found");
            }
            m_substage = SS_Start;
            setState_runCommand(m_shinyWasFound ? C_QuitBoxSave : C_QuitBox);
        }
        break;
    }
    case SS_ToBox:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggColumnsHatched == 0)
            {
                // pick up first column
                m_substage = SS_PickEggs;
                setState_runCommand("Y,1,Nothing,1,Y,1,A,1,Loop,1,DDown,1,Nothing,6,Loop,6,A,6,DLeft,1,DDown,1,A,32");

                m_eggsToHatchColumn = 0;
                m_videoManager->setAreas({A_Pokemon, A_Stat});
            }
            else
            {
                m_substage = SS_CheckShiny;
                setState_runCommand("DLeft,1,Nothing,6,DDown,1,Nothing,20");
            }
        }
        break;
    }
    case SS_CheckView:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkBrightnessMeanTarget(A_Stat.m_rect, C_Color_Dialog, 230))
            {
                if (m_isStatView)
                {
                    incrementStat(m_error);
                    setState_error("Unable to detect Stat/Judge View");
                }
                else
                {
                    m_isStatView = true;
                    emit printLog("Box is not at Stat/Judge View, fixing it for you...", LOG_WARNING);
                    setState_runCommand("Plus,1,Nothing,20");
                }
            }
            else
            {
                m_isStatView = true;
                emit printLog("Stat/Judge View confirmed");
                m_substage = (m_programSettings.m_operation == EggOperationType::EOT_Shiny) ? SS_PartyKeep : SS_ToBox;
                setState_runCommand("LRight,1,Nothing,6");
            }
        }
        break;
    }
    case SS_CheckShiny:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggsToHatchCount == 0)
            {
                if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
                {
                    if (m_programSettings.m_operation == EggOperationType::EOT_Shiny)
                    {
                        // shiny mode put party back
                        emit printLog("Taking 5 filler Pokemon from Keep Box to party");
                        m_substage = SS_PartyBack;
                        setState_runCommand(C_PartyBack);
                    }
                    else
                    {
                        // done
                        m_substage = SS_QuitBox;
                        setState_runCommand(C_QuitBox);
                    }

                    m_videoManager->clearCaptures();
                }
                else
                {
                    // this column is done, pick up next column
                    QString command = "Y,1,DUp,1,Nothing,6,Y,1,DRight,1,Nothing,6,Loop,1"; // go to box top left
                    int moveColumnCount = m_eggColumnsHatched % 6;
                    if (moveColumnCount == 0)
                    {
                        // next box
                        command += ",R,22,Loop,1";
                    }
                    else
                    {
                        // move to column
                        command += ",DRight,1,Nothing,6,Loop," + QString::number(moveColumnCount);
                    }
                    command += ",A,1,Loop,1,DDown,1,Nothing,6,Loop,6,A,6,DDown,1,Loop,1"; // pick up column
                    command += ",DLeft,1,Nothing,1,Loop," + QString::number(moveColumnCount + 1); // move to party
                    command += ",A,32"; // drop at party

                    m_substage = SS_PickEggs;
                    setState_runCommand(command);

                    m_eggsToHatchColumn = 0;
                    m_videoManager->setAreas({A_Pokemon, A_Stat});
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
                m_videoManager->setAreas({A_Shiny, A_Pokemon, A_Stat});
            }
        }
        else if (state == S_CaptureReady)
        {
            if (!checkBrightnessMeanTarget(A_Stat.m_rect, C_Color_Dialog, 230))
            {
                incrementStat(m_error);
                setState_error("Expected Pokemon, none detected");
            }
            else if (checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Dialog, 230))
            {
                incrementStat(m_error);
                setState_error("Unexpected egg detected");
            }
            else
            {
                QString log = "Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(6 - m_eggsToHatchCount);
                if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 60))
                {
                    // shiny found
                    m_shinyCount++;
                    m_shinyWasFound = true;
                    incrementStat(m_statShinyHatched);
                    emit printLog(log + " is SHINY!!! Moving it to Keep Box!", LOG_SUCCESS);

                    // send discord message
                    if (m_programSettings.m_shinyDetection == ShinyDetectionType::SDT_Disable)
                    {
                        QImage frame;
                        m_videoManager->getFrame(frame);
                        sendDiscordMessage("Shiny Found!", true, QColor(255,255,0), &frame);
                    }

                    // run keep command
                    int moveBoxCount = (m_eggColumnsHatched + 5) / 6;
                    QString command = "Y,1,A,6,DUp,1,LUp,1,DUp,1,DRight,1,Loop,1"; // move cursor to Box List
                    command += ",L,1,Nothing,21,Loop," + QString::number(moveBoxCount); // move to keep box
                    command += ",A,1,Nothing,20,Loop,2,B,20,Loop,1"; // drop to keep box
                    command += ",R,1,Nothing,21,Loop," + QString::number(moveBoxCount); // return to egg box
                    command += ",DLeft,1,Loop,1,DDown,1,Nothing,6,Loop,3,Y,1,Nothing,1,Loop,2,Nothing,20"; // return to 2nd party
                    setState_runCommand(command);
                }
                else
                {
                    // release non-shiny
                    emit printLog(log + " is not shiny, releasing");
                    setState_runCommand(C_Release);
                }

                m_eggsToHatchCount--;
            }
        }
        break;
    }
    case SS_PickEggs:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Stat.m_rect, C_Color_Dialog, 230))
            {
                if (!checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Dialog, 230))
                {
                    incrementStat(m_error);
                    setState_error("Only excepting eggs in party");
                    break;
                }

                m_eggsToHatchColumn++;
                if (m_eggsToHatchColumn < 5)
                {
                    setState_runCommand("DDown,1,Nothing,25");
                    break;
                }

                if (m_programSettings.m_operation == EggOperationType::EOT_Shiny && m_eggsToHatchColumn != 5)
                {
                    incrementStat(m_error);
                    setState_error("Always expecting 5 eggs in party in Shiny Mode");
                    break;
                }
            }

            if (m_eggsToHatchColumn == 0)
            {
                incrementStat(m_error);
                setState_error("No eggs detected for current column");
                break;
            }

            emit printLog("Hatching column " + QString::number(m_eggColumnsHatched + 1) + " with " + QString::number(m_eggsToHatchColumn) + " eggs");
            m_substage = SS_QuitBox;
            setState_runCommand(C_QuitBox);

            m_eggsToHatchCount = 0;
            m_videoManager->clearCaptures();

        }
        break;
    }
    case SS_QuitBox:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
            {
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
            }
            else if (m_eggsToHatchCount < m_eggsToHatchColumn)
            {
                // start hatching eggs
                m_substage = SS_HatchEggs;
                setState_runCommand(C_CycleHatch, true);

                m_hatchingDialog = 0;
                m_shinyDetected = false;

                m_videoManager->setAreas({A_Dialog});
                m_timer.restart();
            }
            else
            {
                // finish hatching current column of eggs, check shiny
                m_eggColumnsHatched++;

                m_substage = SS_ToBox;
                setState_runCommand(C_ToBox);

                m_videoManager->clearCaptures();
            }
        }
        break;
    }
    case SS_HatchEggs:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() < 1000) // buffer
            {
                setState_frameAnalyzeRequest();
            }
            else if (m_timer.elapsed() > 600000) // 10min
            {
                incrementStat(m_error);
                setState_error("Unable to hatch any eggs over 10 minutes");
            }
            else if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 230))
            {
                m_hatchingDialog++;
                if (m_hatchingDialog == 1)
                {
                    m_eggsToHatchCount++;
                    incrementStat(m_statEggHatched);

                    // sound detection
                    if (m_shinySoundID > 0)
                    {
                        m_audioManager->startDetection(m_shinySoundID);
                    }

                    // spam and skip the first black flash
                    emit printLog("Oh? Egg no." + QString::number(m_statEggHatched.first) + " is hatching! (" + QString::number(m_eggsToHatchColumn - m_eggsToHatchCount) + " remaining)");
                    setState_runCommand("ASpam,180");
                }
                else // 2 & 3
                {
                    // double click to pass dialog
                    QString command = "ASpam,4,Nothing,120";
                    if (m_hatchingDialog == 3)
                    {
                        m_substage = SS_QuitBox;
                        if (m_shinyDetected)
                        {
                            command += ",Capture,22";
                        }
                        command += ",LLeft,20,LDown,10";

                        m_audioManager->stopDetection(m_shinySoundID);
                    }
                    setState_runCommand(command);
                }
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

void SmartBDSPEggOperation::soundDetected(int id)
{
    if (id == m_shinySoundID && m_substage == SS_HatchEggs)
    {
        m_shinyDetected = true;

        // send discord message
        QImage frame;
        m_videoManager->getFrame(frame);
        sendDiscordMessage("Shiny Found!", true, QColor(255,255,0), &frame);
    }
}
