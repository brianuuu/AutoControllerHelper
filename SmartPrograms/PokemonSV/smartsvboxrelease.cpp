#include "smartsvboxrelease.h"

SmartSVBoxRelease::SmartSVBoxRelease
(
    int releaseTarget,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_releaseTarget(releaseTarget)
{
    init();
}

void SmartSVBoxRelease::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSVBoxRelease::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_releaseCount = 0;
    m_pos = QPoint(1,1);
}

void SmartSVBoxRelease::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Release;
        setState_runCommand(C_Release);
        break;
    }
    case SS_Release:
    {
        if (state == S_CommandFinished)
        {
            m_releaseCount++;
            int x = m_pos.x();
            int y = m_pos.y();
            emit printLog("Released pokemon at pos [" + QString::number(x) + "," + QString::number(y) + "], " + QString::number(m_releaseTarget - m_releaseCount) + " remaining");
            if (m_releaseCount >= m_releaseTarget)
            {
                setState_completed();
            }
            else if (m_pos.x() < 1 || m_pos.x() > 6 || m_pos.y() < 1 || m_pos.y() > 5)
            {
                setState_error("Memory corruption detected: pos = [" + QString::number(x) + "," + QString::number(y) + "]");
            }
            else if (m_pos.x() < 6)
            {
                m_substage = SS_Next;
                m_pos.rx()++;
                setState_runCommand(C_Next);
            }
            else if (m_pos.y() < 5)
            {
                m_substage = SS_Next;
                m_pos.rx() = 1;
                m_pos.ry()++;
                setState_runCommand(C_NextRow);
            }
            else
            {
                m_substage = SS_Next;
                emit printLog("Go to next box");
                m_pos.rx() = 1;
                m_pos.ry() = 1;
                setState_runCommand(C_NextBox);
            }
        }

        break;
    }
    case SS_Next:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Release;
            setState_runCommand(C_Release);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
