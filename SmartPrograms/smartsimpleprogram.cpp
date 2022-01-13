#include "smartsimpleprogram.h"

SmartSimpleProgram::SmartSimpleProgram
(
    SmartProgram programEnum,
    int loopCount,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programEnum(programEnum)
    , m_loopCount(loopCount)
{
    init();

    if (m_loopCount == 0)
    {
        setState_error("loopCount cannot be 0!");
    }
}

void SmartSimpleProgram::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSimpleProgram::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartSimpleProgram::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Loop;
        setState_runCommand(C_Loop);
        if (m_loopCount == -1)
        {
            emit printLog("WARNING: This program will not stop on its own!", LOG_WARNING);
        }
        break;
    }
    case SS_Loop:
    {
        // Loops forever or run once
        if (state == S_CommandFinished)
        {
            if (m_loopCount > 0)
            {
                m_loopCount--;
                if (m_loopCount == 0)
                {
                    setState_completed();
                    break;
                }

                emit printLog("Loop remaining: " + QString::number(m_loopCount));
            }

            setState_runCommand(C_Loop);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
