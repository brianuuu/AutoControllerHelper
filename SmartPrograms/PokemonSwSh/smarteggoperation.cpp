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

    m_eggsCollected = 0;
    m_talkDialogAttempts = 0;
    m_eggCollectAttempts = 0;

    m_eggColumnsHatched = 0;
    m_eggsToHatchCount = 0;
    m_eggsToHatchColumn = 0;
    m_blackScreenDetected = false;
    m_keepColumnCount = 0;

    m_releaseCount = 0;

    m_shinyCount = 0;
    m_keepCount = 0;
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

        switch (m_programSettings.m_operation)
        {
        case EOT_Collector:
        {
            m_substage = SS_CollectCycle;
            setState_runCommand("Nothing,1");
            break;
        }
        case EOT_Hatcher:
        {
            //m_eggColumnsHatched = 1;
            //m_eggsToHatchCount = 5;
            //m_eggsToHatchColumn = 5;
            //m_substage = SS_ToBox;
            //setState_runCommand(C_ToBox);

            m_substage = SS_ToHatch;
            setState_runCommand(C_ToHatch);
            break;
        }
        case EOT_Release:
        {
            m_substage = SS_ReleaseConfirm;
            setState_runCommand("Nothing,1");
            break;
        }
        default:
        {
            setState_error("Invalid operation type");
            break;
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
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
                break;
            }

            m_substage = SS_CollectCycle;
            setState_runCommand(C_CollectCycle);
        }
        break;
    }
    case SS_ToHatch:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_ToBox;
            setState_runCommand(C_ToBox);
        }
        break;
    }
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
                    m_substage = SS_CheckEggCount;
                    setState_runCommand(C_FirstColumn);
                    m_videoManager->setAreas({
                                                 GetPartyCaptureAreaOfPos(2),
                                                 GetPartyCaptureAreaOfPos(3),
                                                 GetPartyCaptureAreaOfPos(4),
                                                 GetPartyCaptureAreaOfPos(5),
                                                 GetPartyCaptureAreaOfPos(6)
                                             });
                }
                else
                {
                    m_keepColumnCount = 0;

                    // go to the bottom of the hatched eggs
                    m_substage = SS_CheckStats;
                    setState_runCommand("Y,1,DLeft,1,Loop,1,DDown,1,Nothing,1,Loop," + QString::number(m_eggsToHatchColumn) + ",Nothing,20");
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
            bool hasError = false;
            m_eggsToHatchColumn = 0;
            for (int i = 2; i <= 6; i++)
            {
                if (checkPixelColorMatch(GetPartyCapturePointOfPos(i).m_point, QColor(253,253,253)))
                {
                    if (checkBrightnessMeanTarget(GetPartyCaptureAreaOfPos(i).m_rect, C_Color_White, 240))
                    {
                        m_eggsToHatchColumn++;
                    }
                    else
                    {
                        hasError = true;
                        incrementStat(m_statError);
                        setState_error("There should not be other Pokemon in the party except for eggs");
                        break;
                    }
                }
            }

            if (!hasError)
            {
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
                m_timer.restart();

                // move back to position while still looking for eggs to hatch
                m_substage = SS_HatchCycle;
                setState_runCommand(C_HatchReturn, m_eggsToHatchCount < m_eggsToHatchColumn);

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
            if (m_eggsToHatchCount > 0)
            {
                QString log = "Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(m_eggsToHatchCount);
                //log += " (Egg no." + QString::number(m_statEggHatched.first - m_eggsToHatchColumn + m_eggsToHatchCount) + ")";

                QString command;
                if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
                {
                    m_keepColumnCount++;
                    emit printLog(log + " is SHINY!!! Moving it to keep box!", LOG_SUCCESS);

                    m_keepCount++;
                    incrementStat(m_statPokemonKept);
                    m_shinyCount++;
                    incrementStat(m_statShinyHatched);

                    command = "A,3,Loop,1"; // pick up
                    int scrollCount = 1 + (m_eggColumnsHatched - 1) / 6;
                    if (m_eggsToHatchCount < 5)
                    {
                        command += ",DDown,1,Nothing,1,Loop," + QString::number(5 - m_eggsToHatchCount); // move down to bottom
                    }
                    command += ",DRight,1,Loop,1,L,1,Nothing,5,Loop," + QString::number(scrollCount); // pick up move to box list
                    command += ",A,1,Nothing,10,ASpam,8,Nothing,4,Loop,1,R,1,Nothing,5,Loop," + QString::number(scrollCount) + ",DLeft,1,Loop,1"; // go back to current box
                    if (m_eggsToHatchCount < 5)
                    {
                        command += ",DUp,1,Nothing,1,Loop," + QString::number(5 - m_eggsToHatchCount); // move back to the egg's origial slot in party
                    }
                }
                else
                {
                    emit printLog(log + " is not shiny...");
                }

                m_eggsToHatchCount--;
                if (command.isEmpty())
                {
                    if (m_eggsToHatchCount > 0)
                    {
                        setState_runCommand("DUp,1,Nothing,20");
                        break;
                    }
                }
                else
                {
                    setState_runCommand(command + (m_eggsToHatchCount == 0 ? "" : ",DUp,1,Nothing,20"));
                    break;
                }
            }

            // we are here if m_eggsToHatchCount == 0
            m_substage = SS_NextColumn;
            QString command;
            if (m_keepColumnCount == m_eggsToHatchColumn)
            {
                emit printLog("There are no more Pokemon in the party");
                setState_runCommand("Y,1,DUp,1,Loop,1,DRight,1,Nothing,1,Loop," + QString::number((m_eggColumnsHatched - 1) % 6 + 1));
            }
            else
            {
                // put current party back to box
                setState_runCommand("Y,1,A,1,DUp,1,LUp,1,A,3,DUp,1,Loop,1,DRight,1,Nothing,1,Loop," + QString::number((m_eggColumnsHatched - 1) % 6 + 1) + ",A,3");
            }
        }
        break;
    }
    case SS_NextColumn:
    {
        if (state == S_CaptureReady)
        {
            if (m_eggColumnsHatched >= m_programSettings.m_columnsToHatch)
            {
                if (m_shinyCount > 0)
                {
                    emit printLog(QString::number(m_shinyCount) + " SHINY Pokemon has been found", LOG_SUCCESS);
                }
                else
                {
                    emit printLog("No Shiny Pokemon is found...");
                }

                if (m_keepCount - m_shinyCount > 0)
                {
                    emit printLog(QString::number(m_keepCount - m_shinyCount) + " Non-Shiny Pokemon has been stored in keep box", LOG_SUCCESS);
                }
                else
                {
                    emit printLog("No Non-Shiny Pokemon is kept...");
                }

                // we are done
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
            }
            else
            {
                QString command = (m_eggColumnsHatched % 6 == 0) ? "R,1,Nothing,5,DRight,1,LRight,1" : "DRight,1";
                command += ",A,1,DUp,1,A,3,Loop,1,DLeft,1,Nothing,1,Loop," + QString::number((m_eggColumnsHatched % 6) + 1);
                command += ",DDown,1,A,1,BSpam,10,Nothing,50";

                m_substage = SS_CheckEggCount;
                setState_runCommand(command);
                m_videoManager->setAreas({
                                             GetPartyCaptureAreaOfPos(2),
                                             GetPartyCaptureAreaOfPos(3),
                                             GetPartyCaptureAreaOfPos(4),
                                             GetPartyCaptureAreaOfPos(5),
                                             GetPartyCaptureAreaOfPos(6)
                                         });
            }
        }
        break;
    }
    case SS_ReleaseHasPokemon:
    {
        if (state == S_CommandFinished)
        {
            // no pokemon, next
            m_releaseCount++;
            emit printLog(GetCurrentBoxPosString(m_releaseCount) + " has no Pokemon");

            m_substage = SS_ReleaseConfirm;
            setState_runCommand("Nothing,1");
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
                    m_releaseCount++;
                    emit printLog(GetCurrentBoxPosString(m_releaseCount) + " cannot be released, was it an egg?", LOG_WARNING);

                    m_substage = SS_ReleaseConfirm;
                    setState_runCommand("A,25");
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("Unable to detect Yes/No box when releasing Pokemon");
                }
            }
            else if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                // shiny, don't release
                m_releaseCount++;
                emit printLog(GetCurrentBoxPosString(m_releaseCount) + " is a SHINY, not releasing", LOG_WARNING);

                m_substage = SS_ReleaseConfirm;
                setState_runCommand("B,30");
            }
            else if (checkBrightnessMeanTarget(A_No.m_rect, C_Color_Black, 200))
            {
                m_releaseCount++;
                emit printLog(GetCurrentBoxPosString(m_releaseCount) + " is released");

                m_substage = SS_ReleaseConfirm;
                setState_runCommand("DUp,1,ASpam,50");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ReleaseConfirm:
    {
        if (state == S_CommandFinished)
        {
            if (m_releaseCount / 30 == m_programSettings.m_targetReleaseBoxCount)
            {
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
            }
            else
            {
                m_substage = SS_ReleaseHasPokemon;
                m_videoManager->setAreas({A_DialogBox});

                if (m_releaseCount == 0)
                {
                    setState_runCommand("A,25", true);
                }
                else
                {
                    if (m_releaseCount % 30 == 0)
                    {
                        // next box
                        setState_runCommand("DRight,1,LRight,1,DDown,1,LDown,1,DDown,1,R,1,Nothing,5,A,25", true);
                    }
                    else if (m_releaseCount % 6 == 0)
                    {
                        // next row
                        setState_runCommand("DDown,1,A,25", true);
                    }
                    else if (((m_releaseCount % 30) / 6) % 2 == 0)
                    {
                        // odd row
                        setState_runCommand("DRight,1,A,25", true);
                    }
                    else
                    {
                        // even row
                        setState_runCommand("DLeft,1,A,25", true);
                    }
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

QPoint SmartEggOperation::GetCurrentBoxPosFromReleaseCount(int count)
{
    if (count < 1)
    {
        // wtf
        return QPoint(0,0);
    }

    // [0-29]
    count = (count - 1) % 30;

    int row = (count / 6) + 1;
    int column = (row % 2 == 0) ? column = 6 - (count % 6) : column = (count % 6) + 1;
    return QPoint(column, row);
}

QString SmartEggOperation::GetCurrentBoxPosString(int count)
{
    QPoint pos = GetCurrentBoxPosFromReleaseCount(count);
    return "Column " + QString::number(pos.x()) + " row " + QString::number(pos.y()) + " in Box " + QString::number(((count - 1) / 30) + 1);
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
