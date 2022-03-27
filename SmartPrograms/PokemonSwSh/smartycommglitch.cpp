#include "smartycommglitch.h"

SmartYCommGlitch::SmartYCommGlitch(SmartProgramParameter parameter) : SmartProgramBase(parameter)
{
    init();
}

void SmartYCommGlitch::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    for (int i = 0; i < 4; i++)
    {
        A_YCommMenu.push_back(CaptureArea(394,98 + 91 * i,40,40));
    }
}

void SmartYCommGlitch::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_internetConnected = false;
}

void SmartYCommGlitch::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_videoManager->setAreas({A_Internet});

        m_substage = SS_CheckInternet;
        setState_frameAnalyzeRequest();
        break;
    }
    case SS_CheckInternet:
    {
        if (state == S_CaptureReady)
        {
            // not connected = 71.1, connected = 184.8
            m_internetConnected = checkBrightnessMeanTarget(A_Internet, C_Color_Internet, 150);
            m_substage = SS_GotoYComm;
            setState_runCommand(C_GotoYComm);

            m_videoManager->setAreas(A_YCommMenu);
        }
        break;
    }
    case SS_GotoYComm:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            // Check if "Link Trade" is highlighted
            bool atYComm = true;
            for (int i = 0; i < A_YCommMenu.size(); i++)
            {
                CaptureArea const& area = A_YCommMenu[i];
                atYComm &= checkAverageColorMatch(area.m_rect, i == 0 ? QColor(0,0,0) : QColor(253,253,253));
            }
            if (atYComm)
            {
                if (m_internetConnected)
                {
                    m_substage = SS_LinkBattle;
                    setState_runCommand(C_LinkBattle);
                    emit printLog("Internect already connected, starting battle...");

                    m_videoManager->setAreas({A_Dialog});
                }
                else
                {
                    m_substage = SS_Connect;
                    setState_runCommand(C_Connect);
                    emit printLog("Connecting to internet...");
                }
            }
            else
            {
                // wait until we are at y-comm
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Connect:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            bool connected = true;
            for (int i = 0; i < A_YCommMenu.size(); i++)
            {
                CaptureArea const& area = A_YCommMenu[i];
                connected &= checkAverageColorMatch(area.m_rect, i == 0 ? QColor(0,0,0) : QColor(253,253,253));
            }
            if (connected)
            {
                m_substage = SS_LinkBattle;
                setState_runCommand(C_LinkBattle);
                emit printLog("Starting link battle...");

                m_videoManager->setAreas({A_Dialog});
            }
            else
            {
                // Press B until "Link Trade" highlights again
                setState_runCommand(C_BPress);
            }
        }
        break;
    }
    case SS_LinkBattle:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            bool opponentFound = checkAverageColorMatch(A_Dialog, C_Color_Dialog);
            if (opponentFound)
            {
                m_substage = SS_Disconnect;
                setState_runCommand(C_Disconnect);
                emit printLog("Opponent found! Disconnecting...");

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Disconnect:
    {
        if (state == S_CommandFinished)
        {
            setState_completed();
            emit printLog("Y-comm glitch is now activated");
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

