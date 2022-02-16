#include "smartbdspduplicateitem1to30.h"

SmartBDSPDuplicateItem1to30::SmartBDSPDuplicateItem1to30
(
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();
}

void SmartBDSPDuplicateItem1to30::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBDSPDuplicateItem1to30::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_bagItemCount = 0;
    m_giveItemCount = 0;
    m_itemCount = 1;
    m_pos1st = QPoint(1,1);
    m_pos2nd = QPoint(1,1);
}

void SmartBDSPDuplicateItem1to30::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        // Quit box and enter
        m_substage = SS_Start;
        setState_runCommand(C_Start);
        break;
    }
    case SS_Start:
    case SS_1stNextPokemon:
    {
        if (state == S_CommandFinished)
        {
            // Give item to current pokemon
            m_substage = SS_GiveItem;
            setState_runCommand(C_GiveItem);
        }
        break;
    }
    case SS_GiveItem:
    {
        if (state == S_CommandFinished)
        {
            m_giveItemCount++;
            emit printLog("No. of item in box: " + QString::number(m_giveItemCount));
            if (m_giveItemCount == 30)
            {
                // Box is now full of items
                emit printLog("Box is now full of the same item, you can now use Box Operation to duplicate item from this box!");
                setState_completed();
            }
            else if (m_giveItemCount < m_itemCount)
            {
                // We still have more item left to give to pokemon
                m_substage = SS_1stNextPokemon;
                gotoNextPokemon(m_pos1st);
            }
            else
            {
                // No more item, start dup from 2nd box
                emit printLog("Duplicating item from 2nd menu...");
                m_substage = SS_2ndMenuEnter;
                setState_runCommand(C_2ndMenuEnter);
            }
        }
        break;
    }
    case SS_2ndMenuEnter:
    case SS_2ndNextPokemon:
    {
        if (state == S_CommandFinished)
        {
            // Bag item from current pokemon
            m_substage = SS_BagItem;
            setState_runCommand(C_BagItem);
        }
        break;
    }
    case SS_BagItem:
    {
        if (state == S_CommandFinished)
        {
            m_bagItemCount++;
            if (m_bagItemCount < m_itemCount)
            {
                // We still have more item to take from pokemon
                m_substage = SS_2ndNextPokemon;
                gotoNextPokemon(m_pos2nd);
            }
            else
            {
                // No more item to take, exit 2nd menu
                m_bagItemCount = 0;
                m_itemCount *= 2;
                m_pos2nd = QPoint(1,1);

                m_substage = SS_2ndMenuExit;
                setState_runCommand(C_2ndMenuExit);
            }
        }
        break;
    }
    case SS_2ndMenuExit:
    {
        if (state == S_CommandFinished)
        {
            // Return to giving item
            m_substage = SS_1stNextPokemon;
            gotoNextPokemon(m_pos1st);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartBDSPDuplicateItem1to30::gotoNextPokemon(QPoint &pos)
{
    if (pos == QPoint(6,5))
    {
        // This should be on 1st box (finished)
        setState_completed();
    }
    else
    {
        if (pos.y() % 2 == 1)
        {
            if (pos.x() == 6)
            {
                pos.ry()++;
                setState_runCommand("DDown,1,Nothing,6");
            }
            else
            {
                pos.rx()++;
                setState_runCommand("DRight,1,Nothing,6");
            }
        }
        else
        {
            if (pos.x() == 1)
            {
                pos.ry()++;
                setState_runCommand("DDown,1,Nothing,6");
            }
            else
            {
                pos.rx()--;
                setState_runCommand("DLeft,1,Nothing,6");
            }
        }
    }
}
