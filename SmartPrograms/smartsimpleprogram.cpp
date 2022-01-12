#include "smartsimpleprogram.h"

SmartSimpleProgram::SmartSimpleProgram
(
    SmartProgram programEnum,
    bool isLoop,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programEnum(programEnum)
    , m_isLoop(isLoop)
{
    init();
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
        if (m_isLoop)
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
            if (m_isLoop)
            {
                setState_runCommand(C_Loop);
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
