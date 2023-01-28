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

    m_firstCollectCycle = true;
    resetCollectorModeMembers();
    resetHatcherModeMembers();
    m_fadeOutDelayTime = 0; // should not reset
    m_hatchExtraEgg = false;

    m_videoCaptured = false;
    m_shinySoundID = 0;
    m_shinyDetected = false;
    m_shinyWasDetected = 0;
    m_shinyCount = 0;
    m_keepCount = 0;

    // extra pokemon we want to keep
    m_hatchedStat = PokemonStatTable();
    if (m_programSettings.m_operation != EOT_Collector)
    {
        m_keepList = m_programSettings.m_statTable->GetTableList();
        updateKeepDummy();
    }
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
    m_keepSingleCount = 0;
}

void SmartEggOperation::updateKeepDummy()
{
    m_keepDummy = PokemonStatTable();
    for (PokemonStatTable const& table : m_keepList)
    {
        if (table.m_target <= 0)
        {
            continue;
        }

        for (int i = 0; i < StatType::ST_COUNT; i++)
        {
            if (table.m_ivs[i] != IVType::IVT_Any)
            {
                m_keepDummy.m_ivs[i] = table.m_ivs[i];
            }
        }
        if (table.m_nature != NatureType::NT_Any)
        {
            m_keepDummy.m_nature = table.m_nature;
        }
        if (table.m_gender != GenderType::GT_Any)
        {
            m_keepDummy.m_gender = table.m_gender;
        }
        if (table.m_shiny != ShinyType::SPT_Any)
        {
            m_keepDummy.m_shiny = table.m_shiny;
        }
    }
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

        if (!m_keepList.isEmpty())
        {
            emit printLog("Keep Pokemon List:");
            for (PokemonStatTable const& table : m_keepList)
            {
                printPokemonStat(table);
            }

            for (int i = 0; i < m_keepList.size(); i++)
            {
                PokemonStatTable const& table = m_keepList[i];
                if (table.m_target > 0)
                {
                    emit printLog("----------Keep Slot " + QString::number(i+1) + ": " + QString::number(table.m_target) + " remaining----------");
                }
            }
        }

        bool testAfterHatched = false;
        bool testHatchExtra = false;
        if (testAfterHatched)
        {
            m_eggColumnsHatched = 1;
            m_eggsToHatchCount = 5;
            m_eggsToHatchColumn = 5;

            if (m_keepDummy.m_gender == GenderType::GT_COUNT)
            {
                m_substage = SS_ToBox;
                setState_runCommand(C_ToBox);
            }
            else
            {
                // need to check gender
                m_substage = SS_CheckGender;
                setState_runCommand(C_ToParty);
                m_videoManager->setAreas(GetPartyGenderCaptureAreas(m_eggsToHatchColumn));
            }
        }
        else if (testHatchExtra)
        {
            m_hatchExtraEgg = true;
            m_substage = SS_CollectCycle;
            setState_runCommand("Nothing,1");
        }
        else
        {
            if (m_programSettings.m_operation == EOT_Shiny && m_keepList.isEmpty())
            {
                incrementStat(m_statError);
                setState_error("No Pokemon to keep is set");
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
            setState_runCommand(C_ToParty);
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
                incrementStat(m_statError);
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
                    emit printLog("Full party confirmed");
                    m_boxViewChecked = true;
                    m_substage = SS_CollectCycle;
                    setState_runCommand("BSpam,40," + m_commands[m_firstCollectCycle ? C_CollectFirst : C_CollectCycle]);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("There should be a full party while using Collector Mode");
                }
                break;
            }
            case EOT_Hatcher:
            {
                if (countPair.second == 0)
                {
                    emit printLog("Only 1 Pokemon in party confirmed");
                    m_boxViewChecked = false;
                    m_substage = SS_ToBox;
                    setState_runCommand(C_PartyToBox);
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("There should only 1 Pokemon with Flame Body in the team in Hatcher Mode");
                }
                break;
            }
            case EOT_Shiny:
            {
                if (countPair.second == 5)
                {
                    emit printLog("Full party confirmed");
                    m_boxViewChecked = false;
                    m_substage = SS_ToBox;
                    setState_runCommand(C_PartyToBox);
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("There should be a full party while using Shiny Mode");
                }
                break;
            }
            default:
            {
                incrementStat(m_statError);
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
                // viewing judge function
                emit printLog("Box at Judge View confirmed");
                m_boxViewChecked = true;

                switch (m_programSettings.m_operation)
                {
                case EOT_Hatcher:
                {
                    m_substage = SS_ToBox;
                    setState_runCommand("DRight,1");
                    m_videoManager->clearCaptures();
                    break;
                }
                case EOT_Shiny:
                {
                    // now check if box has 25 pokmon
                    m_substage = SS_InitEmptyColumnStart;
                    setState_runCommand("DRight,1,Nothing,20");
                    m_videoManager->setAreas({A_Level});
                    break;
                }
                default:
                {
                    incrementStat(m_statError);
                    setState_error("Unhandled operation type");
                    break;
                }
                }
            }
        }
        break;
    }
    case SS_InitEmptyColumnStart:
    {
        if (state == S_CommandFinished)
        {
            // check keep box first column of egg box is empty
            m_substage = SS_InitEmptyColumn;
            setState_runCommand("DDown,1,LDown,1,Loop,2,L,1,Nothing,5,Loop,1,"
                                "DRight,1,LRight,1,DRight,1,LRight,1,DRight,1,LUp,1,"
                                "DLeft,1,LLeft,1,DLeft,1,LLeft,1,DLeft,1,LUp,1,Loop,2,"
                                "DRight,1,LRight,1,DRight,1,LRight,1,DRight,1,Nothing,20",true);
        }
        break;
    }
    case SS_InitEmptyColumn:
    {
        if (state == S_CommandFinished)
        {
            // go back to egg box, cursor should now be on top right
            m_substage = SS_InitOtherColumnsStart;
            setState_runCommand("R,1,Nothing,20");
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Level.m_rect, C_Color_Grey, 190))
            {
                // first column should be empty
                incrementStat(m_statError);
                emit printLog("First column of Egg Box and all columns of Keep Box should be empty in Shiny Mode", LOG_ERROR);

                m_substage = SS_Finished;
                setState_runCommand("Nothing,10");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_InitOtherColumnsStart:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_InitOtherColumns;
            setState_runCommand("DLeft,1,LLeft,1,DLeft,1,LLeft,1,DDown,1,"
                                "LRight,1,DRight,1,LRight,1,DRight,1,LDown,1,Loop,2,"
                                "DLeft,1,LLeft,1,DLeft,1,LLeft,1,Nothing,20",true);
        }
        break;
    }
    case SS_InitOtherColumns:
    {
        if (state == S_CommandFinished)
        {
            // we can finally start
            m_programSettings.m_targetEggCount = 5;
            emit printLog("Keep Box and Egg Box setup correctly confirmed");

            m_substage = SS_CollectCycle;
            setState_runCommand("BSpam,80," + m_commands[m_firstCollectCycle ? C_CollectFirst : C_CollectCycle]);
            m_videoManager->clearCaptures();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Level.m_rect, C_Color_Grey, 190))
            {
                setState_frameAnalyzeRequest();
            }
            else
            {
                // other column should NOT be empty
                incrementStat(m_statError);
                emit printLog("2nd to 6th column of Box should NOT be empty in Shiny Mode", LOG_ERROR);

                m_substage = SS_Finished;
                setState_runCommand("Nothing,10");
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

            m_firstCollectCycle = false;
            m_talkDialogAttempts++;
            m_timer.restart();
            setState_runCommand("A,20,Nothing,20", true);
            m_videoManager->setAreas({A_Nursery1st,A_Nursery2nd,A_Yes,A_YesNo});
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
            if (checkBrightnessMeanTarget(A_Yes.m_rect, C_Color_Black, 180))
            {
                // Collect egg
                m_eggsCollected++;
                m_eggCollectAttempts = 0;
                incrementStat(m_statEggCollected);

                if (m_hatchExtraEgg)
                {
                    // immediately talk to nursey again to take one parent away
                    m_substage = SS_CollectCycle;
                    emit printLog("Remaining Egg collected!");

                    m_programSettings.m_columnsToHatch = 1;
                    resetHatcherModeMembers();
                    m_eggsToHatchColumn = 1;
                }
                else
                {
                    emit printLog("Egg collected! (" + QString::number(m_programSettings.m_targetEggCount - m_eggsCollected) + " remaining)");
                }
                setState_runCommand(C_CollectEgg);
            }
            else
            {
                if (m_hatchExtraEgg)
                {
                    if (!checkBrightnessMeanTarget(A_Nursery2nd.m_rect, C_Color_Black, 180))
                    {
                        incrementStat(m_statError);
                        setState_error("Excepting cursor at \"I'd like to take my Pokemon back\"");
                        break;
                    }

                    emit printLog("Taking one parent back from Nursery");
                    if (m_eggsToHatchColumn > 0)
                    {
                        // hatch the remaining egg
                        m_substage = SS_HatchCycle;
                        setState_runCommand(C_TakeParent);
                    }
                    else
                    {
                        // no remaining egg to hatch
                        m_substage = SS_HatchComplete;
                        setState_runCommand(m_commands[C_TakeParent] + "," + m_commands[C_ToBox] + ",Nothing,30,Loop,1,Y,1,Nothing,1,Loop,2");
                    }
                    break;
                }

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
                    m_programSettings.m_columnsToHatch = 1;
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
                    // go to 2nd slot of the party
                    m_substage = SS_CheckStats;
                    setState_runCommand("DLeft,1,DDown,1,Nothing,20");
                    m_videoManager->setAreas(GetCheckStatCaptureAreas());
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
                else if (m_programSettings.m_operation == EOT_Shiny && m_eggsToHatchColumn < 5)
                {
                    incrementStat(m_statError);
                    setState_error("Shiny Mode always expect to have 5 eggs, something went really wrong");
                }
                else
                {
                    emit printLog("Hatching column " + QString::number(m_eggColumnsHatched + 1) + " with " + QString::number(m_eggsToHatchColumn) + " eggs");
                    m_timer.restart();
                    m_eggsToHatchCount = 0;
                    m_blackScreenDetected = false;

                    m_substage = SS_HatchCycle;
                    setState_runCommand("BSpam,40");
                    m_videoManager->clearCaptures();
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
                m_hatchedGenders.clear();
                m_hatchedStat = PokemonStatTable();

                if (m_keepDummy.m_gender == GenderType::GT_COUNT)
                {
                    m_substage = SS_ToBox;
                    setState_runCommand(C_ToBox);
                }
                else
                {
                    // need to check gender
                    m_substage = SS_CheckGender;
                    setState_runCommand(C_ToParty);
                    m_videoManager->setAreas(GetPartyGenderCaptureAreas(m_eggsToHatchColumn));
                }
            }
            else
            {
                if (m_hatchExtraEgg)
                {
                    emit printLog("Hatching remaining egg");
                }

                // this command never returns finished (loop 0)
                setState_runCommand(C_HatchCycle, true);
                m_videoManager->setAreas({A_Dialog});
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
                QString log = "Oh? Egg no." + QString::number(m_statEggHatched.first) + " is hatching!";
                if (!m_hatchExtraEgg)
                {
                    log += " (" + QString::number(m_eggsToHatchColumn - m_eggsToHatchCount) + " remaining)";
                }
                emit printLog(log);

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
                        // m_videoCaptured = true;
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
                    // add delay after capture as it can freeze the game
                    setState_runCommand("Capture,22,Nothing,300," + m_commands[C_HatchReturn], m_eggsToHatchCount < m_eggsToHatchColumn);
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
    case SS_CheckGender:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            m_hatchedGenders = checkGenderInParty(m_eggsToHatchColumn);
            m_videoManager->clearCaptures();

            // continue to box
            m_substage = SS_ToBox;
            setState_runCommand(C_PartyToBox);
        }
        break;
    }
    case SS_CheckHP:
    case SS_CheckAttack:
    case SS_CheckDefense:
    case SS_CheckSpAtk:
    case SS_CheckSpDef:
    case SS_CheckSpeed:
    {
        if (state == S_OCRReady)
        {
            GameLanguage const gameLanguage = m_settings->getGameLanguage();
            PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_PokemonIV(gameLanguage);
            if (entries.isEmpty())
            {
                incrementStat(m_statError);
                setState_error("Unable to create OCR database from PokemonIV");
                break;
            }

            QString result = matchStringDatabase(entries);
            if (result.isEmpty())
            {
                incrementStat(m_statError);
                emit printLog("OCR unable the match any IV entries, the returned text need to be added to database, please report to brianuuuSonic", LOG_ERROR);
            }
            else
            {
                IVType& type = m_hatchedStat.m_ivs[int(m_substage - SS_CheckHP + ST_HP)];
                if (result == "No Good")
                {
                    type = IVType::IVT_NoGood;
                }
                else if (result == "Decent")
                {
                    type = IVType::IVT_Decent;
                }
                else if (result == "Pretty Good")
                {
                    type = IVType::IVT_PrettyGood;
                }
                else if (result == "Very Good")
                {
                    type = IVType::IVT_VeryGood;
                }
                else if (result == "Fantastic")
                {
                    type = IVType::IVT_Fantastic;
                }
                else if (result == "Best")
                {
                    type = IVType::IVT_Best;
                }
                else
                {
                    incrementStat(m_statError);
                    emit printLog("Hyper trained Pokemon is unexpected", LOG_ERROR);
                }
            }

            goToNextCheckStatState(Substage(m_substage + 1));
        }
        break;
    }
    case SS_CheckNature:
    {
        if (state == S_CaptureReady)
        {
            auto natureStat = checkPokemonNatureInParty();
            m_hatchedStat.m_nature = PokemonDatabase::getNatureFromStats(natureStat.first, natureStat.second);
            if (m_hatchedStat.m_nature == NatureType::NT_COUNT)
            {
                incrementStat(m_statError);
                emit printLog("Error occured when detecting Pokemon's nature (" + PokemonDatabase::getStatTypeName(natureStat.first, false) + ","+ PokemonDatabase::getStatTypeName(natureStat.second, false) + ")", LOG_ERROR);
            }
            goToNextCheckStatState(SS_CheckShiny);
        }
        break;
    }
    case SS_CheckShiny:
    {
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                // TODO: square shiny?
                m_hatchedStat.m_shiny = ShinyType::SPT_Yes;
            }
            else
            {
                m_hatchedStat.m_shiny = ShinyType::SPT_No;
            }

            m_substage = SS_CheckStats;
            runNextStateContinue();
        }
        break;
    }
    case SS_CheckStats:
    {
        if (state == S_CommandFinished)
        {
            // add gender to current stat
            m_hatchedStat = PokemonStatTable();
            if (!m_hatchedGenders.isEmpty())
            {
                m_hatchedStat.m_gender = m_hatchedGenders.front();
                m_hatchedGenders.pop_front();
            }

            // jump to other states to check actual stats
            goToNextCheckStatState(SS_CheckHP);
        }
        else if (state == S_CaptureReady)
        {
            if (m_eggsToHatchCount < 1 && m_eggsToHatchCount > 5)
            {
                incrementStat(m_statError);
                setState_error("m_eggsToHatchCount is invalid, we should not end up here");
                break;
            }

            QString log;
            if (m_hatchExtraEgg)
            {
                log = "Remaining egg";
            }
            else
            {
                log = "Column " + QString::number(m_eggColumnsHatched) + " row " + QString::number(6 - m_eggsToHatchCount);
            }

            // did we checked any stats? if yes print stats
            bool checkedStats = false;
            for (int i = 0; i < StatType::ST_COUNT; i++)
            {
                if (m_keepDummy.m_ivs[i] != IVType::IVT_COUNT)
                {
                    checkedStats = true;
                }
            }
            if (m_keepDummy.m_nature != NatureType::NT_COUNT)
            {
                checkedStats = true;
            }
            if (m_keepDummy.m_gender != GenderType::GT_COUNT)
            {
                checkedStats = true;
            }
            if (checkedStats)
            {
                printPokemonStat(m_hatchedStat);
            }

            // check if any pokemon matched with the target list
            int matchedTarget = -1;
            for (int i = 0; i < m_keepList.size(); i++)
            {
                PokemonStatTable& table = m_keepList[i];
                if (table.m_target > 0 && m_hatchedStat.Match(table))
                {
                    if (--table.m_target == 0)
                    {
                        // this target is done, maybe we can skip some stat check?
                        updateKeepDummy();
                    }
                    matchedTarget = i;
                    break;
                }
            }

            // this is shiny!
            bool isShiny = m_hatchedStat.m_shiny == ShinyType::SPT_Yes || m_hatchedStat.m_shiny == ShinyType::SPT_Star || m_hatchedStat.m_shiny == ShinyType::SPT_Square;
            if (isShiny)
            {
                m_shinyCount++;
                m_shinySingleCount++;
                incrementStat(m_statShinyHatched);

                // to check if we correctly detected shiny icon in box
                if (m_shinyWasDetected > 0)
                {
                    m_shinyWasDetected--;
                }
            }

            if (matchedTarget >= 0)
            {
                if (isShiny)
                {
                    emit printLog(log + " is SHINY!!!", LOG_SUCCESS);
                }
                emit printLog(log + " matched target Pokemon at slot " + QString::number(matchedTarget + 1) + "! Moving it to keep box! " + QString::number(m_keepList[matchedTarget].m_target) + " remaining", LOG_SUCCESS);
                runKeepPokemonCommand();
            }
            else if (isShiny)
            {
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
                m_videoManager->setAreas(GetCheckStatCaptureAreas());
            }
            else
            {
                if (m_shinyWasDetected > 0)
                {
                    incrementStat(m_statError);
                    setState_error("There were " + QString::number(m_shinyWasDetected) + " SHINY Pokemon detected with sound/delay but are released due to unable to detect shiny icon in Box view...");
                    break;
                }

                m_substage = SS_NextColumn;
                setState_runCommand("Nothing,20,Y,1,DUp,1,Y,1,DRight,1");
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
                        emit printLog(QString::number(m_shinyCount) + " SHINY Pokemon has been found", LOG_SUCCESS);
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
                    if (m_keepSingleCount - m_shinySingleCount > 0)
                    {
                        emit printLog(QString::number(m_keepCount - m_shinyCount) + " non-Shiny Pokemon has been stored in keep box", LOG_SUCCESS);
                    }
                    else
                    {
                        emit printLog("No additional non-Shiny Pokemon is found");
                    }
                }
                else
                {
                    emit printLog("No Non-Shiny Pokemon is kept...");
                }

                // any target pokemon we haven't got?
                bool missingTarget = false;
                for (int i = 0; i < m_keepList.size(); i++)
                {
                    PokemonStatTable const& table = m_keepList[i];
                    emit printLog("----------Keep Slot " + QString::number(i+1) + ": " + QString::number(table.m_target) + " remaining----------");
                    if (table.m_target > 0)
                    {
                        missingTarget = true;
                    }
                }

                if (m_programSettings.m_operation == EOT_Shiny && missingTarget)
                {
                    emit printLog("Taking 5 filler Pokemon from keep box to party");
                    m_substage = SS_TakeFiller;
                    setState_runCommand(C_TakeFiller);
                    break;
                }

                m_substage = SS_HatchComplete;
                if (m_eggColumnsHatched > 6)
                {
                    // we are done, move back to first box
                    setState_runCommand("L,1,Nothing,5,Loop," + QString::number((m_eggColumnsHatched - 1) / 6));
                }
                else
                {
                    setState_runCommand("Nothing,1");
                }
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
    case SS_HatchComplete:
    {
        if (state == S_CommandFinished)
        {
            if (m_programSettings.m_isHatchExtra && !m_hatchExtraEgg)
            {
                m_substage = SS_CollectCycle;
                resetCollectorModeMembers();

                m_hatchExtraEgg = true;
                setState_runCommand("BSpam,80");
            }
            else
            {
                m_hatchExtraEgg = false;
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
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
            else if (checkBrightnessMeanTarget(A_No.m_rect, C_Color_Black, 180))
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
        else
        {
            // no more pokemon below
            break;
        }
    }

    return EggPokeCountPair(eggCount, pokemonCount);
}

QVector<GenderType> SmartEggOperation::checkGenderInParty(int count)
{
    QVector<GenderType> types;
    for (int i = 2; i <= 1 + count; i++)
    {
        if (checkPixelColorMatch(GetPartyCapturePointOfPos(i).m_point, QColor(253,253,253)))
        {
            if (checkBrightnessMeanTarget(GetPartyGenderCaptureAreaOfPos(i).m_rect, C_Color_Male, 130))
            {
                types.push_back(GenderType::GT_Male);
            }
            else if (checkBrightnessMeanTarget(GetPartyGenderCaptureAreaOfPos(i).m_rect, C_Color_Female, 130))
            {
                types.push_back(GenderType::GT_Female);
            }
            else
            {
                types.push_back(GenderType::GT_Any);
            }
        }
        else
        {
            // no more pokemon below
            break;
        }
    }

    return types;
}

void SmartEggOperation::runKeepPokemonCommand(int yPos)
{
    if (yPos < 2 || yPos > 6)
    {
        incrementStat(m_statError);
        setState_error("Invalid position when trying to keep Pokemon in party");
        return;
    }

    m_keepCount++;
    m_keepSingleCount++;
    incrementStat(m_statPokemonKept);

    int scrollCount = 1 + (m_eggColumnsHatched - 1) / 6;
    if (m_keepCount > 25)
    {
        // 1st keep box can only store 25 then 30 and so on
        scrollCount += (m_keepCount + 4) / 30;
    }

    QString command = "Y,1,A,3,Loop,1"; // pick up
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

void SmartEggOperation::goToNextCheckStatState(Substage target)
{
    for (int i = 0; i < ST_COUNT; i++)
    {
        if (target == SS_CheckHP + i)
        {
            if (m_keepDummy.m_ivs[i] == IVType::IVT_COUNT)
            {
                target = Substage(target + 1);
            }
            else
            {
                m_substage = target;
                setState_ocrRequest(GetBoxStatNumArea(StatType(i)).m_rect, C_Color_Text);
                return;
            }
        }
    }

    if (target == SS_CheckNature)
    {
        if (m_keepDummy.m_nature == NatureType::NT_COUNT)
        {
            target = SS_CheckShiny;
        }
        else
        {
            m_substage = target;
            setState_frameAnalyzeRequest();
            return;
        }
    }

    if (target == SS_CheckShiny)
    {
        m_substage = target;
        setState_frameAnalyzeRequest();
        return;
    }

    incrementStat(m_statError);
    setState_error("Invalid state when checking stats");
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

const QVector<CaptureArea> SmartEggOperation::GetPartyGenderCaptureAreas(int count)
{
    QVector<CaptureArea> areas;
    for (int i = 0; i < count; i++)
    {
        areas.push_back(GetPartyGenderCaptureAreaOfPos(i + 2));
    }

    return areas;
}

const QVector<CaptureArea> SmartEggOperation::GetCheckStatCaptureAreas()
{
    QVector<CaptureArea> areas;
    for (int i = 0; i < ST_COUNT; i++)
    {
        if (m_keepDummy.m_ivs[i] != IVType::IVT_COUNT)
        {
            areas.push_back(GetBoxStatNumArea(StatType(i)));
        }
    }

    if (m_keepDummy.m_nature != NatureType::NT_COUNT)
    {
        for (int i = ST_Attack; i < ST_COUNT; i++)
        {
            areas.push_back(GetBoxStatNameArea(StatType(i)));
        }
    }

    // always check shiny
    areas.push_back(A_Shiny);
    return areas;
}

QPair<StatType, StatType> SmartEggOperation::checkPokemonNatureInParty()
{
    StatType inc = ST_COUNT;
    StatType dec = ST_COUNT;

    for (int i = ST_Attack; i < ST_COUNT; i++)
    {
        StatType stat = StatType(i);
        if (inc == ST_COUNT && checkBrightnessMeanTarget(GetBoxStatNameArea(stat).m_rect, C_Color_NatureGood, 10))
        {
            inc = stat;
        }
        else if (dec == ST_COUNT && checkBrightnessMeanTarget(GetBoxStatNameArea(stat).m_rect, C_Color_NatureBad, 10))
        {
            dec = stat;
        }
    }

    return {inc,dec};
}

void SmartEggOperation::printPokemonStat(const PokemonStatTable &table)
{
    QString stats = "Stats:<br>";

    // IV
    for (int i = 0; i < StatType::ST_COUNT; i++)
    {
        IVType const& type = table.m_ivs[i];
        if (i > 0)
        {
            stats += " | ";
        }
        stats += PokemonDatabase::getStatTypeName(StatType(i), false) + ": ";
        if (type == IVType::IVT_COUNT)
        {
            stats += "???";
        }
        else if (type == IVType::IVT_Any)
        {
            stats += "Any";
        }
        else
        {
            stats += PokemonDatabase::getIVTypeName(type);
        }
    }

    // Nature
    stats += "<br>Nature: ";
    NatureType const& nature = table.m_nature;
    if (nature == NatureType::NT_COUNT)
    {
        stats += "???";
    }
    else if (nature == NatureType::NT_Any)
    {
        stats += "Any";
    }
    else
    {
        stats += PokemonDatabase::getNatureTypeName(nature, false);
    }

    // Gender
    stats += " | Gender: ";
    GenderType const& gender = table.m_gender;
    if (gender == GenderType::GT_COUNT)
    {
        stats += "???";
    }
    else if (gender == GenderType::GT_Any)
    {
        stats += "Any";
    }
    else
    {
        stats += PokemonDatabase::getGenderTypeName(gender);
    }

    // Shiny
    stats += " | Shiny: ";
    ShinyType const& shiny = table.m_shiny;
    if (shiny == ShinyType::SPT_COUNT)
    {
        incrementStat(m_statError);
        emit printLog("Shiny status unknown", LOG_ERROR);
        stats += "???";
    }
    else if (shiny == ShinyType::SPT_Any)
    {
        stats += "Any";
    }
    else if (shiny == ShinyType::SPT_No)
    {
        stats += "No";
    }
    else if (shiny == ShinyType::SPT_Yes)
    {
        stats += "Yes";
    }
    else
    {
        stats += PokemonDatabase::getShinyTypeName(shiny);
    }

    emit printLog(stats);
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

const CaptureArea SmartEggOperation::GetPartyGenderCaptureAreaOfPos(int y)
{
    if (y < 1 || y > 6)
    {
        y = 1;
    }

    y--;
    return CaptureArea(340, 124 + y * 96, 24, 24);
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
