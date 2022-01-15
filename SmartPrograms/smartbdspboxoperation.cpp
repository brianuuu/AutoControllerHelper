#include "smartbdspboxoperation.h"

SmartBDSPBoxOperation::SmartBDSPBoxOperation
(
    BoxOperationType type,
    int boxCount,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_type(type)
    , m_boxCount(boxCount)
{
    init();

    if (m_type >= BOT_COUNT)
    {
        setState_error("Invalid box operation type!");
    }
}

void SmartBDSPBoxOperation::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPBoxOperation::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_box = 1;
    m_pos = QPoint(1,1);
}

void SmartBDSPBoxOperation::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        // Quit box and enter
        m_substage = SS_Start;
        setState_runCommand(m_type == BOT_BagItem ? C_StartItem : C_Start);
        break;
    }
    case SS_Start:
    case SS_NextPokemon:
    {
        if (state == S_CommandFinished)
        {
            switch (m_type)
            {
            case BOT_Release:
            {
                m_substage = SS_Release;
                setState_runCommand(C_Release);
                emit printLog(getLocation() + "Release pokemon");
                break;
            }
            case BOT_BagItem:
            {
                m_substage = SS_BagItem;
                setState_runCommand(C_BagItem);
                emit printLog(getLocation() + "Bag item");
                break;
            }
            case BOT_DupItem:
            {
                if (m_substage == SS_NextPokemon)
                {
                    // Take item
                    m_substage = SS_BagItem;
                    setState_runCommand(C_BagItem);
                    emit printLog(getLocation() + "Bag item");
                }
                else
                {
                    // Goto 2nd menu
                    m_substage = SS_DupItemStart;
                    setState_runCommand(C_DupItemStart);
                    emit printLog("Entering 2nd menu...");
                }
                break;
            }
            }
        }
        break;
    }
    case SS_Release:
    case SS_BagItem:
    {
        if (state == S_CommandFinished)
        {
            gotoNextPokemon();
        }
        break;
    }
    case SS_DupItemStart:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_BagItem;
            setState_runCommand(C_BagItem);
            emit printLog(getLocation() + "Bag item");
        }
        break;
    }
    case SS_DupItemEnd:
    {
        if (state == S_CommandFinished)
        {
            if (m_box < m_boxCount)
            {
                // Goto 2nd menu
                m_pos = QPoint(1,1);
                m_box++;
                m_substage = SS_DupItemStart;
                setState_runCommand(C_DupItemStart);
                emit printLog("Entering 2nd menu...");
            }
            else
            {
                setState_completed();
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartBDSPBoxOperation::gotoNextPokemon()
{
    m_substage = SS_NextPokemon;
    if (m_pos == QPoint(6,5))
    {
        if (m_type == BOT_DupItem)
        {
            // Quit 2nd menu
            m_substage = SS_DupItemEnd;
            setState_runCommand(C_DupItemEnd);
            emit printLog("Exiting 2nd menu...");
        }
        else if (m_box < m_boxCount)
        {
            // Go back to first pokemon and to next box
            m_pos = QPoint(1,1);
            m_box++;
            setState_runCommand("DLeft,1,Nothing,4,DUp,1,Nothing,4,Loop,4,DLeft,1,Nothing,4,R,22");
            emit printLog("Going to next box...");
        }
        else
        {
            // Finished
            setState_completed();
        }
    }
    else
    {
        if (m_pos.y() % 2 == 1)
        {
            if (m_pos.x() == 6)
            {
                m_pos.ry()++;
                setState_runCommand("DDown,1,Nothing,4");
            }
            else
            {
                m_pos.rx()++;
                setState_runCommand("DRight,1,Nothing,4");
            }
        }
        else
        {
            if (m_pos.x() == 1)
            {
                m_pos.ry()++;
                setState_runCommand("DDown,1,Nothing,4");
            }
            else
            {
                m_pos.rx()--;
                setState_runCommand("DLeft,1,Nothing,4");
            }
        }
    }
}

QString SmartBDSPBoxOperation::getLocation()
{
    return "Box " + QString::number(m_box) + " row " + QString::number(m_pos.y()) + " column " + QString::number(m_pos.x()) + ": ";
}
