#include "smartplapasturesorter.h"

SmartPLAPastureSorter::SmartPLAPastureSorter(SmartProgramParameter parameter) : SmartProgramBase(parameter)
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
    m_readyNextCheck = false;
    m_position = Position();
}

void SmartPLAPastureSorter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Scan;
        setState_runCommand("Nothing,20", true);

        m_parameters.vlcWrapper->setAreas({A_Level, A_Stat, A_Shiny, A_Alpha});

        // Reserve pokemon data
        m_settings.m_pastureCount = 10;
        m_pokemonData.resize(m_settings.m_pastureCount * 30);
        emit printLog("Scanning " + QString::number(m_settings.m_pastureCount) + " Pasture(s)...");
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
                QString str = getPositionString(m_positionTemp);
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
                    // Finished scanning, sort
                    m_pokemonDataSorted = m_pokemonData;
                    std::sort(m_pokemonDataSorted.begin(), m_pokemonDataSorted.end(), PastureSort());

                    /*PastureDebug(m_pokemonData);
                    qDebug() << "SORTING...";
                    PastureDebug(m_pokemonDataSorted);*/

                    emit printLog("Scan completed, now sorting...");

                    // Return to first pokemon
                    m_substage = SS_SortPokemon;
                    m_positionTemp = m_position;
                    m_position = Position();
                    setState_runCommand(gotoPosition(m_positionTemp, m_position, false));
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
                setState_runCommand("Nothing,5");
            }
            else
            {
                gotoNextPokemon(m_position, true);
            }
        }
        break;
    }
    case SS_SortStart:
    {
        if (state == S_CommandFinished)
        {
            if (m_position.m_pasture == m_settings.m_pastureCount && m_position.m_point == QPoint(6,5))
            {
                // Finished
                m_substage = SS_Finish;
                m_positionTemp = m_position;
                m_position = Position();
                setState_runCommand(gotoPosition(m_positionTemp, m_position, false));
            }
            else
            {
                m_substage = SS_SortPokemon;
                gotoNextPokemon(m_position, false);
            }
        }
        break;
    }
    case SS_SortPokemon:
    {
        if (state == S_CommandFinished)
        {
            int id = getIDFromPosition(m_position);
            int idResult = findUnusedResult(m_pokemonData, m_pokemonDataSorted[id]);
            if (idResult < 0)
            {
                setState_error("Unable to find Pokemon to sort");
                break;
            }

            emit printLog("Sorting " + getPositionString(m_position) + getPokemonDataString(m_pokemonDataSorted[id]));

            m_substage = SS_SortStart;
            if (id == idResult)
            {
                // No need to do anything
                setState_runCommand("Nothing,2");
                break;
            }

            // Swap data position
            qSwap(m_pokemonData[id], m_pokemonData[idResult]);

            // Do actual swap
            m_positionTemp = getPositionFromID(idResult);
            if (m_pokemonData[id].m_dexNum == 0 && m_pokemonDataSorted[id].m_dexNum == 0)
            {
                // Both empty
                setState_runCommand("Nothing,5");
            }
            else if (m_pokemonDataSorted[id].m_dexNum == 0)
            {
                // Result is empty, pickup the current and put to empty spot
                setState_runCommand("A,5,Loop,1," + gotoPosition(m_position, m_positionTemp, false) + ",A,5,Loop,1," + gotoPosition(m_positionTemp, m_position, false));
            }
            else
            {
                // Pickup the result and put it here
                setState_runCommand(gotoPosition(m_position, m_positionTemp, false) + ",A,5,Loop,1," + gotoPosition(m_positionTemp, m_position, false) + ",A,5");
            }
        }
        break;
    }
    case SS_Finish:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Sort completed!", LOG_SUCCESS);
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

int SmartPLAPastureSorter::findUnusedResult(QVector<SmartPLAPastureSorter::PokemonData> &dataAll, const SmartPLAPastureSorter::PokemonData &dataQuery)
{
    for (int i = 0; i < dataAll.size(); i++)
    {
        PokemonData& data = dataAll[i];
        if (!data.m_isUsed && data.m_dexNum == dataQuery.m_dexNum && data.m_isAlpha == dataQuery.m_isAlpha && data.m_isShiny == dataQuery.m_isShiny)
        {
            data.m_isUsed = true;
            return i;
        }
    }
    return -1;
}

void SmartPLAPastureSorter::gotoNextPokemon(Position &pos, bool addDelay)
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

    setState_runCommand(gotoPosition(posPrev, pos, addDelay));
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
        command += (from.m_point.x() < to.m_point.x()) ? "DRight" : "DLeft";
        command += ",1,Nothing,1,Loop," + QString::number(diff);
    }

    // Move to position Y
    if (from.m_point.y() != to.m_point.y())
    {
        if (!command.isEmpty()) command += ",";

        int diff = qAbs(from.m_point.y() - to.m_point.y());
        command += (from.m_point.y() < to.m_point.y()) ? "DDown" : "DUp";
        command += ",1,Nothing,1,Loop," + QString::number(diff);
    }

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
    return "Position {" + QString::number(pos.m_pasture) + "," + QString::number(pos.m_point.y()) + "," + QString::number(pos.m_point.x()) + "} - ";
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
