#include "smartplanuggetfarmer.h"

SmartPLANuggetFarmer::SmartPLANuggetFarmer(bool isFindShiny, SmartProgramParameter parameter)
    : SmartProgramBase(parameter)
    , m_isFindShiny(isFindShiny)
{
    init();
}

void SmartPLANuggetFarmer::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartPLANuggetFarmer::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    //setImageMatchFromResource("PLA_AConfirm", m_imageMatch_AConfirm);
    setImageMatchFromResource("PLA_RoyalWyrdeer", m_imageMatch_RoyalWyrdeer);

    m_substageAfterCamp = SS_TalkToLaventon;
    m_searchSisterCount = 0;
    m_isFirstTimeSave = true;
    m_isCoin = false;
}

void SmartPLANuggetFarmer::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        /*m_isCoin = true;
        m_substage = SS_GetOnWyrdeer;
        setState_runCommand(m_isCoin ? C_GetOnWyrdeer : C_GetOnWyrdeerNoMove);*/

        /*m_elapsedTimer.restart();
        m_substage = SS_TalkToLaventon;
        setState_runCommand(C_TalkeToLaventon);*/

        initStat(m_statSearches, "Searches");
        initStat(m_statCharmFound, "Charm");
        initStat(m_statCoinFound, "Coin");
        initStat(m_statError, "Error");
        initStat(m_statShiny, "Shinies");

        // Setup sound detection
        m_shinyDetected = false;
        if (m_isFindShiny)
        {
            m_shinySoundID = m_audioManager->addDetection("PokemonLA/ShinySFX", 0.19f, 5000);
            connect(m_audioManager, &AudioManager::soundDetected, this, &SmartPLANuggetFarmer::soundDetected);
        }
        else
        {
            m_shinySoundID = 0;
        }

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
                m_videoManager->setAreas({A_Title});

                m_substage = SS_Title;
                m_isFirstTimeVillageReturn = true;
            }
        }
        break;
    }
    case SS_Title:
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_Title)
                {
                    emit printLog("Title detected!");
                    setState_runCommand("ASpam,60,Nothing,30");
                    m_substage = SS_GameStart;
                }
                else
                {
                    // If we restart from failing, no need to teleport to camp
                    if (!m_isFirstTimeSave)
                    {
                        if (m_isCoin)
                        {
                            // For Coin, just search Charm immediately
                            m_substage = SS_Save;
                            setState_runCommand("Nothing,20");
                        }
                        else
                        {
                            // For Charm, talk to Laventon immediately
                            m_elapsedTimer.restart();
                            m_substage = SS_TalkToLaventon;
                            setState_runCommand(C_TalkeToLaventon);
                        }
                    }
                    else
                    {
                        m_substageAfterCamp = SS_TalkToLaventon;
                        m_substage = SS_FlyToHeightCamp;
                        setState_runCommand(C_FlyToHeightCamp);
                    }

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
    case SS_FlyToHeightCamp:
    {
        if (state == S_CommandFinished)
        {
            // Find a black screen make sure this succeed
            m_elapsedTimer.restart();
            setState_frameAnalyzeRequest();

            m_videoManager->setAreas({A_BattleEnd});
        }
        else if (state == S_CaptureReady)
        {
            // Wait for black screen to appear
            if (m_elapsedTimer.elapsed() > 5000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to head to camp, might be targeted by Pokemon, restarting game", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);

                m_videoManager->clearCaptures();
            }
            else if(checkAverageColorMatch(A_BattleEnd.m_rect, QColor(0,0,0)))
            {
                m_videoManager->clearCaptures();
                switch (m_substageAfterCamp)
                {
                case SS_Save:
                {
                    m_substage = SS_Save;
                    setState_runCommand(C_Save);
                    break;
                }
                case SS_SelectWyrdeer:
                {
                    // For when failing to find Coin only
                    m_substage = SS_Save;
                    setState_runCommand("Nothing,50");
                    break;
                }
                case SS_TalkToLaventon:
                {
                    m_elapsedTimer.restart();
                    m_substage = SS_TalkToLaventon;
                    setState_runCommand(C_TalkeToLaventon);
                    break;
                }
                default:
                {
                    setState_error("Unknown state to goto after camp");
                    break;
                }
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_WaitTillMorning:
    {
        if (state == S_CommandFinished)
        {
            m_substageAfterCamp = SS_Save;

            m_substage = SS_FlyToHeightCamp;
            setState_runCommand(C_FlyToHeightCamp);
        }
        break;
    }
    case SS_Save:
    {
        if (state == S_CommandFinished)
        {
            // Toggle sister every time we save
            m_isCoin = !m_isCoin;

            m_isFirstTimeSave = false;

            m_searchRoyalCount = 0;
            m_substage = SS_SelectWyrdeer;
            setState_frameAnalyzeRequest();

            m_videoManager->setAreas({A_Royal});
        }
        break;
    }
    case SS_SelectWyrdeer:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkImageMatchTarget(A_Royal.m_rect, C_Color_Royal, m_imageMatch_RoyalWyrdeer, 0.5))
            {
                m_searchRoyalCount = 0;
                m_searchSisterCount++;
                incrementStat(m_statSearches);
                emit printLog(QString("Wyrdeer selected, now finding ") + (m_isCoin ? "Coin" : "Charm") + ": Attempt " + QString::number(m_statSearches.first));

                m_substage = SS_GetOnWyrdeer;
                setState_runCommand(m_isCoin ? C_GetOnWyrdeer : C_GetOnWyrdeerNoMove);

                m_videoManager->clearCaptures();
            }
            else
            {
                m_searchRoyalCount++;
                if (m_searchRoyalCount > 12)
                {
                    incrementStat(m_statError);
                    emit printLog("Unable to find Wyrdeer after 3 cycles, restarting game", LOG_ERROR);

                    m_substage = SS_Restart;
                    setState_runCommand(C_Restart);
                }
                else
                {
                    setState_runCommand("DLeft,1,Nothing,21");
                }
            }
        }
        break;
    }
    case SS_GetOnWyrdeer:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_FindSister;
            setState_runCommand(m_isCoin ? C_FindCoin : C_FindCharm, true);

            m_audioManager->startDetection(m_shinySoundID);
            m_videoManager->setAreas({A_Dialog, A_Royal});
        }
        break;
    }
    case SS_AlphaKnockOff:
    {
        if (state == S_CommandFinished)
        {
            incrementStat(m_statError);
            emit printLog("Got knocked off by Alpha Pokemon, restarting game", LOG_ERROR);

            m_substage = SS_Restart;
            setState_runCommand(C_Restart);

            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_FindSister:
    {
        if (state == S_CommandFinished)
        {
            if (m_isCoin)
            {
                emit printLog("Coin not found, returning camp...", LOG_WARNING);
                m_substageAfterCamp = SS_SelectWyrdeer;
            }
            else
            {
                emit printLog("Charm not found, returning camp and talk to Laventon...", LOG_WARNING);
                m_substageAfterCamp = SS_TalkToLaventon;
            }

            m_substage = SS_FlyToHeightCamp;
            setState_runCommand(C_FlyToHeightCamp);

            m_audioManager->stopDetection(m_shinySoundID);
            m_videoManager->clearCaptures();
        }
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 150))
            {
                // This will interrupt the currently running command
                m_substage = SS_StartBattle;
                setState_runCommand("Nothing,20");
                m_audioManager->stopDetection(m_shinySoundID);
            }
            else if (checkImageMatchTarget(A_Royal.m_rect, C_Color_RoyalWhite, m_imageMatch_RoyalWyrdeer, 0.5))
            {
                // This will interrupt the currently running command
                m_substage = SS_AlphaKnockOff;
                setState_runCommand("Nothing,20");
                m_audioManager->stopDetection(m_shinySoundID);
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_StartBattle:
    {
        if (state == S_CommandFinished)
        {
            incrementStat(m_isCoin ? m_statCoinFound : m_statCharmFound);
            emit printLog(QString("Found ") + (m_isCoin ? "Coin" : "Charm") + "! Starting battle...", LOG_SUCCESS);

            // Minus is added in case we found false positive
            m_elapsedTimer.restart();
            m_substage = SS_DuringBattle;
            setState_runCommand("Minus,1,ASpam,500");

            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_DuringBattle:
    {
        qint64 elapsed = m_elapsedTimer.elapsed();
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            if (elapsed > 20000)
            {
                m_videoManager->setAreas({A_BattleEnd});
            }
            else
            {
                emit printLog("Previous command has ended eariler than expected, delaying until 20s after battle started", LOG_ERROR);
            }
        }
        else if (state == S_CaptureReady)
        {
            if (elapsed > 120000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect battle end for too long, restarting game", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);

                m_videoManager->clearCaptures();
            }
            else if (elapsed > 20000 && checkAverageColorMatch(A_BattleEnd.m_rect, QColor(0,0,0)))
            {
                // TODO: it is possible we have switched to Braviary just before Coin
                emit printLog("Battle complete!");

                // Battle arena may have around, try to fly to safety
                m_substage = SS_AfterBattle;
                setState_runCommand(m_isCoin ? C_AfterBattle : C_AfterBattleCharm);

                m_audioManager->startDetection(m_shinySoundID);
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand("ASpam,30");
            }
        }
        break;
    }
    case SS_AfterBattle:
    {
        if (state == S_CommandFinished)
        {
            if (m_isCoin)
            {
                emit printLog("Returning camp, finding Charm next");
                m_substageAfterCamp = SS_Save;
            }
            else
            {
                emit printLog("Returning camp and talk to Laventon");
                m_substageAfterCamp = SS_TalkToLaventon;
            }

            m_substage = SS_FlyToHeightCamp;
            setState_runCommand(C_FlyToHeightCamp);

            m_audioManager->stopDetection(m_shinySoundID);
        }
        break;
    }
    case SS_TalkToLaventon:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            m_videoManager->setAreas({A_AConfirmReturn, A_AConfirmReport, A_PokedexProgress});
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 60000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to finish talking to Laventon for too long, restarting game", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);

                m_videoManager->clearCaptures();
            }
            else if (checkBrightnessMeanTarget(A_PokedexProgress.m_rect, C_Color_Dialog, 200) || checkBrightnessMeanTarget(A_AConfirmReport.m_rect, C_Color_AConfirmReturn, 160))
            {
                // Pokedex need to press A to exit
                setState_runCommand("A,20,Nothing,1");
            }
            else if (checkBrightnessMeanTarget(A_AConfirmReturn.m_rect, C_Color_AConfirmReturn, 160))
            {
                // Head to village and return
                emit printLog("Heading back to village...");
                m_substage = SS_LoadingToVillageStart;
                setState_runCommand(C_TalkeToLaventonFinish);

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand("B,20,Nothing,1");
            }
        }
        break;
    }
    case SS_LoadingToVillageStart:
    case SS_LoadingToObsidianStart:
    {
        if (state == S_CommandFinished)
        {
            m_elapsedTimer.restart();
            m_videoManager->setAreas({A_Loading});
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 5000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect loading screen for too long...");
                break;
            }
            else if (checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                // Detect loading screen
                m_substage = (m_substage == SS_LoadingToVillageStart) ? SS_LoadingToVillageEnd : SS_LoadingToObsidianEnd;
            }
        }

        setState_frameAnalyzeRequest();
        break;
    }
    case SS_LoadingToVillageEnd:
    case SS_LoadingToObsidianEnd:
    {
        if (state == S_CaptureReady)
        {
            // Detect entering village/obsidian fieldlands
            if (!checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                m_videoManager->clearAreas();
                if (m_substage == SS_LoadingToVillageEnd)
                {
                    // Detect map
                    m_substage = SS_DetectMap;
                    setState_runCommand("LDown,60");

                    m_elapsedTimer.restart();
                    m_videoManager->setAreas({A_Map});
                }
                else
                {
                    // Arrived Obsidian Fieldlands
                    // This is only for checking if we should sleep or not
                    if (m_searchSisterCount >= 4)
                    {
                        m_searchSisterCount = 0;
                    }

                    if (m_searchSisterCount == 0)
                    {
                        emit printLog("Sleeping until morning (program start/every 4 searches)");

                        m_substage = SS_WaitTillMorning;
                        setState_runCommand(C_WaitTillMorning);
                    }
                    else
                    {
                        // Not need to sleep till morning, save immediately
                        m_substage = SS_Save;
                        setState_runCommand(C_Save);
                    }
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_DetectMap:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 10000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect map for too long...");
            }
            else if (checkBrightnessMeanTarget(A_Map.m_rect, C_Color_Map, 240))
            {
                emit printLog("Map detected, returning to Obsidian Fieldlands...");

                m_substage = SS_LoadingToObsidianStart;
                setState_runCommand(QString(m_isFirstTimeVillageReturn ? "DRight,1," : "") + "A,16,DDown,1,ASpam,80");

                m_videoManager->clearCaptures();
                m_isFirstTimeVillageReturn = false;
            }
            else
            {
                setState_runCommand("A,1,Nothing,20");
            }
        }
        break;
    }
    case SS_Capture:
    {
        if (m_shinyDetected)
        {
            setState_runCommand("Capture,22,Minus,1,Nothing,20,Home,1");
            m_shinyDetected = false;
        }
        else if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartPLANuggetFarmer::soundDetected(int id)
{
    if (!m_isFindShiny || id != m_shinySoundID) return;

    if (m_substage == SS_FindSister || m_substage == SS_AfterBattle)
    {
        incrementStat(m_statShiny);
        m_shinyDetected = true;

        emit printLog("SHINY POKEMON FOUND!", LOG_SUCCESS);
        m_substage = SS_Capture;
        runNextStateContinue();
    }
}
