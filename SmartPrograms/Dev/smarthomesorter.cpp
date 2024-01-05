#include "smarthomesorter.h"

SmartHomeSorter::SmartHomeSorter
(
    Settings settings,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(settings)
{
    init();
}

void SmartHomeSorter::init()
{
    SmartProgramBase::init();
}

void SmartHomeSorter::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartHomeSorter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_pokemonCount = 0;
        m_pokemonCountBox = 0;

        m_currentID = 0;
        m_position = Position();
        m_pokemonData.resize(m_programSettings.m_count * 30);

        m_substage = SS_ScanBox;
        setState_frameAnalyzeRequest();

        emit printLog("Scanning number of Pokemon in each box...");
        m_videoManager->setAreas({A_ListStat});
        break;
    }
    case SS_ScanBox:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            // check if slot has pokemon
            m_currentID = getIDFromPosition(m_position);
            if (!checkBrightnessMeanTarget(A_ListStat.m_rect, C_Color_Summary, 230))
            {
                m_pokemonCount++;
                m_pokemonCountBox++;

                PokemonData& data = m_pokemonData[m_currentID];
                data.m_dexNum = -1; // set to -1 = there is a pokemon here
            }
            else
            {
                emit printLog(getPositionString(getPositionFromID(m_currentID)) + ": No Pokemon");
            }

            if (m_position.m_point != QPoint(5,4))
            {
                // next pokemon
                setState_runCommand(gotoNextPokemon(m_position, true));
                break;
            }

            emit printLog("Box " + QString::number(m_position.m_box + 1) + " has " + QString::number(m_pokemonCountBox) + " Pokemon");
            if (m_pokemonCountBox == 0)
            {
                if (m_position.m_box < m_programSettings.m_count - 1)
                {
                    // next box
                    Position target = Position();
                    target.m_box = m_position.m_box + 1;
                    setState_runCommand(gotoPosition(m_position, target, true));
                    m_position = target;
                }
                else
                {
                    // no pokemon in last box, start sorting
                    m_substage = SS_Sort;
                    setState_runCommand("Nothing,5");
                }
                break;
            }

            // go to first available pokemon in this box to check summary
            m_currentID = -1;
            for (int i = m_position.m_box * 30; i < (m_position.m_box + 1) * 30; i++)
            {
                if (m_pokemonData[i].m_dexNum == -1)
                {
                    m_currentID = i;
                    break;
                }
            }

            if (m_currentID == -1)
            {
                setState_error("Unable to find Pokemon to check summary");
                break;
            }

            m_substage = SS_Summary;
            Position target = getPositionFromID(m_currentID);
            setState_runCommand(gotoPosition(m_position, target, false) + ",A,1,DDown,1,A,50");
            m_position = target;

            m_videoManager->setAreas({A_Summary});
        }
        break;
    }
    case SS_Summary:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Summary.m_rect, C_Color_Summary, 180))
            {
                m_substage = SS_ScanDex;
                setState_runCommand("DRight,1,Nothing,25", true);

                m_checkCount = 0;
                m_videoManager->setAreas({A_Number, A_Shiny});
            }
            else
            {
                setState_error("Unable to detect Pokemon summary screen");
            }
        }
        break;
    }
    case SS_ScanDex:
    {
        if (state == S_CaptureReady)
        {
            m_pokemonData[m_currentID].m_isShiny = checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 50);
            setState_ocrRequest(A_Number.m_rect, C_Color_Text);
        }
        else if (state == S_CommandFinished || state == S_OCRReady)
        {
            if (state == S_OCRReady && !getOCRNumber(m_pokemonData[m_currentID].m_dexNum))
            {
                setState_error("Unable to detect Pokemon's dex number");
                break;
            }

            if (m_pokemonData[m_currentID].m_dexNum > m_maxDexNum)
            {
                setState_error("Invalid dex number");
                break;
            }

            // wait for both OCR and command to finish
            m_checkCount++;
            if (m_checkCount == 2)
            {
                emit printLog(getPositionString(getPositionFromID(m_currentID)) + ": No." + QString::number(m_pokemonData[m_currentID].m_dexNum) + (m_pokemonData[m_currentID].m_isShiny ? " (Shiny)" : ""));

                // find next pokemon to check
                m_checkCount = 0;
                while (m_currentID < m_pokemonData.size() && m_currentID < (m_position.m_box + 1) * 30)
                {
                    m_currentID++;
                    if (m_pokemonData[m_currentID].m_dexNum == -1)
                    {
                        break;
                    }
                }

                // scan completed
                if (m_currentID >= m_pokemonData.size())
                {
                    m_substage = SS_Sort;
                    setState_runCommand("B,60");
                    break;
                }

                // scan on current box is completed, go to next box
                if (m_currentID >= (m_position.m_box + 1) * 30)
                {
                    m_substage = SS_ScanBox;
                    Position target = Position();
                    target.m_box = m_position.m_box + 1;
                    setState_runCommand("B,60,Loop,1," + gotoPosition(m_position, target, true));
                    m_position = target;

                    m_pokemonCountBox = 0;
                    m_videoManager->setAreas({A_ListStat});
                    break;
                }

                // scan next pokemon in the same box
                setState_runCommand("DRight,1,Nothing,25", true);
            }
        }
        break;
    }
    case SS_Sort:
    {
        if (state == S_CommandFinished)
        {
            if (m_pokemonCount == 0)
            {
                setState_error("No Pokemon is found within all boxes");
                break;
            }

            int livingDexOutsideCount = 0;
            if (m_programSettings.m_livingDex)
            {
                m_pokemonDataSorted.resize(m_maxDexNum);
                for (int i = 0; i < m_pokemonData.size(); i++)
                {
                    int dexNum = m_pokemonData[i].m_dexNum;
                    if (dexNum == 0)
                    {
                        continue;
                    }

                    bool isShiny = m_pokemonData[i].m_isShiny;
                    if (isShiny == m_programSettings.m_livingDexShiny && m_pokemonDataSorted[dexNum - 1].m_dexNum == 0)
                    {
                        // slot isn't occupied yet
                        m_pokemonDataSorted[dexNum - 1] = m_pokemonData[i];
                    }
                    else
                    {
                        // slot already occupied or not same shiny setting, put it to default sort
                        livingDexOutsideCount++;
                        m_pokemonDataSorted.push_back(m_pokemonData[i]);
                    }
                }

                // sort remaining with default sort
                if (m_pokemonDataSorted.size() > m_maxDexNum)
                {
                    std::sort(m_pokemonDataSorted.begin() + m_maxDexNum, m_pokemonDataSorted.end(), BoxSort());
                }

                // expand data to maximum size required
                int targetSize = (m_pokemonDataSorted.size() + 29) / 30;
                if (targetSize > m_programSettings.m_count)
                {
                    m_pokemonData.resize(targetSize * 30);
                    m_programSettings.m_count = targetSize;
                }
                m_pokemonDataSorted.resize(m_programSettings.m_count * 30);

                emit printLog("Total Boxes required = " + QString::number(m_programSettings.m_count));
            }
            else
            {
                m_pokemonDataSorted = m_pokemonData;
                std::sort(m_pokemonDataSorted.begin(), m_pokemonDataSorted.end(), BoxSort());
            }

            // Mark those that already have correct position
            int sortedCount = 0;
            for (int i = 0; i < m_pokemonData.size(); i++)
            {
                if (m_pokemonData[i] == m_pokemonDataSorted[i])
                {
                    m_pokemonData[i].m_isSorted = true;
                    if (m_pokemonData[i].m_dexNum > 0)
                    {
                        sortedCount++;
                    }
                }
            }

            emit printLog("Pokemon Sorted: " + QString::number(sortedCount) + "/" + QString::number(m_pokemonCount));
            if (m_programSettings.m_livingDex)
            {
                emit printLog("No. of Pokemon Outside Living Dex: " + QString::number(livingDexOutsideCount));
            }

            // quit summary, start sorting
            m_substage = SS_Swap;
            runNextStateContinue();

            emit printLog("Sorting started...");
            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_Next:
    {
        if (state == S_CommandFinished)
        {
            // find the sort this should be put at
            int targetID = findClosestTargetID();
            if (targetID == -1)
            {
                setState_error("Unable to find target slot of the current Pokemon");
                break;
            }
            //qDebug() << m_currentID << "<->" << targetID;

            // swap data
            m_pokemonData[m_currentID].m_isSorted = true;
            if (m_pokemonData[targetID] == m_pokemonDataSorted[m_currentID])
            {
                // the opposite slot is also sorted
                m_pokemonData[targetID].m_isSorted = true;
            }
            qSwap(m_pokemonData[m_currentID], m_pokemonData[targetID]);

            // swap with target
            m_substage = SS_Swap;
            Position target = getPositionFromID(targetID);
            setState_runCommand("Y,10,Loop,1," + gotoPosition(m_position, target, false) + ",Y,1,Nothing,10");
            m_position = target;
        }
        break;
    }
    case SS_Swap:
    {
        if (state == S_CommandFinished)
        {
            m_currentID = findClosestUnsortedID();
            if (m_currentID == -1)
            {
                // Finished, return to first pokemon
                m_substage = SS_Finish;
                setState_runCommand(gotoPosition(m_position, Position(), false) + ",Home,1");
            }
            else
            {
                // goto the closest unsorted pokemon
                m_substage = SS_Next;
                Position target = getPositionFromID(m_currentID);
                setState_runCommand(gotoPosition(m_position, target, false));
                m_position = target;
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

int SmartHomeSorter::findClosestUnsortedID()
{
    // find any unsorted pokemon in the same box, then adjacent box, and so on
    int boxMove = 0;
    while (m_position.m_box + boxMove < m_programSettings.m_count || m_position.m_box - boxMove >= 0)
    {
        if (m_position.m_box + boxMove < m_programSettings.m_count)
        {
            // try find adjacent slots from current cursor position
            Position positionInBox = m_position;
            positionInBox.m_box += boxMove;

            int returnID = -1;
            int maxDist = 10;
            for (int i = (m_position.m_box + boxMove) * 30; i < (m_position.m_box + boxMove + 1) * 30; i++)
            {
                if (!m_pokemonData[i].m_isSorted && m_pokemonData[i].m_dexNum > 0)
                {
                    Position pos = getPositionFromID(i);
                    int dist = qAbs(pos.m_point.x() - positionInBox.m_point.x()) + qAbs(pos.m_point.y() - positionInBox.m_point.y());
                    if (dist < maxDist)
                    {
                        returnID = i;
                        maxDist = dist;
                    }
                }
            }

            if (returnID >= 0)
            {
                return returnID;
            }
        }

        if (boxMove == 0)
        {
            boxMove++;
            continue;
        }

        if (m_position.m_box - boxMove >= 0)
        {
            // try find adjacent slots from current cursor position
            Position positionInBox = m_position;
            positionInBox.m_box -= boxMove;

            int returnID = -1;
            int maxDist = 10;
            for (int i = (m_position.m_box - boxMove) * 30; i < (m_position.m_box - boxMove + 1) * 30; i++)
            {
                if (!m_pokemonData[i].m_isSorted && m_pokemonData[i].m_dexNum > 0)
                {
                    Position pos = getPositionFromID(i);
                    int dist = qAbs(pos.m_point.x() - positionInBox.m_point.x()) + qAbs(pos.m_point.y() - positionInBox.m_point.y());
                    if (dist < maxDist)
                    {
                        returnID = i;
                        maxDist = dist;
                    }
                }
            }

            if (returnID >= 0)
            {
                return returnID;
            }
        }

        boxMove++;
    }

    return -1;
}

int SmartHomeSorter::findClosestTargetID()
{
    // find the target id the current should be swap with
    int boxMove = 0;
    while (m_position.m_box + boxMove < m_programSettings.m_count || m_position.m_box - boxMove >= 0)
    {
        if (m_position.m_box + boxMove < m_programSettings.m_count)
        {
            for (int i = (m_position.m_box + boxMove) * 30; i < (m_position.m_box + boxMove + 1) * 30; i++)
            {
                if (m_pokemonData[m_currentID] == m_pokemonDataSorted[i] && !m_pokemonData[i].m_isSorted)
                {
                    return i;
                }
            }
        }

        if (boxMove == 0)
        {
            boxMove++;
            continue;
        }

        if (m_position.m_box - boxMove >= 0)
        {
            for (int i = (m_position.m_box - boxMove) * 30; i < (m_position.m_box - boxMove + 1) * 30; i++)
            {
                if (m_pokemonData[m_currentID] == m_pokemonDataSorted[i] && !m_pokemonData[i].m_isSorted)
                {
                    return i;
                }
            }
        }

        boxMove++;
    }

    return -1;
}

QString SmartHomeSorter::gotoPosition(Position from, Position to, bool addDelay)
{
    QString command;

    // Move to box
    if (from.m_box != to.m_box)
    {
        int diff = qAbs(to.m_box - from.m_box);
        command += (from.m_box < to.m_box) ? "R" : "L";
        command += ",1,Nothing,10,Loop," + QString::number(diff);
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

    // add delay
    if (addDelay)
    {
        command += ",Nothing,20";
    }

    return command;
}

QString SmartHomeSorter::gotoNextPokemon(Position &pos, bool addDelay)
{
    // This does a zig-zag path to save a bit of time within the same box
    Position posPrev = pos;
    if (pos.m_point == QPoint(5,4))
    {
        // should not be using this
        return "";
    }

    if (pos.m_point.y() % 2 == 0)
    {
        if (pos.m_point.x() == 5)
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
        if (pos.m_point.x() == 0)
        {
            pos.m_point.ry()++;
        }
        else
        {
            pos.m_point.rx()--;
        }
    }

    return gotoPosition(posPrev, pos, addDelay);
}

int SmartHomeSorter::getIDFromPosition(Position pos)
{
    return pos.m_box * 30 + pos.m_point.y() * 6 + pos.m_point.x();
}

SmartHomeSorter::Position SmartHomeSorter::getPositionFromID(int id)
{
    Position pos;
    pos.m_box = (id / 30);
    pos.m_point.ry() = (id % 30) / 6;
    pos.m_point.rx() = id % 6;
    return pos;
}

QString SmartHomeSorter::getPositionString(Position pos)
{
    return "Box " + QString::number(pos.m_box + 1) + ", Pos (" + QString::number(pos.m_point.y() + 1) + "," + QString::number(pos.m_point.x() + 1) + ")";
}

const CaptureArea SmartHomeSorter::GetCaptureAreaOfPos(int x, int y)
{
    // return capture area of location in box
    if (x < 0 || y < 0 || x > 5 || y > 4)
    {
        x = 0;
        y = 0;
    }

    return CaptureArea(52 + x * 92, 126 + y * 76, 92, 76);
}
