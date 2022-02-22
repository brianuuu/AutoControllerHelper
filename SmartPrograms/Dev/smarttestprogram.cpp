#include "smarttestprogram.h"

SmartTestProgram::SmartTestProgram(SmartProgramParameter parameter) : SmartProgramBase(parameter)
{
    init();
}

void SmartTestProgram::init()
{
    SmartProgramBase::init();
}

void SmartTestProgram::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartTestProgram::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Loop;
        setState_runCommand("Nothing,5");
        break;
    }
    case SS_Loop:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_runCommand("DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5,"
                                "DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5,"
                                "DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5,DRight,1,Nothing,5", true);
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() < 2000)
            {
                setState_frameAnalyzeRequest();
            }
            else
            {
                qDebug() << "INTERRUPT";
                m_timer.restart();
                setState_runCommand("Nothing,20", true);
                m_substage = SS_Interrupt;
            }
        }
        break;
    }
    case SS_Interrupt:
    {
        if (state == S_CommandFinished)
        {
            if (m_timer.elapsed() < 500)
            {
                emit printLog("Interrupt command finished earlier than expected", LOG_ERROR);
            }
            setState_runCommand("BSpam,40");
            m_substage = SS_Loop;
        }
    }
    }

    SmartProgramBase::runNextState();
}
