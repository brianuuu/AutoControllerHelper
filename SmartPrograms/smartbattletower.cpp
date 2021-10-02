#include "smartbattletower.h"

SmartBattleTower::SmartBattleTower
(
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();
}

void SmartBattleTower::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartBattleTower::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartBattleTower::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Loop;
        setState_runCommand(C_Loop);
        emit printLog("WARNING: This program will not stop on it's own!", LOG_WARNING);
        break;
    }
    case SS_Loop:
    {
        // Loops forever
        if (state == S_CommandFinished)
        {
            setState_runCommand(C_Loop);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
