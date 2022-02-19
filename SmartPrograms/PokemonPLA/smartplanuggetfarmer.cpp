#include "smartplanuggetfarmer.h"

SmartPLANuggetFarmer::SmartPLANuggetFarmer(SmartProgramParameter parameter) : SmartProgramBase(parameter)
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
    setImageMatchFromResource("PLA_AConfirm", m_imageMatch_AConfirm);
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

                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas({A_Title});

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

                    m_parameters.vlcWrapper->clearCaptures();
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

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_BattleEnd});
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

                m_parameters.vlcWrapper->clearCaptures();
            }
            else if(checkAverageColorMatch(A_BattleEnd.m_rect, QColor(0,0,0)))
            {
                m_parameters.vlcWrapper->clearCaptures();
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

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Royal});
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

                m_parameters.vlcWrapper->clearCaptures();
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

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas({A_Dialog, A_Royal});
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

            m_parameters.vlcWrapper->clearCaptures();
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

            m_parameters.vlcWrapper->clearCaptures();
        }
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 150))
            {
                // This will interrupt the currently running command
                m_substage = SS_StartBattle;
                setState_runCommand("Nothing,20");
            }
            else if (checkImageMatchTarget(A_Royal.m_rect, C_Color_RoyalWhite, m_imageMatch_RoyalWyrdeer, 0.5))
            {
                // This will interrupt the currently running command
                m_substage = SS_AlphaKnockOff;
                setState_runCommand("Nothing,20");
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

            m_parameters.vlcWrapper->clearCaptures();
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
                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas({A_BattleEnd});
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

                m_parameters.vlcWrapper->clearCaptures();
            }
            else if (elapsed > 20000 && checkAverageColorMatch(A_BattleEnd.m_rect, QColor(0,0,0)))
            {
                // TODO: it is possible we have switched to Braviary just before Coin
                emit printLog("Battle complete!");

                // Battle arena may have around, try to fly to safety
                m_substage = SS_AfterBattle;
                setState_runCommand(m_isCoin ? C_AfterBattle : C_AfterBattleCharm);

                m_parameters.vlcWrapper->clearCaptures();
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
        }
        break;
    }
    case SS_TalkToLaventon:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            m_parameters.vlcWrapper->setAreas({A_AConfirmReturn, A_AConfirmReport, A_PokedexProgress});
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 60000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to finish talking to Laventon for too long, restarting game", LOG_ERROR);

                m_substage = SS_Restart;
                setState_runCommand(C_Restart);

                m_parameters.vlcWrapper->clearCaptures();
            }
            else if (checkBrightnessMeanTarget(A_PokedexProgress.m_rect, C_Color_Dialog, 200) || checkImageMatchTarget(A_AConfirmReport.m_rect, C_Color_AConfirmReturn, m_imageMatch_AConfirm, 0.5))
            {
                // Pokedex need to press A to exit
                setState_runCommand("A,20,Nothing,1");
            }
            else if (checkImageMatchTarget(A_AConfirmReturn.m_rect, C_Color_AConfirmReturn, m_imageMatch_AConfirm, 0.5))
            {
                // Head to village and return
                emit printLog("Heading back to village and return...");
                m_substage = SS_VillageReturn;
                setState_runCommand(m_isFirstTimeVillageReturn ? C_VillageReturn : C_VillageReturnNoMove);

                m_parameters.vlcWrapper->clearCaptures();
                m_isFirstTimeVillageReturn = false;
            }
            else
            {
                setState_runCommand("B,20,Nothing,1");
            }
        }
        break;
    }
    case SS_VillageReturn:
    {
        if (state == S_CommandFinished)
        {
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
        break;
    }
    }

    SmartProgramBase::runNextState();
}
