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
    m_initVerified = false;

    m_firstCollectCycle = true;
    resetCollectorModeMembers();
    resetHatcherModeMembers();
    m_fadeOutDelayTime = 0; // should not reset
    m_hatchExtraEgg = false;

    m_parentStat = PokemonStatTable();
    m_leaveParent = false;
    m_natureMatched = false;

    m_videoCaptured = false;
    m_shinySoundID = 0;
    m_shinyDetected = false;
    m_shinyWasDetected = 0;
    m_shinyCount = 0;
    m_keepCount = 0;

    // extra pokemon we want to keep
    m_hatchedStat = PokemonStatTable();
    if (m_programSettings.m_operation != EggOperationType::EOT_Collector)
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

        // need to check gender in parent mode if parent in nursery isn't ditto
        if (m_programSettings.m_operation == EggOperationType::EOT_Parent && m_programSettings.m_parentGender != GenderType::GT_Any)
        {
            m_keepDummy.m_gender = GenderType::GT_Male;
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
                    emit printLog(">>>>> Keep Slot " + QString::number(i+1) + ": " + QString::number(table.m_target) + " remaining");
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
            if (m_programSettings.m_operation == EggOperationType::EOT_Shiny && m_keepList.isEmpty())
            {
                incrementStat(m_statError);
                setState_error("No Pokemon to keep is set");
                break;
            }

            if (m_programSettings.m_operation == EggOperationType::EOT_Remainder && m_programSettings.m_shinyDetection == ShinyDetectionType::SDT_Delay)
            {
                incrementStat(m_statError);
                setState_error("Fade Out Delay is not able in Remainder Mode");
                break;
            }

            if (m_programSettings.m_operation == EggOperationType::EOT_Parent && m_keepList.size() != 1)
            {
                incrementStat(m_statError);
                setState_error("Parent Mode expected only 1 Pokemon in keep list");
                break;
            }

            if (m_programSettings.m_operation == EggOperationType::EOT_Collector)
            {
                // force disable shiny detection
                m_programSettings.m_shinyDetection = ShinyDetectionType::SDT_Disable;
            }
            else if (m_programSettings.m_shinyDetection == ShinyDetectionType::SDT_Sound)
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
            case EggOperationType::EOT_Collector:
            {
                if (countPair.second == 5)
                {
                    // we don't care about box view, go to collect eggs now
                    emit printLog("Full party confirmed");
                    m_initVerified = true;
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
            case EggOperationType::EOT_Hatcher:
            case EggOperationType::EOT_Remainder:
            {
                if (countPair.second == 0)
                {
                    emit printLog("Only 1 Pokemon in party confirmed");
                    if (m_programSettings.m_operation == EggOperationType::EOT_Hatcher)
                    {
                        m_initVerified = false;
                        m_substage = SS_ToBox;
                        setState_runCommand(C_PartyToBox);
                    }
                    else
                    {
                        // we don't care about box view in remainder mode
                        m_initVerified = true;
                        m_hatchExtraEgg = true;
                        m_programSettings.m_isHatchExtra = true;

                        m_substage = SS_CollectCycle;
                        setState_runCommand("BSpam,60");
                        m_videoManager->clearCaptures();
                    }
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("There should only 1 Pokemon with Flame Body in the team in Hatcher/Remainder Mode");
                }
                break;
            }
            case EggOperationType::EOT_Shiny:
            case EggOperationType::EOT_Parent:
            {
                if (countPair.second == 5)
                {
                    emit printLog("Full party confirmed");
                    m_initVerified = false;

                    if (m_programSettings.m_operation == EggOperationType::EOT_Parent && m_keepList[0].m_nature != NatureType::NT_Any)
                    {
                        m_substage = SS_InitCheckItem;
                        setState_runCommand("X,1,Nothing,20");
                        m_videoManager->setAreas({GetPartyItemCaptureAreaOfPos(1)});
                    }
                    else
                    {
                        m_substage = SS_ToBox;
                        setState_runCommand(C_PartyToBox);
                    }
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("There should be a full party while using Shiny/Parent Mode");
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
    case SS_InitCheckItem:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(GetPartyItemCaptureAreaOfPos(1).m_rect, C_Color_Item, 190))
            {
                emit printLog("First Pokemon in party holding an item (should be Everstone) confirmed");
                m_initVerified = false;
                m_substage = SS_ToBox;
                setState_runCommand("B,2,Loop,1," + m_commands[C_PartyToBox]);
                m_videoManager->clearCaptures();
            }
            else
            {
                incrementStat(m_statError);
                setState_error("Parent Mode expected first Pokemon in party holding an Everstone");
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
                switch (m_programSettings.m_operation)
                {
                case EggOperationType::EOT_Hatcher:
                {
                    m_initVerified = true;
                    m_substage = SS_ToBox;
                    setState_runCommand("DRight,1");
                    m_videoManager->clearCaptures();
                    break;
                }
                case EggOperationType::EOT_Shiny:
                {
                    // now check if box has 25 pokmon
                    m_substage = SS_InitEmptyColumnStart;
                    setState_runCommand("DRight,1,Nothing,20");
                    m_videoManager->setAreas({A_Level});
                    break;
                }
                case EggOperationType::EOT_Parent:
                {
                    // now check if box has 25 pokmon (not include parent)
                    m_substage = SS_InitEmptyColumnStart;
                    setState_runCommand("DRight,1,LDown,1,Nothing,20");
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
            QString command = m_programSettings.m_operation == EggOperationType::EOT_Shiny ? "DDown,1,LDown,1,Loop,2" : "DDown,1,LDown,1,DDown,1";
            setState_runCommand(command + ",L,1,Nothing,5,Loop,1,"
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
            if (m_programSettings.m_operation == EggOperationType::EOT_Parent)
            {
                // check parent's current stats
                m_substage = SS_CheckStats;
                setState_runCommand("DLeft,1,LUp,1,DUp,1,LUp,1,DUp,1,Nothing,20");
                m_videoManager->setAreas(GetCheckStatCaptureAreas());
            }
            else
            {
                // we can finally start
                m_initVerified = true;
                m_programSettings.m_targetEggCount = 5;
                emit printLog("Keep Box and Egg Box setup correctly confirmed");

                m_substage = SS_CollectCycle;
                setState_runCommand("BSpam,80," + m_commands[m_firstCollectCycle ? C_CollectFirst : C_CollectCycle]);
                m_videoManager->clearCaptures();
            }
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
    case SS_InitCheckParent:
    {
        if (state == S_CaptureReady)
        {
            if (!checkBrightnessMeanTarget(A_Level.m_rect, C_Color_Grey, 190))
            {
                incrementStat(m_statError);
                setState_error("Parent not found at the top left of Box");
                break;
            }

            m_parentStat = m_hatchedStat;
            printPokemonStat(m_parentStat, 0);
            emit printLog("Parent stats saved");

            if (m_parentStat.Match(m_keepList[0]))
            {
                emit printLog("Parent already matched target Pokemon", LOG_WARNING);
                setState_completed();
                break;
            }

            // we can finally start
            m_initVerified = true;
            m_leaveParent = true;

            QString command = "BSpam,100";
            if (m_keepList[0].m_nature == m_parentStat.m_nature)
            {
                // transfer Everstone to parent
                emit printLog("Nature matched, moving Everstone to parent", LOG_SUCCESS);
                command = m_commands[C_MoveItem] + "," + command;
            }

            m_substage = SS_CollectCycle;
            setState_runCommand(command);
            m_videoManager->clearCaptures();
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
                // when swapping parent (m_hatchExtraEgg = true) we need to not trigger this error
                if (m_leaveParent && !m_hatchExtraEgg)
                {
                    incrementStat(m_statError);
                    setState_error("Attempting to leave parent to Nursery but there is an egg");
                    break;
                }

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

                    // add delay to prevent false dialog detection
                    setState_runCommand(m_commands[C_CollectEgg] + ",Nothing,20");
                }
                else
                {
                    emit printLog("Egg collected! (" + QString::number(m_programSettings.m_targetEggCount - m_eggsCollected) + " remaining)");
                    setState_runCommand(C_CollectEgg);
                }
            }
            else
            {
                // take parent back so it doesn't make anymore eggs
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
                        // no remaining egg to hatch, go to box with multiselect cursor
                        m_substage = SS_HatchComplete;
                        setState_runCommand(m_commands[C_TakeParent] + "," + m_commands[C_ToBox] + ",Nothing,30,Loop,1,Y,1,Nothing,1,Loop,2");
                    }
                    break;
                }

                // leave parent
                if (m_leaveParent)
                {
                    m_leaveParent = false;
                    if (!checkBrightnessMeanTarget(A_Nursery1st.m_rect, C_Color_Black, 180))
                    {
                        incrementStat(m_statError);
                        setState_error("Excepting cursor at \"I'd like to leave Pokemon\"");
                        break;
                    }

                    // we can now confirm the nature is matched
                    if (m_keepList[0].m_nature == NatureType::NT_Any || m_keepList[0].m_nature == m_parentStat.m_nature)
                    {
                        m_natureMatched = true;
                    }

                    m_programSettings.m_targetEggCount = 5;
                    emit printLog("Leaving new parent in Nursery and collecting more eggs");

                    m_substage = SS_CollectCycle;
                    setState_runCommand(m_commands[C_LeaveParent] + "," + m_commands[C_CollectFirst]);
                    m_videoManager->clearCaptures();
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
                if (m_programSettings.m_operation == EggOperationType::EOT_Shiny || m_programSettings.m_operation == EggOperationType::EOT_Parent)
                {
                    m_programSettings.m_columnsToHatch = 1;
                    resetHatcherModeMembers();

                    m_substage = SS_BoxFiller;
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
                        emit printLog("Putting 5 filler Pokemon to Keep Box");
                        m_substage = SS_ToBox;
                        setState_runCommand(C_BoxFiller);
                    }
                    else if (!m_initVerified)
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
                else if ((m_programSettings.m_operation == EggOperationType::EOT_Shiny || m_programSettings.m_operation == EggOperationType::EOT_Parent) && m_eggsToHatchColumn < 5)
                {
                    incrementStat(m_statError);
                    setState_error("Shiny/Parent Mode always expect to have 5 eggs, something went really wrong");
                }
                else
                {
                    emit printLog("Hatching column " + QString::number(m_eggColumnsHatched + 1) + " with " + QString::number(m_eggsToHatchColumn) + " eggs");
                    m_timer.restart();
                    m_eggsToHatchCount = 0;
                    m_blackScreenDetected = false;

                    m_substage = SS_HatchCycle;
                    setState_runCommand("BSpam,60");
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

            // cache image to send if shiny
            m_videoManager->getFrame(m_hatchImage);

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
                case ShinyDetectionType::SDT_Delay:
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
                case ShinyDetectionType::SDT_Sound:
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

                QString command = m_commands[C_HatchReturn];
                if (videoCapture)
                {
                    // add delay after capture as it can freeze the game
                    command = "Capture,22,Nothing,300," + command;
                }
                setState_runCommand(command, m_eggsToHatchCount < m_eggsToHatchColumn);

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
                QString log = "OCR unable the match any IV entries";
                if (!PokemonDatabase::normalizeString(getOCRStringRaw()).isEmpty())
                {
                    log += ", the returned text \"" + getOCRStringRaw() + "\" needs to be added to database, please report to brianuuuSonic";
                }
                emit printLog(log, LOG_ERROR);
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

            if (m_programSettings.m_operation == EggOperationType::EOT_Parent && !m_initVerified)
            {
                m_substage = SS_InitCheckParent;
            }
            else
            {
                m_substage = SS_CheckStats;
            }

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

            // Discord message
            Discord::EmbedField embedField;
            embedField.setName("Keep List");
            QString fieldMsg;
            for (int i = 0; i < m_keepList.size(); i++)
            {
                PokemonStatTable const & table = m_keepList[i];
                fieldMsg += (i == 0) ? "" : "\n";
                fieldMsg += "Slot " + QString::number(i+1) + ": " + QString::number(table.m_target) + " remaining";
            }
            embedField.setValue(fieldMsg);

            // print current stats
            if (checkedStats)
            {
                if (m_keepList.size() == 1)
                {
                    // if there's only one target, always compare with it
                    printPokemonStat(m_hatchedStat, 0);
                }
                else
                {
                    printPokemonStat(m_hatchedStat, matchedTarget);
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

                // send discord message
                if (m_programSettings.m_shinyDetection == ShinyDetectionType::SDT_Disable)
                {
                    m_videoManager->getFrame(m_hatchImage);
                }
                sendDiscordMessage("Shiny Found!", true, QColor(255,255,0), &m_hatchImage, {embedField});
            }

            m_eggsToHatchCount--;
            if (matchedTarget >= 0)
            {
                if (isShiny)
                {
                    emit printLog(log + " is SHINY!!!", LOG_SUCCESS);
                }
                else
                {
                    // keeping non-shiny, report
                    m_videoManager->getFrame(m_hatchImage);
                    sendDiscordMessage("Target Pokemon Found!", true, QColor(255,255,0), &m_hatchImage, {embedField});
                }

                if (m_programSettings.m_operation == EggOperationType::EOT_Parent)
                {
                    // this is the target parent we want, just move it to top right position
                    emit printLog("Target Parent is found! Moving it to top left of Box!");
                    m_substage = SS_KeepPokemon;
                    setState_runCommand(C_SetParent);
                    break;
                }

                emit printLog(log + " matched target Pokemon at slot " + QString::number(matchedTarget + 1) + "! Moving it to Keep Box! " + QString::number(m_keepList[matchedTarget].m_target) + " remaining", LOG_SUCCESS);
                runKeepPokemonCommand();
                break;
            }
            else if (isShiny)
            {
                emit printLog(log + " is SHINY!!! Moving it to Keep Box!", LOG_SUCCESS);
                runKeepPokemonCommand();
                break;
            }
            else if (m_programSettings.m_operation == EggOperationType::EOT_Parent && m_keepList[0].m_target > 0)
            {
                if (m_programSettings.m_parentGender != GenderType::GT_Any && m_programSettings.m_parentGender == m_hatchedStat.m_gender)
                {
                    // reject wrong gender, otherwise we might not be able to get any eggs
                    emit printLog(log + " is not with correct gender and not shiny, releasing");
                    m_substage = SS_ReleaseHasPokemon;
                    setState_runCommand("A,25", true);
                    m_videoManager->setAreas({A_DialogBox});
                    break;
                }

                if (!m_natureMatched)
                {
                    // nature not matched yet, this takes priority over IVs, will swap even if it has fewer matching IVs
                    if (m_keepList[0].m_nature == m_hatchedStat.m_nature)
                    {
                        // this pokemon has matching nature
                        if (m_keepList[0].m_nature != m_parentStat.m_nature)
                        {
                            // this is a new parent, move it to top left position!
                            m_leaveParent = true;
                            m_parentStat = m_hatchedStat;
                            emit printLog("Nature matched, moving parent to top left position for standby", LOG_SUCCESS);

                            m_substage = SS_KeepPokemon;
                            setState_runCommand(C_SetParent);
                            break;
                        }
                        else if (m_keepList[0].Compare(m_parentStat, m_hatchedStat))
                        {
                            // we just found a parent with correct nature in the same column of eggs
                            // but this one has better stats, replace and release old parent
                            m_leaveParent = true;
                            m_parentStat = m_hatchedStat;
                            emit printLog("Nature matched and has more matching stats, swapping with parent at top left position and releasing old one", LOG_SUCCESS);

                            m_substage = SS_ReleaseHasPokemon;
                            setState_runCommand(m_commands[C_SetParent] + ",A,25", true);
                            m_videoManager->setAreas({A_DialogBox});
                            break;
                        }
                    }

                    if (m_keepList[0].m_nature == m_parentStat.m_nature)
                    {
                        // we need to prevent any Pokemon with better stats but not with correct nature to replace the one we just found!
                        emit printLog("Found parent with correct nature, preventing other Pokemon with better IV stats to override it");
                        emit printLog(log + " is not shiny, releasing");
                        m_substage = SS_ReleaseHasPokemon;
                        setState_runCommand("A,25", true);
                        m_videoManager->setAreas({A_DialogBox});
                        break;
                    }
                }
                else if (m_keepList[0].m_nature != NatureType::NT_Any && m_keepList[0].m_nature != m_hatchedStat.m_nature)
                {
                    // verify nature stay the same
                    incrementStat(m_statError);
                    setState_error("Parent in Nursery is presumably holding an Everstone but child does not have matching nature?");
                    break;
                }

                // compare other stats
                if (m_keepList[0].Compare(m_parentStat, m_hatchedStat))
                {
                    m_parentStat = m_hatchedStat;
                    if (m_leaveParent)
                    {
                        emit printLog("Found parent with even more matching stats, swapping with parent at top left position and releasing old one", LOG_SUCCESS);
                        m_substage = SS_ReleaseHasPokemon;
                        setState_runCommand(m_commands[C_SetParent] + ",A,25", true);
                        m_videoManager->setAreas({A_DialogBox});
                    }
                    else
                    {
                        m_leaveParent = true;
                        emit printLog("Found parent with more matching stats, moving to top left position for standby", LOG_SUCCESS);

                        m_substage = SS_KeepPokemon;
                        setState_runCommand(C_SetParent);
                    }
                    break;
                }
            }

            emit printLog(log + " is not shiny, releasing");
            m_substage = SS_ReleaseHasPokemon;
            setState_runCommand("A,25", true);
            m_videoManager->setAreas({A_DialogBox});
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

                // now in multiselect mode
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
                        emit printLog(QString::number(m_keepCount - m_shinyCount) + " non-Shiny Pokemon has been stored in Keep Box", LOG_SUCCESS);
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
                    emit printLog(">>>>> Keep Slot " + QString::number(i+1) + ": " + QString::number(table.m_target) + " remaining");
                    if (table.m_target > 0)
                    {
                        missingTarget = true;
                    }
                }

                // we need to leave a new parent
                if (m_programSettings.m_operation == EggOperationType::EOT_Parent && m_leaveParent)
                {
                    if (m_keepList[0].m_nature == m_parentStat.m_nature)
                    {
                        // new parent with correct nature waiting at top left position
                        if (!m_natureMatched)
                        {
                            if (m_hatchExtraEgg)
                            {
                                // ----------------PARENT SWAP STEP 2----------------
                                // finished hatching remining egg and old parent is in party slot 2, put old parent to keep box, then leave new parent to nursery
                                m_substage = SS_KeepOldParent;
                                setState_runCommand("Y,1,DLeft,1,LDown,1");
                                break;
                            }

                            // ----------------PARENT SWAP STEP 1----------------
                            // transfer Everstone from first slot in party to parent
                            emit printLog("Transfering Everstone from 1st slot party Pokemon to new parent");
                            m_programSettings.m_isHatchExtra = true;

                            // take old parent from nursery and hatch remaining egg
                            m_substage = SS_HatchComplete;
                            setState_runCommand(C_MoveItem);
                            break;
                        }

                        // this parent has more matching stats
                        if (m_hatchExtraEgg)
                        {
                            // ----------------PARENT SWAP STEP 6----------------
                            // transfer Everstone from previous parent (slot 2) to new one, put old parent to keep box, then leave new parent to nursery
                            emit printLog("Transfering Everstone from old parent (2nd slot in party) to new parent");
                            m_substage = SS_KeepOldParent;
                            setState_runCommand(C_MoveItemParent);
                            //m_videoManager->setAreas({A_Bag});
                            break;
                        }
                    }

                    if (m_hatchExtraEgg)
                    {
                        // ----------------PARENT SWAP STEP 4----------------
                        // put old parent at slot 2 to keep box, then leave new parent to nursery
                        m_substage = SS_KeepOldParent;
                        setState_runCommand("Y,1,DLeft,1,LDown,1");
                        break;
                    }

                    // ----------------PARENT SWAP STEP 3/5----------------
                    // parent should already be on top right position, start taking old parent from nursery
                    m_programSettings.m_isHatchExtra = true;
                    m_substage = SS_HatchComplete;
                    setState_runCommand("Nothing,1");
                    break;
                }

                // continue collecting more eggs
                if ((m_programSettings.m_operation == EggOperationType::EOT_Shiny || m_programSettings.m_operation == EggOperationType::EOT_Parent) && missingTarget)
                {
                    emit printLog("Taking 5 filler Pokemon from Keep Box to party");
                    m_substage = SS_TakeFiller;
                    setState_runCommand(C_TakeFiller);
                    break;
                }

                // For parent mode after finishing, always take parent out
                if (m_programSettings.m_operation == EggOperationType::EOT_Parent)
                {
                    m_programSettings.m_isHatchExtra = true;
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
    case SS_KeepOldParent:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            /*
            if (checkBrightnessMeanTarget(A_Bag.m_rect, C_Color_Item, 210))
            {
                incrementStat(m_statError);
                setState_error("Entered Bag menu, the current Pokemon is expected to be holding an item but it is not");
                break;
            }
            */

            emit printLog("Putting old parent to Keep Box");
            runKeepPokemonCommand();

            // set stage after since runKeepPokemonCommand() sets it too
            m_substage = SS_HatchComplete;
        }
        break;
    }
    case SS_HatchComplete:
    {
        if (state == S_CommandFinished)
        {
            if (m_programSettings.m_isHatchExtra && !m_hatchExtraEgg)
            {
                m_hatchExtraEgg = true;
                resetCollectorModeMembers();

                m_substage = SS_CollectCycle;
                setState_runCommand("BSpam,100");
            }
            else
            {
                // force param to reset (mainly for parent mode)
                m_programSettings.m_isHatchExtra = false;
                m_hatchExtraEgg = false;

                if (m_leaveParent)
                {
                    // about to leave new parent, fill party with 6 pokemon to prepare collecting eggs
                    emit printLog("Taking 5 filler Pokemon from Keep Box to party");
                    m_substage = SS_TakeFiller;
                    setState_runCommand("Y,1,DUp,1,Y,1,DRight,1," + m_commands[C_TakeFiller]);
                    break;
                }

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
            if (!m_leaveParent)
            {
                emit printLog("Collecting more eggs...");
            }
            resetCollectorModeMembers();

            // go back to collecting eggs
            m_substage = SS_CollectCycle;
            setState_runCommand("BSpam,100");
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

void SmartEggOperation::printPokemonStat(const PokemonStatTable &table, int matchID)
{
    QString stats = "Stats:<br>";
    PokemonStatTable const* matchTable = (matchID >= 0 && matchID < m_keepList.size()) ? &m_keepList[matchID] : nullptr;

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
            bool matched = matchTable && matchTable->m_ivs[i] == type;
            if (matched) stats += "<font color=\"#FF00AA00\">";
            stats += PokemonDatabase::getIVTypeName(type);
            if (matched) stats += "</font>";
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
        bool matched = matchTable && matchTable->m_nature == nature;
        if (matched) stats += "<font color=\"#FF00AA00\">";
        stats += PokemonDatabase::getNatureTypeName(nature, false);
        if (matched) stats += "</font>";
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
        bool matched = matchTable && matchTable->m_gender == gender;
        if (matched) stats += "<font color=\"#FF00AA00\">";
        stats += PokemonDatabase::getGenderTypeName(gender);
        if (matched) stats += "</font>";
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
    else
    {
        bool matched = matchTable && (matchTable->m_shiny == shiny || (matchTable->m_shiny == ShinyType::SPT_Yes && (shiny == ShinyType::SPT_Star || shiny == ShinyType::SPT_Square)));
        if (matched) stats += "<font color=\"#FF00AA00\">";
        if (shiny == ShinyType::SPT_No)
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
        if (matched) stats += "</font>";
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

const CaptureArea SmartEggOperation::GetPartyItemCaptureAreaOfPos(int y)
{
    if (y < 1 || y > 6)
    {
        y = 1;
    }

    y--;
    return CaptureArea(146, 156 + y * 96, 300, 40);
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
