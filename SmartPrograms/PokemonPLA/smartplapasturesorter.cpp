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
        m_substage = SS_ScanPokemon;
        setState_runCommand("Nothing,20", true);

        m_parameters.vlcWrapper->setAreas({A_Level, A_Stat, A_Shiny, A_Alpha});

        // Reserve pokemon data
        m_settings.m_pastureCount = 2;
        m_pokemonData.resize(m_settings.m_pastureCount * 30);
        emit printLog("Scanning " + QString::number(m_settings.m_pastureCount) + " Pasture(s)...");
        break;
    }
    case SS_ScanPokemon:
    {
        static Position position = Position();
        static PokemonData data = PokemonData();
        bool isGotoNextPokemon = false;

        if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Stat.m_rect, C_Color_Stat, 230))
            {
                data.m_isShiny = !checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Stat, 230);
                data.m_isAlpha = !checkBrightnessMeanTarget(A_Alpha.m_rect, C_Color_Stat, 230);

                setState_ocrRequest(A_Level.m_rect, C_Color_Text);
                runNextStateContinue();
            }
            else
            {
                // No pokemon
                data = PokemonData();
                isGotoNextPokemon = true;
                m_readyNextCheck = true;
            }
        }
        else if (state == S_CommandFinished || state == S_OCRReady)
        {
            if (state == S_OCRReady)
            {
                if (!getOCRNumber(data.m_dexNum))
                {
                    setState_error("Unable to detect Pokemon's level");
                }
            }

            if (m_readyNextCheck)
            {
                // Push current pokemon data, reset
                QString str = getPositionString(position);
                if (data.m_dexNum == 0)
                {
                    emit printLog(str + "No Pokemon");
                }
                else
                {
                    QString padding = (data.m_dexNum < 10) ? "00" : ((data.m_dexNum < 100) ? "0" : "");
                    emit printLog(str + "Dex No.: " + padding + QString::number(data.m_dexNum) + ", Shiny: " + (data.m_isShiny ? "Y" : "N") + ", Alpha: " + (data.m_isAlpha ? "Y" : "N"));
                }

                m_pokemonData[getIDFromPosition(position)] = data;
                data = PokemonData();

                if (position.m_pasture == m_settings.m_pastureCount && position.m_point == QPoint(6,5))
                {
                    // TODO: Finished scanning
                    for (PokemonData const& d: m_pokemonData)
                    {
                        qDebug() << d.m_dexNum;
                    }
                    setState_completed();
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
            position = m_position; // cache position
            if (position.m_pasture == m_settings.m_pastureCount && position.m_point == QPoint(6,5))
            {
                setState_runCommand("Nothing,5");
            }
            else
            {
                gotoNextPokemon(m_position);
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartPLAPastureSorter::gotoNextPokemon(Position &pos)
{
    // This function is for scanning only, it will do zig-zag path to save a bit of time
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

    gotoPosition(posPrev, pos);
}

void SmartPLAPastureSorter::gotoPosition(SmartPLAPastureSorter::Position from, SmartPLAPastureSorter::Position to)
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
    command += ",Nothing,20";
    setState_runCommand(command);
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
