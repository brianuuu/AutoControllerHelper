#include "smartplapasturesorter.h"

SmartPLAPastureSorter::SmartPLAPastureSorter
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_settings(setting)
{
    init();
}

void SmartPLAPastureSorter::init()
{
    SmartProgramBase::init();
}

void SmartPLAPastureSorter::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartPLAPastureSorter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_readyNextCheck = false;
        m_position = Position();
        m_positionTemp = Position();
        m_dataTemp = PokemonData();
        m_pokemonData.clear();
        m_pokemonDataSorted.clear();
        m_searchResult = SearchResult();

        m_substage = SS_MoveMode;
        setState_runCommand("Plus,10,B,10,X,1,B,1,Y,20");
        break;
    }
    case SS_MoveMode:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Scan;
            setState_runCommand("Nothing,20", true);

            m_videoManager->setAreas({A_Level, A_Stat, A_Shiny, A_Alpha});

            // Reserve pokemon data
            m_pokemonData.resize(m_settings.m_pastureCount * 30);
            emit printLog("Scanning " + QString::number(m_settings.m_pastureCount) + " Pasture(s)...");
        }
        break;
    }
    case SS_Scan:
    {
        bool isGotoNextPokemon = false;
        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Stat.m_rect, C_Color_Stat, 230))
            {
                m_dataTemp.m_isShiny = !checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Stat, 230);
                m_dataTemp.m_isAlpha = !checkBrightnessMeanTarget(A_Alpha.m_rect, C_Color_Stat, 230);

                setState_ocrRequest(A_Level.m_rect, C_Color_Text);
                runNextStateContinue();
            }
            else
            {
                // No pokemon
                m_dataTemp = PokemonData();
                isGotoNextPokemon = true;
                m_readyNextCheck = true;
            }
        }
        else if (state == S_CommandFinished || state == S_OCRReady)
        {
            if (state == S_OCRReady)
            {
                if (!getOCRNumber(m_dataTemp.m_dexNum))
                {
                    setState_error("Unable to detect Pokemon's level");
                }
            }

            if (m_readyNextCheck)
            {
                // Push current pokemon data, reset
                QString str = getPositionString(m_positionTemp) + " - ";
                if (m_dataTemp.m_dexNum == 0)
                {
                    emit printLog(str + "No Pokemon");
                }
                else
                {
                    emit printLog(str + getPokemonDataString(m_dataTemp));
                }

                m_pokemonData[getIDFromPosition(m_positionTemp)] = m_dataTemp;
                m_dataTemp = PokemonData();

                if (m_positionTemp.m_pasture == m_settings.m_pastureCount && m_positionTemp.m_point == QPoint(6,5))
                {
                    emit printLog("Scan completed, now sorting...");

                    // Finished scanning, sort
                    if (m_settings.m_livingDex)
                    {
                        QSet<int> idUsedForLivingDex;

                        // Sort by living dex first
                        int firstNonEmptySlot = -1;
                        int lastNonEmptySlot = -1;
                        for (int i = 1; i <= 270; i++)
                        {
                            // Last entry is 242: Darkrai
                            if (i > 242)
                            {
                                m_pokemonDataSorted.push_back(PokemonData());
                                continue;
                            }

                            PokemonData dexEntry(i, m_settings.m_livingDexShiny, m_settings.m_livingDexAlpha);
                            int idResult = findUnsortedResult(m_pokemonData, dexEntry);
                            if (idResult < 0)
                            {
                                // Empty slot
                                m_pokemonDataSorted.push_back(PokemonData());
                            }
                            else
                            {
                                m_pokemonDataSorted.push_back(dexEntry);
                                idUsedForLivingDex.insert(idResult);

                                // Remember the first non-empty slot
                                if (firstNonEmptySlot == -1)
                                {
                                    firstNonEmptySlot = m_pokemonDataSorted.size() - 1;
                                }

                                // Remember the last non-empty slot
                                lastNonEmptySlot = m_pokemonDataSorted.size() - 1;
                            }
                        }

                        // Clear leading boxes that are empty
                        if (!m_pokemonDataSorted.isEmpty())
                        {
                            int removeBoxCount = (firstNonEmptySlot == -1) ? 0 : (firstNonEmptySlot / 30);
                            for (int i = 0; i < removeBoxCount * 30; i++)
                            {
                                m_pokemonDataSorted.pop_front();
                            }
                            qDebug() << "firstNonEmptySlot:" << firstNonEmptySlot;
                            qDebug() << "Remove" << removeBoxCount << "empty leading boxes, pokemon count =" << m_pokemonDataSorted.size();
                        }

                        // Clear lagging boxes that are empty
                        if (!m_pokemonDataSorted.isEmpty())
                        {
                            int removeBoxCount = (lastNonEmptySlot == -1) ? 9 : (8 - lastNonEmptySlot / 30);
                            for (int i = 0; i < removeBoxCount * 30; i++)
                            {
                                m_pokemonDataSorted.pop_back();
                            }
                            qDebug() << "lastNonEmptySlot:" << lastNonEmptySlot;
                            qDebug() << "Remove" << removeBoxCount << "empty lagging boxes, pokemon count =" << m_pokemonDataSorted.size();
                        }

                        Q_ASSERT(m_pokemonDataSorted.size() % 30 == 0);
                        int livingDexBoxes = m_pokemonDataSorted.size() / 30;

                        // Push remaining pokemon
                        for (int i = 0; i < m_pokemonData.size(); i++)
                        {
                            if (!idUsedForLivingDex.contains(i) && m_pokemonData[i].m_dexNum != 0)
                            {
                                m_pokemonDataSorted.push_back(m_pokemonData[i]);
                            }
                        }
                        qDebug() << "Added remaining pokemon, pokemon count =" << m_pokemonDataSorted.size();

                        // Push until it's multiple of 30
                        int emptySlotRemainder = m_pokemonDataSorted.size() % 30;
                        if (emptySlotRemainder != 0)
                        {
                            for (int i = 0; i < 30 - emptySlotRemainder; i++)
                            {
                                m_pokemonDataSorted.push_back(PokemonData());
                            }
                        }
                        qDebug() << "Added empty slots, pokemon count =" << m_pokemonDataSorted.size();

                        // Sort the remaining
                        std::sort(m_pokemonDataSorted.begin() + livingDexBoxes * 30, m_pokemonDataSorted.end(), PastureSort());

                        // Update pasture count, expand data if necessary, assuming they are empty
                        int pastureCount = m_settings.m_pastureCount;
                        int pastureCountSorted = m_pokemonDataSorted.size() / 30;
                        if (pastureCount > pastureCountSorted)
                        {
                            // Sorted has less box than non-sorted, pad them
                            qDebug() << "Empty pasture count =" << pastureCount - pastureCountSorted;
                            for (int i = 0; i < (pastureCount - pastureCountSorted) * 30; i++)
                            {
                                m_pokemonDataSorted.push_back(PokemonData());
                            }
                        }
                        else if (pastureCount < pastureCountSorted)
                        {
                            // Require extra pastures
                            m_settings.m_pastureCount = pastureCountSorted;

                            qDebug() << "Extra pasture count =" << pastureCountSorted - pastureCount;
                            QString warning = "Creating Living Dex required " + QString::number(pastureCountSorted - pastureCount) + " extra pastures!";
                            emit printLog(warning, LOG_WARNING);

                            QMessageBox::StandardButton resBtn = QMessageBox::Yes;
                            resBtn = QMessageBox::warning(this, "Warning", warning + "\nAre you sure you want to continue?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                            if (resBtn == QMessageBox::Yes)
                            {
                                for (int i = 0; i < (pastureCountSorted - pastureCount) * 30; i++)
                                {
                                    m_pokemonData.push_back(PokemonData());
                                }
                            }
                            else
                            {
                                emit printLog("Stopped by user");
                                setState_completed();
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Default sorting
                        m_pokemonDataSorted = m_pokemonData;
                        std::sort(m_pokemonDataSorted.begin(), m_pokemonDataSorted.end(), PastureSort());
                    }

                    Q_ASSERT(m_pokemonData.size() == m_pokemonDataSorted.size());
                    /*PastureDebug(m_pokemonData);
                    qDebug() << "SORTING...";
                    PastureDebug(m_pokemonDataSorted);*/

                    if (m_pokemonData == m_pokemonDataSorted)
                    {
                        // Already finished, return to first pokemon
                        m_substage = SS_Finish;
                        setState_runCommand(gotoPosition(m_position, Position(), false));
                    }
                    else
                    {
                        // Mark those that already have correct position
                        for (int i = 0; i < m_pokemonData.size(); i++)
                        {
                            if (m_pokemonData[i] == m_pokemonDataSorted[i])
                            {
                                m_pokemonData[i].m_isSorted = true;
                            }
                        }

                        m_substage = SS_Sort;
                        setState_runCommand("Nothing,2");

                        // We are at the end of list, so it should be close to result
                        m_searchResult.m_idCurrent = -1;
                        m_searchResult.m_isNearResult = true;
                    }

                    m_videoManager->clearCaptures();
                }
                else
                {
                    // Ready to check next pokemon
                    setState_frameAnalyzeRequest();
                }
            }
            else
            {
                // Command or OCR is finished
                m_readyNextCheck = true;
            }
        }
        else if (state == S_OCRRequested)
        {
            // Immediately goto next pokemon while we wait for OCR result to save time
            isGotoNextPokemon = true;
            m_readyNextCheck = false;
        }

        if (isGotoNextPokemon)
        {
            m_positionTemp = m_position; // cache position
            if (m_positionTemp.m_pasture == m_settings.m_pastureCount && m_positionTemp.m_point == QPoint(6,5))
            {
                setState_runCommand("Nothing,2");
            }
            else
            {
                setState_runCommand(gotoNextPokemon(m_position, true));
            }
        }
        break;
    }
    case SS_Sort:
    {
        if (state == S_CommandFinished)
        {
            m_searchResult.m_idCurrent++;
            if (m_searchResult.m_idCurrent >= m_pokemonData.size())
            {
                // Finished
                m_substage = SS_Finish;
                setState_runCommand(gotoPosition(m_position, Position(), false));
            }
            else
            {
                if (m_pokemonData[m_searchResult.m_idCurrent].m_isSorted)
                {
                    // Already sorted
                    m_searchResult.m_idResult = m_searchResult.m_idCurrent;
                }
                else
                {
                    // Get the result ID of the current slot
                    m_searchResult.m_idResult = findUnsortedResult(m_pokemonData, m_pokemonDataSorted[m_searchResult.m_idCurrent]);
                    if (m_searchResult.m_idResult < 0)
                    {
                        setState_error("Unable to find Pokemon to sort");
                        break;
                    }
                    else if (m_pokemonData[m_searchResult.m_idResult].m_dexNum != 0)
                    {
                        // Mark pokemon as sorted (we don't care about empty slots)
                        m_pokemonData[m_searchResult.m_idResult].m_isSorted = true;
                    }
                }

                Position currentPos = getPositionFromID(m_searchResult.m_idCurrent);
                Position resultPos = getPositionFromID(m_searchResult.m_idResult);

                if (m_searchResult.m_idCurrent == m_searchResult.m_idResult)
                {
                    // Both empty or already sorted
                    emit printLog(getPositionString(currentPos) + " - (Already sorted) " + getPokemonDataString(m_pokemonDataSorted[m_searchResult.m_idCurrent]));
                    runNextStateContinue();
                    break;
                }

                if (m_pokemonData[m_searchResult.m_idCurrent].m_dexNum != 0 && m_pokemonData[m_searchResult.m_idResult].m_dexNum == 0)
                {
                    // Current slot need to be replace with empty slot
                    // but we can ignore this since later on the slot that need current slot will grab this
                    emit printLog(getPositionString(currentPos) + " has pokemon but will be an empty slot, ignoring for now...");
                    runNextStateContinue();
                    break;
                }

                emit printLog(getPositionString(currentPos) + " swapping with " + getPositionString(resultPos) + " - " + getPokemonDataString(m_pokemonData[m_searchResult.m_idResult]));
                if (m_pokemonData[m_searchResult.m_idCurrent].m_dexNum == 0 || (m_searchResult.m_isNearResult && m_pokemonData[m_searchResult.m_idResult].m_dexNum != 0))
                {
                    // Grab result slot and put it to current slot
                    setState_runCommand(gotoPosition(m_position, resultPos, false) + ",A,5,Loop,1," + gotoPosition(resultPos, currentPos, false) + ",A,5");
                    m_position = currentPos;
                    m_searchResult.m_isNearResult = false;
                }
                else
                {
                    // Grab current slot and put it to result slot
                    setState_runCommand(gotoPosition(m_position, currentPos, false) + ",A,5,Loop,1," + gotoPosition(currentPos, resultPos, false) + ",A,5");
                    m_position = resultPos;
                    m_searchResult.m_isNearResult = true;
                }

                // Swap data position
                qSwap(m_pokemonData[m_searchResult.m_idCurrent], m_pokemonData[m_searchResult.m_idResult]);
            }
        }
        break;
    }
    case SS_Finish:
    {
        if (state == S_CommandFinished)
        {
            // If we are here, m_pokemonData and m_pokemonDataSorted should be the same
            emit printLog("Sort completed!", LOG_SUCCESS);
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

int SmartPLAPastureSorter::findUnsortedResult(const QVector<PokemonData> &dataAll, const SmartPLAPastureSorter::PokemonData &dataQuery)
{
    for (int i = 0; i < dataAll.size(); i++)
    {
        PokemonData const& data = dataAll[i];
        if (!data.m_isSorted && data.m_dexNum == dataQuery.m_dexNum && data.m_isAlpha == dataQuery.m_isAlpha && data.m_isShiny == dataQuery.m_isShiny)
        {
            return i;
        }
    }
    return -1;
}

QString SmartPLAPastureSorter::gotoNextPokemon(Position &pos, bool addDelay)
{
    // This does a zig-zag path to save a bit of time within the same box
    Position posPrev = pos;
    if (pos.m_point == QPoint(6,5))
    {
        pos.m_pasture++;
        pos.m_point = QPoint(1,1);
    }
    else
    {
        if (pos.m_point.y() % 2 == 1)
        {
            if (pos.m_point.x() == 6)
            {
                pos.m_point.ry()++;
            }
            else
            {
                pos.m_point.rx()++;
            }
        }
        else
        {
            if (pos.m_point.x() == 1)
            {
                pos.m_point.ry()++;
            }
            else
            {
                pos.m_point.rx()--;
            }
        }
    }

    return gotoPosition(posPrev, pos, addDelay);
}

QString SmartPLAPastureSorter::gotoPosition(SmartPLAPastureSorter::Position from, SmartPLAPastureSorter::Position to, bool addDelay)
{
    // Move to pasture first
    QString command;
    if (from.m_pasture != to.m_pasture)
    {
        int diff = qAbs(from.m_pasture - to.m_pasture);
        command += (from.m_pasture < to.m_pasture) ? "R" : "L";
        command += ",1,Nothing,5,Loop," + QString::number(diff);
    }

    // Move to position X
    if (from.m_point.x() != to.m_point.x())
    {
        if (!command.isEmpty()) command += ",";

        int diff = qAbs(from.m_point.x() - to.m_point.x());
        QString direction = (from.m_point.x() < to.m_point.x()) ? "Right" : "Left";
        if (diff % 2 == 1)
        {
            command += "D" + direction + ",1,Loop,1";
            if (diff >= 2) command += ",";
        }

        if (diff >= 2)
        {
            command += "L" + direction + ",1,D" + direction + ",1,Loop," + QString::number(diff / 2);
        }
    }

    // Move to position Y
    if (from.m_point.y() != to.m_point.y())
    {
        if (!command.isEmpty()) command += ",";

        int diff = qAbs(from.m_point.y() - to.m_point.y());
        QString direction = (from.m_point.y() < to.m_point.y()) ? "Down" : "Up";
        if (diff % 2 == 1)
        {
            command += "D" + direction + ",1,Loop,1";
            if (diff >= 2) command += ",";
        }

        if (diff >= 2)
        {
            command += "L" + direction + ",1,D" + direction + ",1,Loop," + QString::number(diff / 2);
        }
    }

    // Dummy command if we're not moving at all
    if (command.isEmpty()) command = "Nothing,1";

    // Add delay to account for camera delay
    if (addDelay)
    {
        command += ",Nothing,20";
    }
    return command;
}

int SmartPLAPastureSorter::getIDFromPosition(SmartPLAPastureSorter::Position pos)
{
    return (pos.m_pasture - 1) * 30 + (pos.m_point.y() - 1) * 6 + (pos.m_point.x() - 1);
}

SmartPLAPastureSorter::Position SmartPLAPastureSorter::getPositionFromID(int id)
{
    Position pos;
    pos.m_pasture = (id / 30) + 1;
    pos.m_point.ry() = ((id % 30) / 6) + 1;
    pos.m_point.rx() = (id % 6) + 1;
    return pos;
}

QString SmartPLAPastureSorter::getPositionString(SmartPLAPastureSorter::Position pos)
{
    return "Pos {" + QString::number(pos.m_pasture) + "," + QString::number(pos.m_point.y()) + "," + QString::number(pos.m_point.x()) + "}";
}

QString SmartPLAPastureSorter::getPokemonDataString(const SmartPLAPastureSorter::PokemonData &data)
{
    QString padding = (data.m_dexNum < 10) ? "00" : ((data.m_dexNum < 100) ? "0" : "");
    return "Dex No.: " + padding + QString::number(data.m_dexNum) + (data.m_isAlpha ? ", Alpha" : "") + (data.m_isShiny ? ", Shiny" : "");
}

void SmartPLAPastureSorter::PastureDebug(const QVector<SmartPLAPastureSorter::PokemonData> &dataAll)
{
    for (PokemonData const& data : dataAll)
    {
        qDebug() << "Dex No: " + QString::number(data.m_dexNum) + (data.m_isAlpha ? ", Alpha" : "") + (data.m_isShiny ? ", Shiny" : "");
    }
}
