#include "smartboxrelease.h"

SmartBoxRelease::SmartBoxRelease
(
    int targetBoxCount,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_targetBoxCount(targetBoxCount)
{
    init();
}

void SmartBoxRelease::init()
{
    SmartProgramBase::init();
}

void SmartBoxRelease::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_releaseCount = 0;
}

void SmartBoxRelease::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_ReleaseConfirm;
        setState_runCommand("Nothing,1");
        break;
    }
    case SS_ReleaseHasPokemon:
    {
        if (state == S_CommandFinished)
        {
            // no pokemon, next
            m_releaseCount++;
            emit printLog(GetCurrentBoxPosString(m_releaseCount) + " has no Pokemon");

            m_substage = SS_ReleaseConfirm;
            setState_runCommand("Nothing,1");
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_DialogBox.m_rect, C_Color_Dialog, 240))
            {
                m_substage = SS_ReleaseYesNo;
                setState_runCommand("DUp,1,LUp,1,A,20");
                m_videoManager->setAreas({A_No,A_DialogBox,A_Shiny});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ReleaseYesNo:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 1000)
            {
                if (checkBrightnessMeanTarget(A_DialogBox.m_rect, C_Color_Dialog, 240))
                {
                    m_releaseCount++;
                    emit printLog(GetCurrentBoxPosString(m_releaseCount) + " cannot be released, was it an egg?", LOG_WARNING);

                    m_substage = SS_ReleaseConfirm;
                    setState_runCommand("A,25");
                }
                else
                {
                    setState_error("Unable to detect Yes/No box when releasing Pokemon");
                }
            }
            else if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
            {
                // shiny, don't release
                m_releaseCount++;
                emit printLog(GetCurrentBoxPosString(m_releaseCount) + " is a SHINY, not releasing", LOG_WARNING);

                m_substage = SS_ReleaseConfirm;
                setState_runCommand("B,30");
            }
            else if (checkBrightnessMeanTarget(A_No.m_rect, C_Color_Black, 200))
            {
                m_releaseCount++;
                emit printLog(GetCurrentBoxPosString(m_releaseCount) + " is released");

                m_substage = SS_ReleaseConfirm;
                setState_runCommand("DUp,1,ASpam,50");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_ReleaseConfirm:
    {
        if (state == S_CommandFinished)
        {
            if (m_releaseCount / 30 == m_targetBoxCount)
            {
                m_substage = SS_Finished;
                setState_runCommand("Home,2");
            }
            else
            {
                m_substage = SS_ReleaseHasPokemon;
                m_videoManager->setAreas({A_DialogBox});

                if (m_releaseCount == 0)
                {
                    setState_runCommand("A,25", true);
                }
                else
                {
                    if (m_releaseCount % 30 == 0)
                    {
                        // next box
                        setState_runCommand("DRight,1,LRight,1,DDown,1,LDown,1,DDown,1,R,1,Nothing,5,A,25", true);
                    }
                    else if (m_releaseCount % 6 == 0)
                    {
                        // next row
                        setState_runCommand("DDown,1,A,25", true);
                    }
                    else if (((m_releaseCount % 30) / 6) % 2 == 0)
                    {
                        // odd row
                        setState_runCommand("DRight,1,A,25", true);
                    }
                    else
                    {
                        // even row
                        setState_runCommand("DLeft,1,A,25", true);
                    }
                }
            }
        }
        break;
    }
    case SS_Finished:
    {
        if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

QPoint SmartBoxRelease::GetCurrentBoxPosFromReleaseCount(int count)
{
    if (count < 1)
    {
        // wtf
        return QPoint(0,0);
    }

    // [0-29]
    count = (count - 1) % 30;

    int row = (count / 6) + 1;
    int column = (row % 2 == 0) ? column = 6 - (count % 6) : column = (count % 6) + 1;
    return QPoint(column, row);
}

QString SmartBoxRelease::GetCurrentBoxPosString(int count)
{
    QPoint pos = GetCurrentBoxPosFromReleaseCount(count);
    return "Column " + QString::number(pos.x()) + " row " + QString::number(pos.y()) + " in Box " + QString::number(((count - 1) / 30) + 1);
}
