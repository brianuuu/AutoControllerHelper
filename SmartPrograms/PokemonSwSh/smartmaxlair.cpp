#include "smartmaxlair.h"

QVector<SmartMaxLair::IDRentalPair> SmartMaxLair::m_bossData;
QVector<SmartMaxLair::IDRentalPair> SmartMaxLair::m_rentalData;
PokemonDatabase::OCREntries SmartMaxLair::m_allBossEntries;
PokemonDatabase::OCREntries SmartMaxLair::m_allRentalEntries;
QMap<QString, QVector<double>> SmartMaxLair::m_matchupData;
QMap<int, SmartMaxLair::MoveData> SmartMaxLair::m_moveData;

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
}

void SmartMaxLair::init()
{
    SmartProgramBase::init();
}

void SmartMaxLair::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartMaxLair::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statError, "Errors");
        m_timer.restart();

        bool testRentalSelect = true;
        if (testRentalSelect)
        {
            m_substage = SS_RentalSelect;
            setState_frameAnalyzeRequest();
            break;
        }

        m_substage = SS_Start;
        setState_runCommand("Nothing,5");

        m_videoManager->setAreas({A_Selection[1]});
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
            }
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Selection[1].m_rect, C_Color_Black, 180))
            {
                m_substage = SS_BossSelect;
                m_videoManager->setAreas({A_Selection[0], A_Selection[1], A_Selection[2]});
            }

            setState_runCommand("A,42");
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
                if (m_programSettings.m_bossDownPress > 0)
                {
                    command = "DDown,1,Nothing,1,Loop," + QString::number(m_programSettings.m_bossDownPress) + "," + command;
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
                    QString id;
                    int selectIndex = -1;
                    double maxScore = 0;

                    for (int i = 0; i < 3; i++)
                    {
                        RentalSearch const& search = m_rentalSearch[i];
                        if (search.m_name.isEmpty() || search.m_ability.isEmpty() || search.m_firstMove < 0)
                        {
                            continue;
                        }

                        bool found = false;
                        for (IDRentalPair const& idPair : m_rentalData)
                        {
                            RentalData const& data = idPair.second;
                            if (data.m_name == search.m_name && data.m_moves.at(0) == search.m_firstMove && data.m_ability == search.m_ability)
                            {
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

                        double score = m_matchupData[id].at(m_programSettings.m_bossIndex);
                        emit printLog("Rental Pokemon '" + id + "' VS '" + m_bossData[m_programSettings.m_bossIndex].first + "' score: " + QString::number(score));
                        if (score > maxScore)
                        {
                            maxScore = score;
                            selectIndex = i;
                        }
                    }

                    if (selectIndex >= 0)
                    {
                        emit printLog("Selecting Slot " + QString::number(selectIndex + 1) + ": " + m_rentalSearch[selectIndex].m_name, LOG_IMPORTANT);
                        QString command = "A,1,Nothing,100";
                        if (selectIndex > 0)
                        {
                            command = "DDown,1,Nothing,1,Loop," + QString::number(selectIndex) + "," + command;
                        }

                        m_substage = SS_FindPath;
                        setState_runCommand(command);

                        m_timer.restart();
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
        }
        else if (state == S_CaptureReady)
        {
            // TODO:
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
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
            if (itData.key() == "pp")
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
        }
    }

    return true;
}
