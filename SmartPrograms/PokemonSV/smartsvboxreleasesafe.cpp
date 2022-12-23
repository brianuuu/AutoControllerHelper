#include "smartsvboxreleasesafe.h"

#include "smartsveggoperation.h"

SmartSVBoxReleaseSafe::SmartSVBoxReleaseSafe
(
    int boxTarget,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_boxTarget(boxTarget)
{
    init();
}

void SmartSVBoxReleaseSafe::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSVBoxReleaseSafe::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_currentBox = 1;
    m_posToRelease = QPoint(1,1);
    m_fixAttempt = 0;
}

void SmartSVBoxReleaseSafe::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_CheckPosColumn1;
        setState_frameAnalyzeRequest();
        setAreaAllPosition();
        break;
    }
    case SS_CheckPosColumn1:
    case SS_CheckPosColumn2:
    case SS_CheckPosColumn3:
    case SS_CheckPosColumn4:
    case SS_CheckPosColumn5:
    case SS_CheckPosColumn6:
    case SS_CheckPosParty:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_CheckPosColumn1;
            setState_frameAnalyzeRequest();
            setAreaAllPosition();
        }
        else if (state == S_CaptureReady)
        {
            QPoint detectedPos(0,0);
            if (m_substage != SS_CheckPosParty)
            {
                int x = int(m_substage) - int(SS_CheckPosColumn1) + 1;
                for (int y = 1; y <= 5; y++)
                {
                    if (checkBrightnessMeanTarget(SmartSVEggOperation::GetBoxCaptureAreaOfPos(x,y).m_rect, C_Color_Yellow, 130))
                    {
                        detectedPos = QPoint(x,y);
                        break;
                    }
                }

                if (detectedPos.y() == 0)
                {
                    m_substage = Substage(int(m_substage) + 1);
                    setState_frameAnalyzeRequest();
                    break;
                }
            }
            else
            {
                for (int i = 1; i <= 6; i++)
                {
                    if (checkBrightnessMeanTarget(SmartSVEggOperation::GetPartyCaptureAreaOfPos(i).m_rect, C_Color_Yellow, 200))
                    {
                        detectedPos = QPoint(0,i);
                        break;
                    }
                }
            }

            if (m_fixAttempt >= 5 || detectedPos.y() == 0)
            {
                // invalid
                emit printLog(m_fixAttempt >= 5 ? "Unable to goto target position, exiting box" : "Unable to detect cursor position in box, exiting box", LOG_WARNING);
                m_substage = SS_MainMenu;
                setState_runCommand("B,5,Nothing,5");
                m_fixAttempt = 0;
            }
            else if (detectedPos == m_posToRelease)
            {
                m_fixAttempt = 0;
                QString command = "Box " + QString::number(m_currentBox) + " column " + QString::number(m_posToRelease.x()) + " row " + QString::number(m_posToRelease.y()) + ": ";
                if (checkBrightnessMeanTarget(A_Shiny.m_rect, C_Color_Shiny, 25))
                {
                    emit printLog(command + "SHINY pokemon, NOT releasing", LOG_ERROR);
                    runCommandToNextTarget();
                }
                else if (checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Yellow, 200))
                {
                    emit printLog(command + "Releasing...");
                    m_substage = SS_Release1;
                    setState_runCommand(C_Release1);
                    m_videoManager->setAreas({GetReleaseCaptureAreaOfPos(m_posToRelease.x()), A_Yes});
                }
                else
                {
                    // no pokemon
                    emit printLog(command + "No pokemon");
                    runCommandToNextTarget();
                }
            }
            else
            {
                // get command to move to target position
                m_fixAttempt++;
                qDebug() << "cursor position:" << detectedPos;
                runCommandToTargetPosition(detectedPos, m_posToRelease);
            }
        }
        break;
    }
    case SS_Release1:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(GetReleaseCaptureAreaOfPos(m_posToRelease.x()).m_rect, C_Color_Yellow, 200))
            {
                m_substage = SS_Release2;
                setState_runCommand(C_Release2);
            }
            else
            {
                // missed input, quit box and try again...?
                emit printLog("Unable to detect pop up menu, exiting Box", LOG_WARNING);
                m_substage = SS_MainMenu;
                setState_runCommand("B,5,Nothing,5");
                m_fixAttempt = 0;
            }
        }
        break;
    }
    case SS_Release2:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Yes.m_rect, C_Color_Yellow, 200))
            {
                m_substage = SS_Release3;
                setState_runCommand(C_Release3);
            }
            else
            {
                // missed input, quit box and try again...?
                emit printLog("Unable to detect yes/no menu, exiting Box", LOG_WARNING);
                m_substage = SS_MainMenu;
                setState_runCommand("B,5,Nothing,5");
                m_fixAttempt = 0;
            }
        }
        break;
    }
    case SS_Release3:
    {
        if (state == S_CommandFinished)
        {
            runCommandToNextTarget();
        }
        break;
    }
    case SS_MainMenu:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Box});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                m_fixAttempt++;
                if (m_fixAttempt >= 5)
                {
                    emit printLog("Unable to recover", LOG_ERROR);
                    m_substage = SS_Finished;
                    setState_runCommand("Home,2");
                }
                else
                {
                    emit printLog("Attempt exiting box again...", LOG_WARNING);
                    setState_runCommand("B,5,Nothing,5");
                }
            }
            else if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Yellow, 200))
            {
                // back to box again
                m_substage = SS_CheckPosColumn1;
                setState_runCommand("ASpam,10,Nothing,60");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
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

void SmartSVBoxReleaseSafe::setAreaAllPosition()
{
    QVector<CaptureArea> areas{A_Pokemon, A_Shiny};
    for (int i = 1; i <= 6; i++)
    {
        areas.push_back(SmartSVEggOperation::GetPartyCaptureAreaOfPos(i));
    }
    for (int x = 1; x <= 6; x++)
    {
        for (int y = 1; y <= 5; y++)
        {
            areas.push_back(SmartSVEggOperation::GetBoxCaptureAreaOfPos(x,y));
        }
    }
    m_videoManager->setAreas(areas);
}

const CaptureArea SmartSVBoxReleaseSafe::GetReleaseCaptureAreaOfPos(int x)
{
    if (x < 1 || x > 6)
    {
        x = 1;
    }

    x--;
    return CaptureArea(576 + x * 84, 306, 60, 30);
}

void SmartSVBoxReleaseSafe::runCommandToTargetPosition(QPoint current, QPoint target)
{
    if (target.x() <= 0 || target.y() >= 6)
    {
        qDebug() << target;
        setState_error("Invalid arguement");
        return;
    }

    QString command = "Nothing,3,Loop,1";
    int yDiff = target.y() - current.y();
    if (yDiff > 0)
    {
        command += ",LDown,3,Nothing,3,Loop," + QString::number(qAbs(yDiff));
    }
    else if (yDiff < 0)
    {
        command += ",LUp,3,Nothing,3,Loop," + QString::number(qAbs(yDiff));
    }

    int xDiff = target.x() - current.x();
    if (xDiff > 0)
    {
        command += ",LRight,3,Nothing,3,Loop," + QString::number(qAbs(xDiff));
    }
    else if (xDiff < 0)
    {
        command += ",LLeft,3,Nothing,3,Loop," + QString::number(qAbs(xDiff));
    }

    command += ",Nothing,20";
    m_substage = SS_CheckPosColumn1;
    setState_runCommand(command);
}

void SmartSVBoxReleaseSafe::runCommandToNextTarget()
{
    if (m_posToRelease == QPoint(6,5))
    {
        if (m_currentBox < m_boxTarget)
        {
            m_currentBox++;
            m_posToRelease = QPoint(1,1);
            m_substage = SS_CheckPosColumn1;
            setState_runCommand("R,3,Nothing,20");
        }
        else
        {
            m_substage = SS_Finished;
            setState_runCommand("Home,2");
        }
        return;
    }

    if (m_posToRelease.x() < 6)
    {
        m_posToRelease.rx()++;
    }
    else if (m_posToRelease.y() < 5)
    {
        m_posToRelease.rx() = 1;
        m_posToRelease.ry()++;
    }

    // immediately check for current position
    m_substage = SS_CheckPosColumn1;
    setState_frameAnalyzeRequest();
}
