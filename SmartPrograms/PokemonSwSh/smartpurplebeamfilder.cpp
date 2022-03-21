#include "smartpurplebeamfilder.h"

SmartPurpleBeamFilder::SmartPurpleBeamFilder
(
    QPoint denHolePos,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_denHoleRect(denHolePos.x(), denHolePos.y(), 140, 80)
{
    init();
}

void SmartPurpleBeamFilder::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    A_MenuIcons.clear();
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            A_MenuIcons.push_back(CaptureArea(123 + 235*x, 115 + 240*y, 95, 95));
        }
    }
}

void SmartPurpleBeamFilder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_beamFound = false;
    m_textSpeedSlow = false;
    m_triesCount = 0;
}

void SmartPurpleBeamFilder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_CollectWatt;
        setState_runCommand(C_CollectWatt);
        emit printLog("Collecting watts if not already");
        break;
    }
    case SS_CollectWatt:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_GoToMenu;
            setState_runCommand(C_GotoMenu);
            emit printLog("Setting text speed to slow");

            m_videoManager->clearCaptures();
            m_videoManager->setAreas(A_MenuIcons);
        }
        break;
    }
    case SS_GoToMenu:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            m_substage = SS_GoToOption;
            setState_runCommand(getCommandToOption());
        }
        break;
    }
    case SS_GoToOption:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_SetTextSpeed;
            setState_runCommand(m_textSpeedSlow ? C_SetTextSpeedFast : C_SetTextSpeedSlow);

            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_SetTextSpeed:
    {
        if (state == S_CommandFinished)
        {
            m_textSpeedSlow = !m_textSpeedSlow;
            if (m_textSpeedSlow)
            {
                m_substage = SS_TalkToDen;
                setState_runCommand(C_TalkToDen);
            }
            else
            {
                setState_completed();
            }
        }
        break;
    }
    case SS_TalkToDen:
    {
        if (state == S_CommandFinished)
        {
            m_triesCount++;
            m_substage = SS_BeamCheck;
            setState_frameAnalyzeRequest();

            m_videoManager->clearCaptures();
            m_videoManager->setPoints({P_Home});

            // emit command without calling set state, we need to do frame analyze immediately
            emit runSequence(m_commands[C_BeamCheck]);
            emit printLog("Throwing wishing piece (" + QString::number(m_triesCount) + ")");
        }
        break;
    }
    case SS_BeamCheck:
    {
        if (state == S_CaptureReady)
        {
            bool atHome = checkPixelColorMatch(P_Home.m_point, C_Color_Home);
            bool atHome2 = checkPixelColorMatch(P_Home.m_point, C_Color_Home2);
            if (atHome || atHome2)
            {
                if (m_beamFound)
                {
                    m_substage = SS_BeamFound;
                    setState_runCommand(C_BeamFound);
                    emit printLog("Purple beam found!");
                }
                else
                {
                    m_substage = SS_RestartGame;
                    setState_runCommand(C_RestartGame);
                    emit printLog("Not purple beam, restarting game...");

                    m_videoManager->clearCaptures();
                    m_videoManager->setPoints({P_Center});
                }
            }
            else
            {
                // Check again if it still thinks it is purple beam
                if (!m_beamFound)
                {
                    m_beamFound = checkBrightnessMeanTarget(m_denHoleRect, C_Color_Beam, 20);
                }

                // keep checking until we get to Home
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_BeamFound:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_GoToMenu;
            setState_runCommand(C_GotoMenu);
            emit printLog("Setting text speed to fast");

            m_videoManager->clearCaptures();
            m_videoManager->setAreas(A_MenuIcons);
        }
        break;
    }
    case SS_RestartGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen is not black anymore (intro plays)
            bool introStarted = !checkPixelColorMatch(P_Center.m_point, QColor(0,0,0));
            if (introStarted)
            {
                m_substage = SS_StartGame;
                setState_runCommand(C_StartGameA);
                emit printLog("Intro started, entering game...");
            }
            else
            {
                runNextStateContinue();
            }
        }
        break;
    }
    case SS_StartGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen to be black
            bool enteredBlackScreen = checkPixelColorMatch(P_Center.m_point, QColor(0,0,0));
            if (enteredBlackScreen)
            {
                m_substage = SS_EnterGame;

                m_videoManager->clearCaptures();
                m_videoManager->setAreas({A_EnterGame});
            }

            runNextStateContinue();
        }

        break;
    }
    case SS_EnterGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen to be not black anymore
            bool enteredGame = !checkAverageColorMatch(A_EnterGame.m_rect, QColor(0,0,0));
            if (enteredGame)
            {
                m_substage = SS_TalkToDen;
                setState_runCommand(C_TalkToDen);

                m_videoManager->clearCaptures();
            }
            else
            {
                runNextStateContinue();
            }
        }

        break;
    }
    }

    SmartProgramBase::runNextState();
}

QString SmartPurpleBeamFilder::getCommandToOption()
{
    QPoint cursor;
    QPoint option;

    // Find where cursor is located, and where option is
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            QRect rect = A_MenuIcons[y * 5 + x].m_rect;
            if (checkBrightnessMeanTarget(rect, C_Color_CursorOn, 20))
            {
                cursor = QPoint(x,y);
            }
            if (checkBrightnessMeanTarget(rect, C_Color_Option, 20))
            {
                option = QPoint(x,y);
            }
        }
    }

    qDebug() << "cursor:" << cursor;
    qDebug() << "option:" << option;

    if (cursor == option)
    {
        return "Nothing,1";
    }
    else
    {
        QPoint diff = option - cursor;
        QString output = diff.y() == 0 ? "Nothing,1" : "LUp,1,Nothing,1";
        for (int i = 0; i < qAbs(diff.x()); i++)
        {
            output += (diff.x() < 0) ? ",LLeft,1,Nothing,1" : ",LRight,1,Nothing,1";
        }

        return output;
    }
}
