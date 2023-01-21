#include "smarteggoperation.h"

SmartEggOperation::SmartEggOperation
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(setting)
{
    init();
}

void SmartEggOperation::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartEggOperation::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_boxViewChecked = false;

    resetCollectorModeMembers();
    resetHatcherModeMembers();
    m_fadeOutDelayTime = 0; // should not reset

    m_videoCaptured = false;
    m_shinySoundID = 0;
    m_shinyDetected = false;
    m_shinyWasDetected = 0;
    m_shinyCount = 0;
    m_keepCount = 0;
}

void SmartEggOperation::resetCollectorModeMembers()
{
    m_eggsCollected = 0;
    m_talkDialogAttempts = 0;
    m_eggCollectAttempts = 0;
}

void SmartEggOperation::resetHatcherModeMembers()
{
    m_eggColumnsHatched = 0;
    m_eggsToHatchCount = 0;
    m_eggsToHatchColumn = 0;
    m_blackScreenDetected = false;

    m_shinySingleCount = 0;
}

void SmartEggOperation::runNextState()
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
        initStat(m_statPokemonKept, "Kept");

        bool testAfterHatched = false;
        if (testAfterHatched)
        {
            m_eggColumnsHatched = 1;
            m_eggsToHatchCount = 5;
            m_eggsToHatchColumn = 5;
            m_substage = SS_ToBox;
            setState_runCommand(C_ToBox);
        }
        else
        {
            if (m_programSettings.m_operation == EOT_Shiny && m_programSettings.m_targetShinyCount == 0)
            {
                setState_error("0 Shiny Pokemon target is set");
                break;
            }

            if (m_programSettings.m_operation == EOT_Collector)
            {
                // force disable shiny detection
                m_programSettings.m_shinyDetection = SDT_Disable;
            }
            else if (m_programSettings.m_shinyDetection == SDT_Sound)
            {
                // Setup sound detection
                m_shinySoundID = m_audioManager->addDetection("PokemonSwSh/ShinySFXHatch", 0.2f, 5000);
                connect(m_audioManager, &AudioManager::soundDetected, this, &SmartEggOperation::soundDetected);
            }

            m_substage = SS_InitCheckCount;
            setState_runCommand("X,1,ASpam,20,Nothing,40");
            m_videoManager->setAreas(GetPartyCaptureAreas());
        }
        break;
    }
    case SS_InitCheckCount:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            EggPokeCountPair countPair = checkPokemonCountInParty();
            if (countPair.first > 0)
            {
                setState_error("There should not be any eggs in the party");
                break;
            }

            switch (m_programSettings.m_operation)
            {
            case EOT_Collector:
            {
                if (countPair.second == 5)
                {
                    // we don't care about box view, go to collect eggs now
                    m_boxViewChecked = true;
                    m_substage = SS_CollectCycle;
                    setState_runCommand("BSpam,40," + m_commands[C_CollectCycle]);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    setState_error("There should be a full party while using Collector Mode");
                }
                break;
            }
            case EOT_Hatcher:
            {
                if (countPair.second == 0)
                {
                    m_boxViewChecked = false;
                    m_substage = SS_ToBox;
                    setState_runCommand("R,1,Nothing,1,Loop,5");
                }
                else
                {
                    setState_error("There should only 1 Pokemon with Flame Body in the team in Hatcher Mode");
                }
                break;
            }
            case EOT_Shiny:
            {
                m_programSettings.m_targetEggCount = 30;
                if (countPair.second == 5)
                {
                    m_boxViewChecked = false;
                    m_substage = SS_ToBox;
                    setState_runCommand("R,1,Nothing,1,Loop,5");
                }
                else
                {
                    setState_error("There should be a full party while using Shiny Mode");
                }
                break;
            }
            default:
            {
                setState_error("Invalid operation type");
                break;
            }
            }
        }
        break;
    }
    case SS_InitBoxView:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Level.m_rect, C_Color_Grey, 190))
            {
                setState_ocrRequest(GetBoxStatNumArea(ST_Attack).m_rect, C_Color_Text);
            }
            else
            {
                // viewing model
                emit printLog("Box is not at Judge View, fixing it for you...", LOG_WARNING);
                setState_runCommand("Plus,1,Nothing,1,Loop,2,Nothing,20");
            }
        }
        else if (state == S_OCRReady)
        {
            QString text = getOCRStringRaw();
            bool ok = false;
            text.toInt(&ok);

            if (ok)
            {
                // viewing number stats
                emit printLog("Box is not at Judge View, fixing it for you...", LOG_WARNING);
                setState_runCommand("Plus,1,Nothing,20");
            }
            else
            {
                // viewing judge function (TODO: check match with text?)
                emit printLog("Box at Judge View confirmed");
                m_boxViewChecked = true;
                m_videoManager->clearCaptures();

                switch (m_programSettings.m_operation)
                {
                case EOT_Hatcher:
                {
                    m_substage = SS_ToBox;
                    setState_runCommand("DRight,1");
                    break;
                }
                case EOT_Shiny:
                {
                    m_substage = SS_CollectCycle;
                    setState_runCommand("BSpam,80," + m_commands[C_CollectCycle]);
                    break;
                }
                default:
                {
                    setState_error("Unhandled operation type");
                    break;
                }
                }
            }
        }
        break;
    }
    case SS_CollectCycle:
    {
        if (state == S_CommandFinished)
        {
            if (m_talkDialogAttempts >= 2)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect Yes/No box when talking to Nursery");
                break;
            }

            m_talkDialogAttempts++;
            m_timer.restart();
            setState_runCommand("A,20,Nothing,20", true);
            m_videoManager->setAreas({A_Yes,A_YesNo});
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_YesNo.m_rect, C_Color_White, 240))
            {
                // wait a little for box to fully open
                m_talkDialogAttempts = 0;
                m_substage = SS_CollectTalk;
                setState_runCommand("Nothing,10");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CollectTalk:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            m_substage = SS_CollectEgg;
            m_videoManager->clearCaptures();
            if (checkBrightnessMeanTarget(A_Yes.m_rect, C_Color_Black, 200))
            {
                // Collect egg
                m_eggsCollected++;
                m_eggCollectAttempts = 0;
                incrementStat(m_statEggCollected);
                emit printLog("Egg collected! (" + QString::number(m_programSettings.m_targetEggCount - m_eggsCollected) + " remaining)");
                setState_runCommand(C_CollectEgg);
            }
            else
            {
                // No egg
                m_eggCollectAttempts++;
                if (m_eggCollectAttempts >= 10)
                {
                    incrementStat(m_statError);
                    setState_error("Unable to collect eggs after 10 attempts");
                }
                else
                {
                    emit printLog("No eggs to collect");
                    setState_runCommand("BSpam,20");
                }
            }
        }
        break;
    }
    case SS_CollectEgg:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggsCollected >= m_programSettings.m_targetEggCount)
            {
                if (m_programSettings.m_operation == EOT_Shiny)
                {
                    m_programSettings.m_columnsToHatch = 6;
                    resetHatcherModeMembers();

                    m_substage = (m_programSettings.m_operation == EOT_Shiny) ? SS_BoxFiller : SS_ToBox;
                    setState_runCommand(C_ToBox);
                }
                else
                {
                    m_substage = SS_Finished;
                    setState_runCommand("Home,2");
                }
            }
            else
            {
                m_substage = SS_CollectCycle;
                setState_runCommand(C_CollectCycle);
            }
        }
        break;
    }
    case SS_BoxFiller:
    case SS_ToBox:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Box});
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect in Box menu for too long");
            }
            else if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Box, 240))
            {
                if (m_eggColumnsHatched == 0)
                {
                    if (m_substage == SS_BoxFiller)
                    {
                        // put 5 pokemon in team to keep box
                        emit printLog("Putting 5 filler Pokemon to keep box");
                        m_substage = SS_ToBox;
                        setState_runCommand(C_BoxFiller);
                    }
                    else if (!m_boxViewChecked)
                    {
                        m_substage = SS_InitBoxView;
                        setState_runCommand("DLeft,1,Nothing,20");
                        m_videoManager->setAreas({A_Level,GetBoxStatNumArea(ST_Attack)});
                    }
                    else
                    {
                        m_substage = SS_CheckEggCount;
                        setState_runCommand(C_FirstColumn);
                        m_videoManager->setAreas(GetPartyCaptureAreas());
                    }
                }
                else
                {
                    // go to the bottom of the hatched eggs
                    m_substage = SS_CheckStats;
                    setState_runCommand("DLeft,1,DDown,1,Nothing,20");
                    m_videoManager->setAreas({A_Shiny});
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CheckEggCount:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            EggPokeCountPair countPair = checkPokemonCountInParty();
            if (countPair.second > 0)
            {
                incrementStat(m_statError);
                setState_error("There should not be other Pokemon in the party except for eggs");
            }
            else
            {
                m_eggsToHatchColumn = countPair.first;
                if (m_eggsToHatchColumn == 0)
                {
                    incrementStat(m_statError);
                    setState_error("There are no eggs in party, something went really wrong");
                }
                else
                {
                    emit printLog("Hatching column " + QString::number(m_eggColumnsHatched + 1) + " with " + QString::number(m_eggsToHatchColumn) + " eggs");
                    m_timer.restart();
                    m_eggsToHatchCount = 0;
                    m_blackScreenDetected = false;

                    m_substage = SS_HatchCycle;
                    setState_runCommand("BSpam,40");
                    m_videoManager->setAreas({A_Dialog});
                }
            }
        }
        break;
    }
    case SS_HatchCycle:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggsToHatchCount >= m_eggsToHatchColumn)
            {
                // finished hatching the current column
                m_eggColumnsHatched++;
                m_substage = SS_ToBox;
                setState_runCommand(C_ToBox);
            }
            else
            {
                // this command never returns finished (loop 0)
                setState_runCommand(C_HatchCycle, true);
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 180000)
            {
                incrementStat(m_statError);
                setState_error("Did not hatch any eggs for 3 minutes");
            }
            else if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 240))
            {
                m_eggsToHatchCount++;
                incrementStat(m_statEggHatched);
                emit printLog("Oh? Egg no." + QString::number(m_statEggHatched.first) + " is hatching! (" + QString::number(m_eggsToHatchColumn - m_eggsToHatchCount) + " remaining)");

                m_substage = SS_Hatching;
                setState_runCommand("BSpam,250");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Hatching:
    {
        if (state == S_CommandFinished)
        {
            // restart timer to check for fade out delay
            m_timer.restart();

            // sound detection
            if (m_shinySoundID > 0)
            {
                m_audioManager->startDetection(m_shinySoundID);
            }

            setState_runCommand("BSpam,2,Loop,0", true);
            m_videoManager->setAreas({A_Dialog});
        }
        else if (state == S_CaptureReady)
        {
            bool blackScreenDetected = checkAverageColorMatch(A_Dialog.m_rect, QColor(0,0,0));
            if (!m_blackScreenDetected && blackScreenDetected)
            {
                m_blackScreenDetected = true;
            }
            else if (m_blackScreenDetected && !blackScreenDetected)
            {
                m_blackScreenDetected = false;
                qint64 elapsed = m_timer.restart();

                // shiny detection
                switch (m_programSettings.m_shinyDetection)
                {
                case SDT_Delay:
                {
                    if (m_fadeOutDelayTime == 0)
                    {
                        m_fadeOutDelayTime = elapsed;
                        emit printLog("Fade Out Delay time calibrated = " + QString::number(m_fadeOutDelayTime) + "ms");
                    }
                    else if (elapsed > m_fadeOutDelayTime + 1000)
                    {
                        emit printLog("Fade Out Delay: " + QString::number(elapsed) + "ms > " + QString::number(m_fadeOutDelayTime + 1000) + "ms", LOG_SUCCESS);
                        m_shinyDetected = true;
                    }
                    else
                    {
                        if (elapsed <= m_fadeOutDelayTime - 1000)
                        {
                            emit printLog("Fade Out Delay is much lower than before, the previous Pokemon might have been shiny? Delay is now updated to " + QString::number(elapsed) + "ms", LOG_WARNING);
                        }
                        m_fadeOutDelayTime = qMin(m_fadeOutDelayTime, elapsed);
                    }
                    break;
                }
                case SDT_Sound:
                {
                    m_audioManager->stopDetection(m_shinySoundID);
                    break;
                }
                default: break;
                }

                // move back to position while still looking for eggs to hatch
                m_substage = SS_HatchCycle;
                bool videoCapture = false;
                if (m_shinyDetected)
                {
                    m_shinyWasDetected++;
                    m_shinyDetected = false;

                    if (!m_videoCaptured)
                    {
                        videoCapture = true;
                        m_videoCaptured = true;
                        emit printLog("SHINY Pokemon detected! Capturing video!", LOG_SUCCESS);
                    }
                    else
                    {
                        // TODO: square/star shiny one video each
                        emit printLog("SHINY Pokemon detected! But we have already captured the same video, skipping capture");
                    }
                }

                if (videoCapture)
                {
                    setState_runCommand("Capture,22," + m_commands[C_HatchReturn], m_eggsToHatchCount < m_eggsToHatchColumn);
                }
                else
                {
                    setState_runCommand(C_HatchReturn, m_eggsToHatchCount < m_eggsToHatchColumn);
                }

                if (m_eggsToHatchCount >= m_eggsToHatchColumn)
                {
                    m_videoManager->clearCaptures();
                }
                break;
            }

            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_CheckStats:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_eggsToHatchCount < 1 && m_eggsToHatchCount > 5)
            {
                setState_error("m_eggsToHatchCount is invalid, we should not end up here");
                break;
            }

            QString log = "Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(6 - m_eggsToHatchCount);
            if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                m_shinyCount++;
                m_shinySingleCount++;
                incrementStat(m_statShinyHatched);

                // to check if we correctly detected shiny icon in box
                if (m_shinyWasDetected > 0)
                {
                    m_shinyWasDetected--;
                }

                emit printLog(log + " is SHINY!!! Moving it to keep box!", LOG_SUCCESS);
                runKeepPokemonCommand();
            }
            else
            {
                emit printLog(log + " is not shiny, releasing");
                m_substage = SS_ReleaseHasPokemon;
                m_videoManager->setAreas({A_DialogBox});
                setState_runCommand("A,25", true);
            }

            m_eggsToHatchCount--;
        }
        break;
    }
    case SS_KeepPokemon:
    case SS_ReleaseConfirm:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggsToHatchCount > 0)
            {
                m_substage = SS_CheckStats;
                setState_runCommand("Nothing,20");
                m_videoManager->setAreas({A_Shiny});
            }
            else
            {
                if (m_shinyWasDetected > 0)
                {
                    setState_error("There were " + QString::number(m_shinyWasDetected) + " SHINY Pokemon detected with sound/delay but are released due to unable to detect shiny icon in Box view...");
                    break;
                }

                m_substage = SS_NextColumn;
                setState_runCommand("Y,1,DUp,1,Y,1,DRight,1");
            }
        }
        break;
    }
    case SS_NextColumn:
    {
        if (state == S_CommandFinished)
        {
            if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
            {
                if (m_shinyCount > 0)
                {
                    if (m_shinySingleCount > 0)
                    {
                        QString log = QString::number(m_shinyCount) + " SHINY Pokemon has been found";
                        if (m_programSettings.m_operation == EOT_Shiny)
                        {
                            log += ", " + QString::number(m_programSettings.m_targetShinyCount - m_shinyCount) + " remaining";
                        }
                        emit printLog(log, LOG_SUCCESS);
                        m_shinySingleCount = 0;
                    }
                    else
                    {
                        emit printLog("No additional Shiny Pokemon is found");
                    }
                }
                else
                {
                    emit printLog("No Shiny Pokemon is found...");
                }

                if (m_keepCount - m_shinyCount > 0)
                {
                    emit printLog(QString::number(m_keepCount - m_shinyCount) + " non-Shiny Pokemon has been stored in keep box", LOG_SUCCESS);
                }
                else
                {
                    emit printLog("No Non-Shiny Pokemon is kept...");
                }

                if (m_programSettings.m_operation == EOT_Shiny)
                {
                    if (m_shinyCount < m_programSettings.m_targetShinyCount)
                    {
                        emit printLog("Taking 5 filler Pokemon from keep box to party");
                        m_substage = SS_TakeFiller;
                        setState_runCommand(C_TakeFiller);
                        break;
                    }

                    // TODO: other keep pokemon
                }

                // we are done, move back to first box
                m_substage = SS_Finished;
                QString command;
                if (m_eggColumnsHatched > 6)
                {
                    command = "L,1,Nothing,5,Loop," + QString::number((m_eggColumnsHatched - 1) / 6) + ",";
                }
                setState_runCommand(command + "Home,2");
            }
            else
            {
                QString command = (m_eggColumnsHatched % 6 == 0) ? "R,1,Nothing,5" : ("DRight,1,Nothing,1,Loop," + QString::number(m_eggColumnsHatched % 6));
                command += ",A,1,DUp,1,A,3,Loop,1,DLeft,1,Nothing,1,Loop," + QString::number((m_eggColumnsHatched % 6) + 1);
                command += ",DDown,1,A,1,BSpam,10,Nothing,50";

                m_substage = SS_CheckEggCount;
                setState_runCommand(command);
                m_videoManager->setAreas(GetPartyCaptureAreas());
            }
        }
        break;
    }
    case SS_ReleaseHasPokemon:
    {
        if (state == S_CommandFinished)
        {
            incrementStat(m_statError);
            setState_error("No Pokemon detected when trying to release");
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_DialogBox.m_rect, C_Color_Dialog, 240))
            {
                m_substage = SS_ReleaseYesNo;
                setState_runCommand("DUp,1,LUp,1,A,20");
                m_videoManager->setAreas({A_No,A_DialogBox,A_Shiny});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ReleaseYesNo:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 1000)
            {
                if (checkBrightnessMeanTarget(A_DialogBox.m_rect, C_Color_Dialog, 240))
                {
                    incrementStat(m_statError);
                    setState_error("Unable to release current Pokemon, it was an egg or team registered Pokemon");
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("Unable to detect Yes/No box when releasing Pokemon");
                }
            }
            else if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                // shiny, we should not end up here
                incrementStat(m_statError);
                setState_error("Current Pokemon is a SHINY, not releasing, we should've detected it eariler");
            }
            else if (checkBrightnessMeanTarget(A_No.m_rect, C_Color_Black, 200))
            {
                m_substage = SS_ReleaseConfirm;
                setState_runCommand("DUp,1,A,25,Nothing,1,A,4");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_TakeFiller:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Collecting more eggs...");
            resetCollectorModeMembers();

            // go back to collecting eggs
            m_substage = SS_CollectCycle;
            setState_runCommand("BSpam,80");
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

SmartEggOperation::EggPokeCountPair SmartEggOperation::checkPokemonCountInParty()
{
    int eggCount = 0;
    int pokemonCount = 0;
    for (int i = 2; i <= 6; i++)
    {
        if (checkPixelColorMatch(GetPartyCapturePointOfPos(i).m_point, QColor(253,253,253)))
        {
            if (checkBrightnessMeanTarget(GetPartyCaptureAreaOfPos(i).m_rect, C_Color_White, 240))
            {
                eggCount++;
            }
            else
            {
                pokemonCount++;
            }
        }
    }

    return EggPokeCountPair(eggCount, pokemonCount);
}

void SmartEggOperation::runKeepPokemonCommand(int yPos)
{
    if (yPos < 2 || yPos > 6)
    {
        setState_error("Invalid position when releasing Pokemon in party");
        return;
    }

    m_keepCount++;
    incrementStat(m_statPokemonKept);

    QString command = "Y,1,A,3,Loop,1"; // pick up
    int scrollCount = 1 + (m_eggColumnsHatched - 1) / 6;
    if (yPos < 6)
    {
        command += ",DDown,1,Nothing,1,Loop," + QString::number(6 - yPos); // move down to bottom
    }
    command += ",DRight,1,Loop,1,L,1,Nothing,5,Loop," + QString::number(scrollCount); // pick up move to box list
    command += ",A,1,Nothing,10,ASpam,8,Nothing,4,Loop,1,R,1,Nothing,5,Loop," + QString::number(scrollCount) + ",Y,1,DLeft,1,Y,1,Loop,1"; // go back to current box
    if (yPos < 6)
    {
        command += ",DUp,1,Nothing,1,Loop," + QString::number(6 - yPos); // move back to the egg's origial slot in party
    }

    m_substage = SS_KeepPokemon;
    setState_runCommand(command);
}

const QVector<CaptureArea> SmartEggOperation::GetPartyCaptureAreas()
{
    return
    {
        GetPartyCaptureAreaOfPos(2),
        GetPartyCaptureAreaOfPos(3),
        GetPartyCaptureAreaOfPos(4),
        GetPartyCaptureAreaOfPos(5),
        GetPartyCaptureAreaOfPos(6)
    };
}

const CaptureArea SmartEggOperation::GetPartyCaptureAreaOfPos(int y)
{
    if (y < 1 || y > 6)
    {
        y = 1;
    }

    y--;
    return CaptureArea(256, 151 + y * 96, 104, 12);
}

const CapturePoint SmartEggOperation::GetPartyCapturePointOfPos(int y)
{
    if (y < 1 || y > 6)
    {
        y = 1;
    }

    y--;
    return CapturePoint(402, 134 + y * 96);
}

const CaptureArea SmartEggOperation::GetBoxStatNumArea(StatType type)
{
    return CaptureArea(998, 146 + int(type) * 38, 133, 30);
}

const CaptureArea SmartEggOperation::GetBoxStatNameArea(StatType type)
{
    return CaptureArea(882, 146 + int(type) * 38, 106, 30);
}

void SmartEggOperation::soundDetected(int id)
{
    if (id == m_shinySoundID && m_substage == SS_Hatching)
    {
        m_shinyDetected = true;
    }
}
