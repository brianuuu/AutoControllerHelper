#include "smartwattfarmer.h"

SmartWattFarmer::SmartWattFarmer
(
    int skips,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_skipsLeft(skips)
    , m_daySkipped(0)
{
    init();
}

void SmartWattFarmer::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartWattFarmer::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartWattFarmer::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_SyncTime;
        setState_runCommand(C_SyncTime);
        if (m_skipsLeft == 0)
        {
            emit printLog("WARNING: This program will not stop on its own!", LOG_WARNING);
        }
        break;
    }
    case SS_SyncTime:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_SkipYear;
            switch (m_parameters.settings->getDateArrangement())
            {
                case DA_JP: setState_runCommand(C_SkipYearJP); break;
                case DA_EU: setState_runCommand(C_SkipYearEU); break;
                case DA_US: setState_runCommand(C_SkipYearUS); break;
            }
        }
        break;
    }
    case SS_SkipYear:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_BackToGame;
            setState_runCommand(C_BackToGame);
        }
        break;
    }
    case SS_BackToGame:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Loop;
            setState_runCommand(C_Loop);
            emit printLog("Collecting watts (Day " + QString::number(m_daySkipped + 1) + ", total collected: " + QString::number((m_daySkipped + 1) * 2000) + ")");
        }
        break;
    }
    case SS_Loop:
    {
        if (state == S_CommandFinished)
        {
            m_daySkipped++;
            if (m_skipsLeft > 0)
            {
                m_skipsLeft--;
                emit printLog("Skips remaining = " + QString::number(m_skipsLeft));

                if (m_skipsLeft == 0)
                {
                    setState_completed();
                    break;
                }
            }

            m_substage = SS_SyncTime;
            setState_runCommand(C_SyncTime);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
