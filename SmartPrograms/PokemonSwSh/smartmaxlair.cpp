#include "smartmaxlair.h"

QVector<SmartMaxLair::IDRentalPair> SmartMaxLair::m_bossData;
QVector<SmartMaxLair::IDRentalPair> SmartMaxLair::m_rentalData;
PokemonDatabase::OCREntries SmartMaxLair::m_allBossEntries;
PokemonDatabase::OCREntries SmartMaxLair::m_allRentalEntries;
QMap<QString, QVector<double>> SmartMaxLair::m_matchupData;
QMap<int, SmartMaxLair::MoveData> SmartMaxLair::m_moveData;
QMap<QString, MoveType> SmartMaxLair::m_abilityImmunity;

SmartMaxLair::SmartMaxLair
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(setting)
{
    init();

    if (!populateMaxLairMatchupData() || m_matchupData.empty())
    {
        setState_error("Unable to load match up data");
        return;
    }

    if (!populateMaxLairRentalBossData() || m_bossData.empty() || m_rentalData.empty())
    {
        setState_error("Unable to load rental and boss data");
        return;
    }

    if (!populateMaxLairMoveData() || m_moveData.empty())
    {
        setState_error("Unable to load move data");
        return;
    }

    if (m_allBossEntries.empty())
    {
        m_allBossEntries = PokemonDatabase::getEntries_SwShMaxLairBoss(m_settings->getGameLanguage());
        if (m_allBossEntries.empty())
        {
            setState_error("Unable to boss ocr entries");
            return;
        }
    }

    if (m_allRentalEntries.empty())
    {
        m_allRentalEntries = PokemonDatabase::getEntries_SwShMaxLairRental(m_settings->getGameLanguage());
        if (m_allRentalEntries.empty())
        {
            setState_error("Unable to rental ocr entries");
            return;
        }
    }

    if (m_abilityImmunity.empty())
    {
        m_abilityImmunity["lightning-rod"] = MT_Electric;
        m_abilityImmunity["volt-absorb"] = MT_Electric;
        m_abilityImmunity["water-absorb"] = MT_Water;
        m_abilityImmunity["storm-drain"] = MT_Water;
        m_abilityImmunity["dry-skin"] = MT_Water;
        m_abilityImmunity["levitate"] = MT_Ground;
        m_abilityImmunity["flash-fire"] = MT_Fire;
        m_abilityImmunity["sap-sipper"] = MT_Grass;
    }
}

void SmartMaxLair::init()
{
    SmartProgramBase::init();
}

void SmartMaxLair::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    setImageMatchFromResource("SwSh_RStick", m_imageMatch_RStick);
    setImageMatchFromResource("SwSh_Dynamax", m_imageMatch_Dynamax);

    m_bufferDetection = 0;
    m_runCount = 0;
    resetBattleParams(true);
}

void SmartMaxLair::resetBattleParams(bool isBeginning)
{
    m_bossCurrent = RentalData();
    m_bossChecked = false;

    m_turnCount = 0;
    m_dynamaxCount = -1; // -1: can dynamax, >=0: dynamax turns remain
    m_cursorPos = 0; // first move
    m_moveScoreList.clear();
    m_moveUsable.resize(4);

    m_hungerSwitch = false;

    if (isBeginning)
    {
        m_battleCount = 0;
        m_rentalCurrent = RentalData();
        m_bossNames.clear();
    }
}

void SmartMaxLair::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statError, "Errors");
        initStat(m_statRuns, "Runs");
        initStat(m_statCaught, "Caught");
        initStat(m_statWins, "Wins");
        initStat(m_statShiny, "Shiny Found");
        m_timer.restart();

        bool testRentalSelect = false;
        if (testRentalSelect)
        {
            m_substage = SS_RentalSelect;
            setState_frameAnalyzeRequest();
            break;
        }

        bool testFindPath = false;
        if (testFindPath)
        {
            m_substage = SS_FindPath;
            setState_runCommand("Nothing,10");
            break;
        }

        bool testBattle = false;
        if (testBattle)
        {
            // for losing
            //m_bossChecked = true;
            //m_battleCount = 3;

            m_substage = SS_Battle;
            setState_runCommand("Nothing,10");
            break;
        }

        bool testCatch = false;
        if (testCatch)
        {
            m_substage = SS_Catch;
            setState_runCommand("ASpam,4,Nothing,30");
            break;
        }

        bool testResult = false;
        if (testResult)
        {
            m_battleCount = 4;
            m_substage = SS_Result;
            setState_runCommand("Nothing,10");
            break;
        }

        m_substage = SS_Start;
        setState_runCommand("Nothing,5");
        break;
    }
    case SS_Start:
    {
        // Press A until "Yes, Please!"
        if (state == S_CommandFinished)
        {
            if (m_timer.elapsed() > 60000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect DA dialog start");
            }
            else
            {
                setState_frameAnalyzeRequest();
                m_videoManager->setAreas({A_SelectionBase, A_Selection[1], A_Ore});
            }
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Ore.m_rect, C_Color_Ore, 190))
            {
                setState_ocrRequest(A_Ore.m_rect, C_Color_TextOre);
                break;
            }
            else if (checkAverageColorMatch(A_SelectionBase.m_rect, QColor(253,253,253)) && checkBrightnessMeanTarget(A_Selection[1].m_rect, C_Color_Black, 180))
            {
                resetBattleParams(true);
                m_substage = SS_BossSelect;
                m_videoManager->setAreas({A_Selection[0], A_Selection[1], A_Selection[2]});
            }

            setState_runCommand("A,42");
        }
        else if (state == S_OCRReady)
        {
            int oreCount = 0;
            if (getOCRNumber(oreCount))
            {
                if (oreCount < 20)
                {
                    emit printLog("Stopping program, less than 20 Dynite Ores", LOG_WARNING);
                    setState_completed();
                    break;
                }

                emit printLog("Dynite Ore left: " + QString::number(oreCount) + ", paying...");
            }
            else
            {
                emit printLog("Unable to detect ore count, attempting to pay anyway...", LOG_WARNING);
            }

            setState_runCommand("A,42,Nothing,1,Loop,2");
        }
        break;
    }
    case SS_BossSelect:
    {
        // Press A until boss selection
        if (state == S_CommandFinished)
        {
            if (m_timer.elapsed() > 60000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect boss selection");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Selection[0].m_rect, C_Color_Black, 180)
             || checkBrightnessMeanTarget(A_Selection[1].m_rect, C_Color_Black, 180)
             || checkBrightnessMeanTarget(A_Selection[2].m_rect, C_Color_Black, 180))
            {
                emit printLog("Selecting Boss...");
                m_substage = SS_StartDA;

                QString command = "ASpam,40";
                if (m_programSettings.m_legendDownPress > 0)
                {
                    command = "DDown,1,Nothing,1,Loop," + QString::number(m_programSettings.m_legendDownPress) + "," + command;
                }
                setState_runCommand(command);
                m_videoManager->setAreas({A_RaidStart[0], A_RaidStart[1], A_RaidStart[2]});
            }
            else
            {
                setState_runCommand("A,42");
            }
        }
        break;
    }
    case SS_StartDA:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 60000)
            {
                incrementStat(m_statError);
                setState_error("Unable Dynamax Adventure start screen");
            }
            else if (checkAverageColorMatch(A_RaidStart[0].m_rect, QColor(0,0,0))
                  && checkAverageColorMatch(A_RaidStart[1].m_rect, QColor(253,253,253))
                  && checkAverageColorMatch(A_RaidStart[2].m_rect, QColor(253,253,253)))
            {
                incrementStat(m_statRuns);
                emit printLog("Starting Run No." + QString::number(m_statRuns.first), LOG_IMPORTANT);

                m_substage = SS_RentalSelect;
                setState_runCommand("DDown,1,A,1");

                m_timer.restart();
                m_videoManager->setAreas({A_RentalSelect[0], A_RentalSelect[1], A_RentalSelect[2]});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_RentalSelect:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect Rental Pokemon screen");
            }
            else if (checkAverageColorMatch(A_RentalSelect[0].m_rect, QColor(0,0,0))
                  && checkAverageColorMatch(A_RentalSelect[1].m_rect, QColor(253,253,253))
                  && checkAverageColorMatch(A_RentalSelect[2].m_rect, QColor(253,253,253)))
            {
                setState_ocrRequest(A_RentalName[0].m_rect, C_Color_TextW);

                m_ocrIndex = 0;
                m_rentalSearch.clear();
                m_rentalSearch.resize(3);
                m_videoManager->setAreas({A_RentalName[0], A_RentalName[1], A_RentalName[2],
                                          A_RentalAbility[0], A_RentalAbility[1], A_RentalAbility[2],
                                          A_RentalMove[0], A_RentalMove[1], A_RentalMove[2]});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_OCRReady)
        {
            if (m_ocrIndex < 3)
            {
                // Name OCR
                QString const result = matchStringDatabase(m_allRentalEntries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Rental Pokemon entry not found", LOG_ERROR);
                }

                m_rentalSearch[m_ocrIndex].m_name = result;
                m_ocrIndex++;
                if (m_ocrIndex == 3)
                {
                    setState_ocrRequest(A_RentalAbility[0].m_rect, C_Color_TextW);
                }
                else
                {
                    setState_ocrRequest(A_RentalName[m_ocrIndex].m_rect, C_Color_TextB);
                }
            }
            else if (m_ocrIndex < 6)
            {
                // Ability OCR
                PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_SwShMaxLairAbilities(m_settings->getGameLanguage());
                QString const result = matchStringDatabase(entries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Ability entry not found", LOG_ERROR);
                }

                m_rentalSearch[m_ocrIndex - 3].m_ability = result;
                m_ocrIndex++;
                if (m_ocrIndex == 6)
                {
                    setState_ocrRequest(A_RentalMove[0].m_rect, C_Color_TextB);
                }
                else
                {
                    setState_ocrRequest(A_RentalAbility[m_ocrIndex - 3].m_rect, C_Color_TextB);
                }
            }
            else
            {
                // First Move OCR
                PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_SwShMaxLairMoves(m_settings->getGameLanguage());
                QString const result = matchStringDatabase(entries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Move entry not found", LOG_ERROR);

                    m_rentalSearch[m_ocrIndex - 6].m_firstMove = -1;
                }
                else
                {
                    m_rentalSearch[m_ocrIndex - 6].m_firstMove = result.toInt();
                }

                m_ocrIndex++;
                if (m_ocrIndex == 9)
                {
                    m_rentalCurrent = RentalData();
                    QVector<int> rentalIndices(3, -1);

                    QString id;
                    int selectIndex = -1;
                    m_rentalScore = 0;

                    for (int i = 0; i < 3; i++)
                    {
                        RentalSearch const& search = m_rentalSearch[i];
                        if (search.m_name.isEmpty() || search.m_ability.isEmpty() || search.m_firstMove < 0)
                        {
                            continue;
                        }

                        bool found = false;
                        for (int j = 0; j < m_rentalData.size(); j++)
                        {
                            IDRentalPair const& idPair = m_rentalData[j];
                            RentalData const& data = idPair.second;
                            if (data.m_name == search.m_name && data.m_moves.at(0) == search.m_firstMove && data.m_ability == search.m_ability)
                            {
                                rentalIndices[i] = j;
                                id = idPair.first;
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            incrementStat(m_statError);
                            emit printLog("Unable to find Rental Pokemon {" + search.m_name + ", " + search.m_ability + ", " + QString::number(search.m_firstMove) + "}", LOG_ERROR);
                            continue;
                        }

                        if (!m_matchupData.contains(id))
                        {
                            incrementStat(m_statError);
                            emit printLog("Unable to find matchup data for '" + id + "'", LOG_ERROR);
                            continue;
                        }

                        double score = m_matchupData[id].at(m_programSettings.m_legendIndex);
                        emit printLog("Rental Pokemon '" + id + "' VS '" + m_bossData[m_programSettings.m_legendIndex].first + "' score: " + QString::number(score));
                        if (score > m_rentalScore)
                        {
                            m_rentalScore = score;
                            selectIndex = i;
                        }
                    }

                    if (selectIndex >= 0)
                    {
                        emit printLog("Selecting Slot " + QString::number(selectIndex + 1) + ": " + m_rentalSearch[selectIndex].m_name, LOG_IMPORTANT);
                        QString command = "ASpam,10,Nothing,300";
                        if (selectIndex > 0)
                        {
                            command = "DDown,1,Nothing,1,Loop," + QString::number(selectIndex) + "," + command;
                        }

                        m_timer.restart();
                        m_substage = SS_FindPath;
                        setState_runCommand(command);

                        m_rentalCurrent = m_rentalData[ rentalIndices[selectIndex] ].second;
                        m_videoManager->clearCaptures();
                    }
                    else
                    {
                        incrementStat(m_statError);
                        setState_error("Unable to pick any Rental Pokemon");
                    }
                }
                else
                {
                    setState_ocrRequest(A_RentalMove[m_ocrIndex - 6].m_rect, C_Color_TextB);
                }
            }
        }
        break;
    }
    case SS_FindPath:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            m_videoManager->setPoints({P_Pokemon,P_Run});
            m_videoManager->setAreas({A_RStick, A_Fight, A_Person});
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 90000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect pick path sequence");
            }
            else if (checkPixelColorMatch(P_Pokemon.m_point, QColor(253,253,253)) && checkPixelColorMatch(P_Run.m_point, QColor(253,253,253)) && checkBrightnessMeanTarget(A_Fight.m_rect, C_Color_Fight, 80))
            {
                // Battle started
                m_substage = SS_Battle;
                setState_frameAnalyzeRequest();
            }
            else if (checkAverageColorMatch(A_Person.m_rect, QColor(253,253,253)))
            {
                // detect scientist or backpacker
                m_substage = SS_Person;
                setState_frameAnalyzeRequest();

                m_timer.restart();
                m_videoManager->clearPoints();
                m_videoManager->setAreas({A_Backpacker, A_SelectionBase});
            }
            else if (checkImageMatchTarget(A_RStick.m_rect, C_Color_RStick, m_imageMatch_RStick, 0.5))
            {
                emit printLog("Picking path...");
                setState_runCommand("ASpam,10,Nothing,40");

                m_bossChecked = false;
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand("BSpam,20");
            }
        }
        break;
    }
    case SS_Person:
    {
        if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                emit printLog("Unable to detect Scientist or Backpacker...", LOG_WARNING);
                m_substage = SS_FindPath;
                setState_runCommand("BSpam,20");
                m_videoManager->clearCaptures();
            }
            else if (checkAverageColorMatch(A_Backpacker.m_rect, QColor(12,178,247)))
            {
                emit printLog("Encountered a Backpacker!");
                m_substage = SS_FindPath;
                setState_runCommand("BSpam,100");
                m_videoManager->clearCaptures();
            }
            else if (checkAverageColorMatch(A_SelectionBase.m_rect, QColor(253,253,253)))
            {
                emit printLog("Encountered a Scientist!");
                m_substage = SS_FindPath;
                setState_runCommand("BSpam,120");
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
    {
        if (state == S_CommandFinished)
        {
            if (m_bufferDetection > 0)
            {
                // double detection
                setState_frameAnalyzeRequest();
                break;
            }

            setState_runCommand("BSpam,2,Loop,0", true);

            m_timer.restart();
            if (m_bossChecked)
            {
                m_videoManager->setPoints({P_Pokemon,P_Run,P_Catch[0],P_Catch[1]});
                m_videoManager->setAreas({A_Fight,A_Caught[0],A_Caught[1]});
            }
            else
            {
                m_videoManager->setPoints({P_Pokemon,P_Run});
                m_videoManager->setAreas({A_Fight});
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 120000)
            {
                incrementStat(m_statError);
                setState_error("No detection for too long");
                break;
            }

            // Fight or Cheer
            if (checkPixelColorMatch(P_Pokemon.m_point, QColor(253,253,253)) && checkPixelColorMatch(P_Run.m_point, QColor(253,253,253)))
            {
                // Fight
                if (checkBrightnessMeanTarget(A_Fight.m_rect, C_Color_Fight, 80))
                {
                    if (m_bufferDetection != 1)
                    {
                        // double detection
                        m_bufferDetection = 1;
                        setState_runCommand("Nothing,10");

                        m_videoManager->setPoints({P_Pokemon,P_Run});
                        m_videoManager->setAreas({A_Fight});
                        break;
                    }
                    else
                    {
                        m_bufferDetection = 0;
                    }

                    m_videoManager->clearCaptures();
                    if (!m_bossChecked)
                    {
                        m_battleCount++;
                        emit printLog("Battle " + QString::number(m_battleCount) + " started!", LOG_IMPORTANT);

                        if (m_battleCount < 4)
                        {
                            emit printLog("Checking Boss...");
                            m_substage = SS_Target;
                            setState_runCommand("Y,1,Nothing,20");
                            m_videoManager->setPoints({A_Trainers[0], A_Trainers[1], A_Trainers[2], A_Trainers[3]});
                            break;
                        }
                        else
                        {
                            m_bossCurrent = m_bossData[m_programSettings.m_legendIndex].second;
                            m_bossChecked = true;

                            // Our Ditto transformed into boss
                            if (m_rentalCurrent.m_name == "Ditto")
                            {
                                m_rentalCurrent = m_bossCurrent;
                                m_rentalCurrent.m_name = "Ditto";
                            }
                        }
                    }

                    m_substage = SS_Fight;
                    setState_runCommand("ASpam,4,Nothing,30");
                    m_videoManager->clearPoints();
                    m_videoManager->setAreas({A_MoveUsable[0],A_MoveUsable[1],A_MoveUsable[2],A_MoveUsable[3]});

                    if (m_dynamaxCount > 0)
                    {
                        m_dynamaxCount--;
                        if (m_dynamaxCount == 0)
                        {
                            // cursor resets when dynamax completes
                            m_cursorPos = 0;
                        }
                    }
                    else if (m_dynamaxCount == -1 && m_moveScoreList.empty())
                    {
                        m_videoManager->setAreas({A_MoveUsable[0],A_MoveUsable[1],A_MoveUsable[2],A_MoveUsable[3],A_Dynamax});
                    }

                    // Hunger Switch changes Aura Wheel typing
                    if (m_rentalCurrent.m_name == "Morpeko" && m_hungerSwitch)
                    {
                        m_moveData[783].m_type = MT_Dark;
                        m_moveData[759140].m_type = MT_Dark;
                        m_moveData[759140].m_name = "Max Darkness";
                    }
                    else
                    {
                        m_moveData[783].m_type = MT_Electric;
                        m_moveData[759140].m_type = MT_Electric;
                        m_moveData[759140].m_name = "Max Lightning";
                    }

                    break;
                }

                // Cheer
                if (checkBrightnessMeanTarget(A_Fight.m_rect, C_Color_Cheer, 60))
                {
                    if (m_bufferDetection != 2)
                    {
                        // double detection
                        m_bufferDetection = 2;
                        setState_runCommand("Nothing,10");

                        m_videoManager->setPoints({P_Pokemon,P_Run});
                        m_videoManager->setAreas({A_Fight});
                        break;
                    }
                    else
                    {
                        m_bufferDetection = 0;
                    }

                    m_turnCount++;
                    emit printLog("Turn " + QString::number(m_turnCount) + ": Cheering...");
                    setState_runCommand("ASpam,10,BSpam,100");

                    if (m_dynamaxCount > 0)
                    {
                        // died during dynamax
                        m_dynamaxCount = 0;
                        m_cursorPos = 0;
                    }

                    // Back to normal Morpeko form
                    m_hungerSwitch = false;

                    m_videoManager->clearCaptures();
                    break;
                }
            }

            if (m_bossChecked)
            {
                // Battle complete
                if (checkPixelColorMatch(P_Catch[0].m_point, QColor(0,0,0)) && checkPixelColorMatch(P_Catch[1].m_point, QColor(253,253,253)))
                {
                    if (m_bufferDetection != 3)
                    {
                        // double detection
                        m_bufferDetection = 3;
                        setState_runCommand("Nothing,10");

                        m_videoManager->setPoints({P_Catch[0],P_Catch[1]});
                        m_videoManager->clearAreas();
                        break;
                    }
                    else
                    {
                        m_bufferDetection = 0;
                    }

                    if (m_battleCount == 4)
                    {
                        incrementStat(m_statWins);
                    }
                    emit printLog(m_bossCurrent.m_name + " defeated!", LOG_SUCCESS);

                    m_substage = SS_Catch;
                    setState_runCommand("ASpam,4,Nothing,30");

                    m_videoManager->clearCaptures();
                    m_ocrIndex = 0;
                    for (int i = 0; i < BT_COUNT; i++)
                    {
                        m_ballFound[i] = false;
                    }
                    break;
                }

                // Pokemon Caught (Lost)
                if (checkAverageColorMatch(A_Caught[0].m_rect, QColor(0,0,0)) && checkBrightnessMeanTarget(A_Caught[1].m_rect, C_Color_Caught, 230))
                {
                    if (m_bufferDetection != 4)
                    {
                        // double detection
                        m_bufferDetection = 4;
                        setState_runCommand("Nothing,10");

                        m_videoManager->clearPoints();
                        m_videoManager->setAreas({A_Caught[0],A_Caught[1]});
                        break;
                    }
                    else
                    {
                        m_bufferDetection = 0;
                    }

                    emit printLog("You've lost...");

                    m_substage = SS_Result;
                    setState_runCommand("Nothing,20");

                    m_videoManager->setAreas({A_Caught[0],A_Caught[1],A_SelectionBase});
                    break;
                }
            }

            if (m_bufferDetection > 0)
            {
                // if we are here, it had false detection
                m_bufferDetection = 0;
                setState_runCommand("Nothing,3");
                break;
            }

            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_Fight:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            QString command;

            // Check move usable
            for (int i = 0; i < 4; i++)
            {
                m_moveUsable[i] = checkAverageColorMatch(A_MoveUsable[i].m_rect, QColor(0,0,0));
            }

            // Calculate best move to use
            if (m_moveScoreList.isEmpty())
            {
                // Check if we can dynamax
                if (m_dynamaxCount == -1 && checkImageMatchTarget(A_Dynamax.m_rect, C_Color_Dynamax, m_imageMatch_Dynamax, 0.5))
                {
                    m_dynamaxCount = 3;
                }

                calculateBestMove();
                if (m_moveScoreList.isEmpty())
                {
                    m_turnCount++;
                    emit printLog("Turn " + QString::number(m_turnCount) + ": Using Struggle");

                    m_substage = SS_Battle;
                    setState_runCommand("BSpam,2");
                    break;
                }
            }
            else if (m_dynamaxCount == 3)
            {
                // cursor was on dynamax button, go back to moves
                command = "DRight,1,Nothing,1,Loop,1,";
            }

            // Choose move
            MoveScore const& moveScore = m_moveScoreList.front();
            if (moveScore.m_isMaxMove)
            {
                if (m_dynamaxCount == 3 || m_dynamaxCount == -1)
                {
                    // decided to dynamax
                    m_dynamaxCount = 3;
                    command += "DLeft,1,ASpam,4,Nothing,10,Loop,1,";
                }
            }
            else if (m_dynamaxCount == 3)
            {
                // decided to not dynamax
                m_dynamaxCount = -1;
            }

            // move cursor up/down
            if (moveScore.m_moveIndex > m_cursorPos)
            {
                command += "DDown,1,Nothing,1,Loop," + QString::number(moveScore.m_moveIndex - m_cursorPos) + ",";
            }
            else if (moveScore.m_moveIndex < m_cursorPos)
            {
                command += "DUp,1,Nothing,1,Loop," + QString::number(m_cursorPos - moveScore.m_moveIndex) + ",";
            }

            command += "ASpam,4,Nothing,20";
            m_cursorPos = moveScore.m_moveIndex;

            m_substage = SS_Target;
            setState_runCommand(command);

            m_videoManager->clearCaptures();
            m_videoManager->setPoints({A_Trainers[0], A_Trainers[1], A_Trainers[2], A_Trainers[3]});
        }
        break;
    }
    case SS_Target:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                if (m_bossChecked)
                {
                    if (m_moveScoreList.size() == 1)
                    {
                        incrementStat(m_statError);
                        setState_error("Unable to use any moves");
                        break;
                    }

                    // use next best move, can be disabled/tormented
                    int const moveID = m_moveScoreList.front().m_isMaxMove ? m_rentalCurrent.m_maxMoves[m_cursorPos] : m_rentalCurrent.m_moves[m_cursorPos];
                    MoveData const& moveData = m_moveData[moveID];
                    emit printLog("Unable to use " + moveData.m_name + " (due to Torment/Disable etc.), trying next best move", LOG_WARNING);
                    m_moveScoreList.pop_front();

                    // need to fake increase dynamax turns as SS_Battle decrements it
                    if (m_dynamaxCount > 0)
                    {
                        m_dynamaxCount++;
                    }

                    m_substage = SS_Battle;
                    setState_runCommand("BSpam,2");
                }
                else
                {
                    incrementStat(m_statError);
                    setState_error("Unable to detect target select menu");
                }
            }
            else if (checkPixelColorMatch(A_Trainers[0].m_point, QColor(0,0,0))
                  && checkPixelColorMatch(A_Trainers[1].m_point, QColor(0,0,0))
                  && checkPixelColorMatch(A_Trainers[2].m_point, QColor(0,0,0))
                  && checkPixelColorMatch(A_Trainers[3].m_point, QColor(0,0,0)))
            {
                if (m_bossChecked)
                {
                    // TODO: healing move default is enemy lol
                    // TODO: self target move, Outrage etc.
                    // use move on default target
                    m_substage = SS_Battle;
                    setState_runCommand("ASpam,10,BSpam,100");

                    // grab data to print...
                    m_turnCount++;
                    int const moveID = m_moveScoreList.front().m_isMaxMove ? m_rentalCurrent.m_maxMoves[m_cursorPos] : m_rentalCurrent.m_moves[m_cursorPos];
                    MoveData const& moveData = m_moveData[moveID];
                    emit printLog("Turn " + QString::number(m_turnCount) + ": Using " + moveData.m_name + " (Score = " + QString::number(m_moveScoreList.front().m_score) + ")");

                    // used move successfully
                    m_moveScoreList.clear();
                    m_hungerSwitch = !m_hungerSwitch;
                }
                else
                {
                    m_substage = SS_CheckBoss;
                    setState_runCommand("DUp,1,ASpam,4,Nothing,30");
                }
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CheckBoss:
    {
        if (state == S_CommandFinished)
        {
            m_ocrIndex = 0;
            setState_ocrRequest(A_Boss.m_rect, C_Color_TextB);

            m_bossSearch.m_types[0] = MT_COUNT;
            m_bossSearch.m_types[1] = MT_COUNT;
            m_videoManager->setAreas({A_Boss, A_BossTypes[0], A_BossTypes[1]});
        }
        else if (state == S_OCRReady)
        {
            if (m_ocrIndex == 0)
            {
                // Name OCR
                QString const result = matchStringDatabase(m_allRentalEntries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    setState_error("Rental Pokemon entry not found");
                    break;
                }

                m_bossSearch.m_name = result;
                setState_ocrRequest(A_BossTypes[0].m_rect, C_Color_TextW);
            }
            else
            {
                // Type OCR
                PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_PokemonTypes(m_settings->getGameLanguage());
                QString const result = matchStringDatabase(entries);
                if (result.isEmpty() && m_ocrIndex == 1)
                {
                    incrementStat(m_statError);
                    emit printLog("Type entry not found", LOG_ERROR);
                }

                m_bossSearch.m_types[m_ocrIndex - 1] = PokemonDatabase::getMoveTypeFromString(result);
                if (m_ocrIndex == 2)
                {
                    m_bossCurrent = RentalData();
                    for (int i = 0; i < m_rentalData.size(); i++)
                    {
                        IDRentalPair const& idPair = m_rentalData[i];
                        RentalData const& data = idPair.second;
                        if (data.m_name == m_bossSearch.m_name)
                        {
                            m_bossCurrent = data;
                            if (m_bossSearch.m_types[0] == MT_COUNT)
                            {
                                // language does not support type search
                                break;
                            }
                            else if (data.m_types[0] == m_bossSearch.m_types[0] && data.m_types[1] == m_bossSearch.m_types[1])
                            {
                                break;
                            }
                        }
                    }

                    if (!m_bossCurrent.m_name.isEmpty())
                    {
                        // Boss Ditto transformed into our Pokemon
                        if (m_bossCurrent.m_name == "Ditto")
                        {
                            emit printLog("Boss: " + m_bossCurrent.m_name + " (Transformed to " + m_rentalCurrent.m_name + ")", LOG_IMPORTANT);
                            m_bossCurrent = m_rentalCurrent;
                            m_bossCurrent.m_name = "Ditto";
                        }
                        else
                        {
                            emit printLog("Boss: " + m_bossCurrent.m_name, LOG_IMPORTANT);
                        }

                        // Our Ditto transformed into boss
                        if (m_rentalCurrent.m_name == "Ditto")
                        {
                            m_rentalCurrent = m_bossCurrent;
                            m_rentalCurrent.m_name = "Ditto";
                        }

                        m_substage = SS_Battle;
                        setState_runCommand("BSpam,2");
                        m_bossChecked = true;
                    }
                    else
                    {
                        incrementStat(m_statError);
                        setState_error("Unable to detect Boss Pokemon");
                    }
                }
                else
                {
                    setState_ocrRequest(A_BossTypes[1].m_rect, C_Color_TextW);
                }
            }

            m_ocrIndex++;
        }
        break;
    }
    case SS_Catch:
    {
        if (state == S_CommandFinished)
        {
            m_ocrIndex++;
            if (m_ocrIndex >= BT_COUNT)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect any Pokeball names");
                break;
            }

            setState_ocrRequest(A_Ball.m_rect, C_Color_TextW);
            m_videoManager->setAreas({A_Ball});
        }
        else if (state == S_OCRReady)
        {
            PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_Pokeballs(m_settings->getGameLanguage());
            QString const result = matchStringDatabase(entries);
            if (result.isEmpty())
            {
                incrementStat(m_statError);
                emit printLog("Pokeball entry not found", LOG_ERROR);
            }
            else
            {
                BallType type = PokemonDatabase::getBallTypeFromString(result);
                if (type < BT_COUNT)
                {
                    BallType targetType = (m_battleCount == 4 ? m_programSettings.m_legendBall : m_programSettings.m_bossBall);
                    if (m_ballFound[type])
                    {
                        incrementStat(m_statError);
                        setState_error("Unable to find " + PokemonDatabase::getList_Pokeballs().at(targetType) + " Ball");
                        break;
                    }

                    m_ballFound[type] = true;
                    if (type == targetType)
                    {
                        incrementStat(m_statCaught);
                        emit printLog("Catching Boss with " + PokemonDatabase::getList_Pokeballs().at(type) + " Ball");

                        m_substage = (m_battleCount == 4) ? SS_Result : SS_RentalSwap;
                        setState_runCommand("ASpam,10,Nothing,300");

                        m_bossNames.push_back(m_bossCurrent.m_name);
                        m_videoManager->clearCaptures();
                        break;
                    }
                }
            }

            setState_runCommand("DLeft,1,Nothing,20");
        }
        break;
    }
    case SS_RentalSwap:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            m_timer.restart();
            m_videoManager->setAreas({A_SwapButtons[0], A_SwapButtons[1]});
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect rental swap screen for too long");
            }
            else if (checkAverageColorMatch(A_SwapButtons[0].m_rect, QColor(0,0,0)) && checkAverageColorMatch(A_SwapButtons[1].m_rect, QColor(253,253,253)))
            {
                m_ocrIndex = 0;
                setState_ocrRequest(A_RentalName[3].m_rect, C_Color_TextB);
                m_videoManager->setAreas({A_RentalName[3], A_RentalAbility[3], A_RentalMove[3]});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_OCRReady)
        {
            if (m_ocrIndex == 0)
            {
                // Name OCR
                QString const result = matchStringDatabase(m_allRentalEntries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Rental Pokemon entry not found", LOG_ERROR);
                }

                m_bossSearch.m_name = result;
                m_ocrIndex++;
                setState_ocrRequest(A_RentalAbility[3].m_rect, C_Color_TextB);
            }
            else if (m_ocrIndex == 1)
            {
                // Ability OCR
                PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_SwShMaxLairAbilities(m_settings->getGameLanguage());
                QString const result = matchStringDatabase(entries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Ability entry not found", LOG_ERROR);
                }

                m_bossSearch.m_ability = result;
                m_ocrIndex++;
                setState_ocrRequest(A_RentalMove[3].m_rect, C_Color_TextB);
            }
            else
            {
                // First Move OCR
                PokemonDatabase::OCREntries const& entries = PokemonDatabase::getEntries_SwShMaxLairMoves(m_settings->getGameLanguage());
                QString const result = matchStringDatabase(entries);
                if (result.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Move entry not found", LOG_ERROR);
                }
                else
                {
                    m_bossSearch.m_firstMove = result.toInt();
                }

                QString id;
                m_bossCurrent = RentalData();

                for (int i = 0; i < m_rentalData.size(); i++)
                {
                    IDRentalPair const& idPair = m_rentalData[i];
                    RentalData const& data = idPair.second;
                    if (data.m_name == m_bossSearch.m_name && data.m_moves.at(0) == m_bossSearch.m_firstMove && data.m_ability == m_bossSearch.m_ability)
                    {
                        m_bossCurrent = data;
                        id = idPair.first;
                        break;
                    }
                }

                m_timer.restart();
                m_substage = SS_FindPath;
                m_videoManager->clearCaptures();

                if (m_bossCurrent.m_name.isEmpty())
                {
                    incrementStat(m_statError);
                    emit printLog("Unable to find Rental Pokemon {" + m_bossSearch.m_name + ", " + m_bossSearch.m_ability + ", " + QString::number(m_bossSearch.m_firstMove) + "}", LOG_ERROR);
                }
                else if (!m_matchupData.contains(id))
                {
                    incrementStat(m_statError);
                    emit printLog("Unable to find matchup data for '" + id + "'", LOG_ERROR);
                }
                else
                {
                    double score = m_matchupData[id].at(m_programSettings.m_legendIndex);
                    emit printLog("Rental Pokemon '" + id + "' VS '" + m_bossData[m_programSettings.m_legendIndex].first + "' score: " + QString::number(score));
                    if (score > m_rentalScore)
                    {
                        m_rentalScore = score;
                        emit printLog("Swapping Rental Pokemon with " + m_bossCurrent.m_name, LOG_IMPORTANT);
                        setState_runCommand("ASpam,10,Nothing,30");

                        m_rentalCurrent = m_bossCurrent;
                        resetBattleParams(false);
                        break;
                    }
                }

                emit printLog("Not swapping Rental Pokemon", LOG_IMPORTANT);
                setState_runCommand("BSpam,10,Nothing,30");
                resetBattleParams(false);
            }
        }
        break;
    }
    case SS_Result:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();

            m_timer.restart();
            if (m_bossNames.size() < 4)
            {
                m_videoManager->setAreas({A_Caught[0], A_Caught[1], A_SelectionBase});
            }
            else
            {
                m_videoManager->setAreas({A_Caught[0], A_Caught[1]});
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect Pokemon Caught screen for too long");
            }
            else if (m_bossNames.size() < 4 && checkAverageColorMatch(A_SelectionBase.m_rect, QColor(253,253,253)))
            {
                // cancel Yes/No dialog (from BSpam after losing)
                setState_runCommand("B,1,Nothing,30");
            }
            else if (checkAverageColorMatch(A_Caught[0].m_rect, QColor(0,0,0)) && checkBrightnessMeanTarget(A_Caught[1].m_rect, C_Color_Caught, 230))
            {
                // Display average time
                m_runCount++;
                qint64 secs = m_programStartDateTime.secsTo(QDateTime::currentDateTime()) / m_runCount;
                qint64 mins = secs / 60;
                mins %= 60;
                secs %= 60;
                emit printLog("Average Time per Run: " + QString::number(mins) + "m" + QString::number(secs) + "s");

                m_videoManager->clearCaptures();
                if (m_bossNames.empty())
                {
                    emit printLog("No caught Pokemon to check...");
                    m_substage = SS_Start;
                    setState_runCommand("ASpam,100");

                    m_timer.restart();
                    break;
                }

                emit printLog("Checking shiny for caught Pokemon");
                m_substage = SS_CheckShiny;
                setState_runCommand("Nothing,10,DUp,1,A,10,DDown,1,A,1,Nothing,70");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CheckShiny:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Shiny});
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 30))
            {
                incrementStat(m_statShiny);
                emit printLog(m_bossNames.back() + " is SHINY, taking screenshot!", LOG_SUCCESS);

                m_substage = SS_TakeReward;
                setState_runCommand("BSpam,4,Nothing,60,Capture,1");

                m_videoManager->clearCaptures();
            }
            else
            {
                emit printLog(m_bossNames.back() + " is not shiny...");
                m_bossNames.pop_back();
                if (m_bossNames.empty())
                {
                    emit printLog("Not taking any Pokemon");

                    m_substage = SS_Start;
                    setState_runCommand("BSpam,4,Nothing,60,BSpam,4,ASpam,140");

                    m_timer.restart();
                    m_videoManager->clearCaptures();
                }
                else
                {
                    // check next Pokemon above
                    setState_runCommand("DUp,1,Nothing,30");
                }
            }
        }
        break;
    }
    case SS_TakeReward:
    {
        if (state == S_CommandFinished)
        {
            QImage frame;
            m_videoManager->getFrame(frame);
            sendDiscordMessage("Shiny Found!", true, QColor(255,255,0), &frame);

            if (m_bossNames.size() == 4)
            {
                // let player take legendary
                setState_completed();
            }
            else
            {
                // take normal shiny
                m_substage = SS_Start;
                setState_runCommand("ASpam,140");

                m_timer.restart();
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartMaxLair::calculateBestMove()
{
    m_moveScoreList.clear();
    auto calculateBestMoveFromList = [&](QVector<int> const& moves, bool isMaxMoves)
    {
        MoveScore moveScore;
        moveScore.m_isMaxMove = isMaxMoves;

        for (int i = 0; i < 4; i++)
        {
            MoveData const& moveData = m_moveData[moves.at(i)];
            if (!m_moveUsable.at(i))
            {
                // ran out of PP
                continue;
            }

            double power = moveData.m_power;
            double accuracy = moveData.m_accuracy;
            double factor = moveData.m_factor;
            bool useTypeMatchup = true;

            // Immunity
            if (m_abilityImmunity.count(m_bossCurrent.m_ability) && moveData.m_type == m_abilityImmunity[m_bossCurrent.m_ability])
            {
                power = 0;
            }

            // STAB bonus
            if (moveData.m_type == m_rentalCurrent.m_types[0] || moveData.m_type == m_rentalCurrent.m_types[1])
            {
                if (m_rentalCurrent.m_ability == "adaptability")
                {
                    power *= 2.0;
                }
                else
                {
                    power *= 1.5;
                }
            }

            // No Guard
            if (m_bossCurrent.m_ability == "no-guard" || m_rentalCurrent.m_ability == "no-guard")
            {
                // No Guard ignores accuracy
                accuracy = 1.0;
            }

            // Boss Abilities
            if (m_bossCurrent.m_ability == "fur-coat")
            {
                // Fur Coat halves physical damage
                if (!moveData.m_isSpecial)
                {
                    power *= 0.5;
                }
            }
            else if (m_bossCurrent.m_ability == "thick-fat")
            {
                // Thick Fat halves Fire/Ice damage
                if (moveData.m_type == MT_Fire || moveData.m_type == MT_Ice)
                {
                    power *= 0.5;
                }
            }
            else if (m_bossCurrent.m_ability == "dry-skin")
            {
                // Fire-type attacks do 25% more damage to a Pokémon with Dry Skin
                if (moveData.m_type == MT_Fire)
                {
                    power *= 1.25;
                }
            }
            else if (m_bossCurrent.m_ability == "ice-scales")
            {
                // Ice Scales halves special damage
                if (moveData.m_isSpecial)
                {
                    power *= 0.5;
                }
            }

            // Rental Abilities
            if (m_rentalCurrent.m_ability == "iron-fist")
            {
                // Increase punching move by 20%
                // All moves from Hiitmonchan, Focus Punch, Shadow Punch, Bullet Punch, Hammer Arm
                if ((m_rentalCurrent.m_name == "Hitmonchan" && !isMaxMoves) || moves.at(i) == 264 || moves.at(i) == 325 || moves.at(i) == 418 || moves.at(i) == 359)
                {
                    power *= 1.2;
                }
            }
            else if (m_rentalCurrent.m_ability == "scrappy")
            {
                // Scrappy can hit Ghost type with Normal/Fighting type moves
                if ((moveData.m_type == MT_Normal || moveData.m_type == MT_Fighting) && (m_bossCurrent.m_types[0] == MT_Ghost || m_bossCurrent.m_types[1] == MT_Ghost))
                {
                    useTypeMatchup = false;
                    if (m_bossCurrent.m_types[0] == MT_Ghost)
                    {
                        factor *= PokemonDatabase::typeMatchupMultiplier(moveData.m_type, m_bossCurrent.m_types[1]);
                    }
                    else
                    {
                        factor *= PokemonDatabase::typeMatchupMultiplier(moveData.m_type, m_bossCurrent.m_types[0]);
                    }
                }
            }
            else if (m_rentalCurrent.m_ability == "technician")
            {
                // Technician boost move power 60 or less by 50%
                if (moveData.m_power <= 60)
                {
                    power *= 1.5;
                }
            }
            else if (m_rentalCurrent.m_ability == "huge-power")
            {
                // Huge Power doubles physical moves power
                if (!moveData.m_isSpecial)
                {
                    power *= 2.0;
                }
            }
            else if (m_rentalCurrent.m_ability == "refrigerate")
            {
                // Increases the power of Normal-type moves by 20%. It then changes those moves to Ice-type.
                if (moveData.m_type == MT_Normal)
                {
                    power *= 1.2;

                    useTypeMatchup = false;
                    factor *= PokemonDatabase::typeMatchupMultiplier(MT_Ice, m_bossCurrent.m_types[0], m_bossCurrent.m_types[1]);
                }
            }
            else if (m_rentalCurrent.m_ability == "tough-claws")
            {
                // Barbaracle increases damage for all moves except for Blizzard
                if (moves.at(i) != 59 && !isMaxMoves)
                {
                    power *= 1.3;
                }
            }
            else if (m_rentalCurrent.m_ability == "strong-jaw")
            {
                // Increase biting move power by 50% (only Crunch)
                if (moves.at(i) == 242)
                {
                    power *= 1.5;
                }
            }
            else if (m_rentalCurrent.m_ability == "sheer-force")
            {
                // Moves with a secondary effect are increased in power by 30% but lose their secondary effect
                if (moves.at(i) == 442 || moves.at(i) == 430)
                {
                    power *= 1.3;
                }
            }

            // Calculate final score
            double score = factor * power * accuracy;
            if (useTypeMatchup)
            {
                score *= PokemonDatabase::typeMatchupMultiplier(moveData.m_type, m_bossCurrent.m_types[0], m_bossCurrent.m_types[1]);
            }

            // Zygarde & Wide Guard
            if (moves.at(i) == 469 && m_bossCurrent.m_name == "Zygarde")
            {
                score = 99999;
            }

            moveScore.m_moveIndex = i;
            moveScore.m_score = score;
            m_moveScoreList.push_back(moveScore);
        }
    };

    if (m_dynamaxCount == 3)
    {
        // can dynamax, calculate all moves
        calculateBestMoveFromList(m_rentalCurrent.m_maxMoves, true);
        calculateBestMoveFromList(m_rentalCurrent.m_moves, false);
    }
    else if (m_dynamaxCount > 0)
    {
        // during dynamax, only calculate max moves
        calculateBestMoveFromList(m_rentalCurrent.m_maxMoves, true);
    }
    else
    {
        // normal form
        calculateBestMoveFromList(m_rentalCurrent.m_moves, false);
    }

    std::sort(m_moveScoreList.begin(), m_moveScoreList.end(),
        [](MoveScore const& a, MoveScore const& b)
        {
            return a.m_score > b.m_score;
        }
    );
}

void SmartMaxLair::populateMaxLairBoss(QComboBox *cb)
{
    QStringList const& list = PokemonDatabase::getList_SwShMaxLairBoss();
    cb->addItems(list);
}

bool SmartMaxLair::populateMaxLairMatchupData()
{
    if (!m_matchupData.isEmpty()) return true;

    QJsonObject jsonObject;
    bool success = PokemonDatabase::readJson(RESOURCES_PATH + QString("PokemonSwSh/MaxLair/Boss-Matchup.json"), jsonObject);
    if (!success || jsonObject.isEmpty())
    {
        return false;
    }

    // Read all entries from json file
    m_matchupData.clear();
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
    {
        //qDebug() << it.key();
        QVector<double>& matchups = m_matchupData[it.key()];
        matchups.resize(47);

        QJsonObject bossObject = it.value().toObject();
        int index = 0;
        for (auto itBoss = bossObject.begin(); itBoss != bossObject.end(); ++itBoss)
        {
            //qDebug() << itBoss.key() << itBoss.value().toDouble();
            matchups[index] = itBoss.value().toDouble();
            index++;
        }
    }

    return true;
}

bool SmartMaxLair::populateMaxLairRentalBossData()
{
    if (!m_bossData.isEmpty() && !m_rentalData.isEmpty()) return true;

    auto readData = [](QString const& fileName, QVector<IDRentalPair>& list) -> bool
    {
        QJsonObject jsonObject;
        bool success = PokemonDatabase::readJson(RESOURCES_PATH + QString("PokemonSwSh/MaxLair/") + fileName + ".json", jsonObject);
        if (!success || jsonObject.isEmpty())
        {
            return false;
        }

        // Read all entries from json file
        list.clear();
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
        {
            //qDebug() << it.key();
            list.push_back(IDRentalPair(it.key(), RentalData()));
            RentalData& data = list[list.size() - 1].second;
            data.m_types[0] = MT_COUNT;
            data.m_types[1] = MT_COUNT;

            QJsonObject dataObject = it.value().toObject();
            for (auto itData = dataObject.begin(); itData != dataObject.end(); ++itData)
            {
                if (itData.key() == "id")
                {
                    //qDebug() << "id" << itData.value().toString();
                    data.m_name = itData.value().toString();
                }
                else if (itData.key() == "ability_id")
                {
                    //qDebug() << "ability_id" << itData.value().toString();
                    data.m_ability = itData.value().toString();
                }
                else if (itData.key() == "base_stats")
                {
                    int index = 0;
                    for (QJsonValueRef value : itData.value().toArray())
                    {
                        //qDebug() << "stat" << index << ":" << value.toInt();
                        data.m_stats[index] = value.toInt();
                        index++;
                    }
                }
                else if (itData.key() == "moves")
                {
                    for (QJsonValueRef value : itData.value().toArray())
                    {
                        //qDebug() << "move" << data.m_moves.size() << ":" << value.toInt();
                        data.m_moves.push_back(value.toInt());
                    }
                }
                else if (itData.key() == "max_moves")
                {
                    for (QJsonValueRef value : itData.value().toArray())
                    {
                        //qDebug() << "maxMove" << data.m_maxMoves.size() << ":" << value.toInt();
                        data.m_maxMoves.push_back(value.toInt());
                    }
                }
                else if (itData.key() == "types")
                {
                    int index = 0;
                    for (QJsonValueRef value : itData.value().toArray())
                    {
                        //qDebug() << "types" << index << ":" << value.toString();
                        data.m_types[index++] = PokemonDatabase::getMoveTypeFromString(value.toString());
                    }
                }
            }
        }

        return true;
    };

    return readData("Pokemon-BossData", m_bossData) && readData("Pokemon-RentalData", m_rentalData);
}

bool SmartMaxLair::populateMaxLairMoveData()
{
    if (!m_moveData.isEmpty()) return true;

    QJsonObject jsonObject;
    bool success = PokemonDatabase::readJson(RESOURCES_PATH + QString("PokemonSwSh/MaxLair/Pokemon-MovesData.json"), jsonObject);
    if (!success || jsonObject.isEmpty())
    {
        return false;
    }

    // Read all entries from json file
    m_moveData.clear();
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
    {
        //qDebug() << it.key();
        MoveData& data = m_moveData[it.key().toInt()];

        QJsonObject dataObject = it.value().toObject();
        for (auto itData = dataObject.begin(); itData != dataObject.end(); ++itData)
        {
            if (itData.key() == "name")
            {
                data.m_name = itData.value().toString();
                //qDebug() << "id" << data.m_name;
            }
            else if (itData.key() == "pp")
            {
                data.m_pp = itData.value().toInt();
                //qDebug() << "pp" << data.m_pp;
            }
            else if (itData.key() == "base_power")
            {
                data.m_power = itData.value().toInt();
                //qDebug() << "base_power" << data.m_power;
            }
            else if (itData.key() == "accuracy")
            {
                data.m_accuracy = itData.value().toDouble();
                //qDebug() << "accuracy" << data.m_accuracy;
            }
            else if (itData.key() == "correction_factor")
            {
                data.m_factor = itData.value().toDouble();
                //qDebug() << "correction_factor" << data.m_factor;
            }
            else if (itData.key() == "category")
            {
                data.m_isSpecial = itData.value().toString() == "special";
                //qDebug() << "isSpecial" << data.m_isSpecial;
            }
            else if (itData.key() == "type")
            {
                data.m_type = PokemonDatabase::getMoveTypeFromString(itData.value().toString());
                //qDebug() << "type" << itData.value().toString();
            }
        }
    }

    return true;
}
