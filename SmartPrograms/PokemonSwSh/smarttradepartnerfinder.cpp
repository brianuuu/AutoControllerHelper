#include "smarttradepartnerfinder.h"

#include "../Dev/smartcodeentry.h"

SmartTradePartnerFinder::SmartTradePartnerFinder
(
    QString partnerName,
    QString linkCode,
    bool spamA,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_partnerName(partnerName)
    , m_linkCode(linkCode)
    , m_spamA(spamA)
{
    init();

    if (m_partnerName.isEmpty())
    {
        setState_error("Please enter partner name");
    }
	
    if (m_linkCode.size() != 8)
    {
        setState_error("Please enter 8-digit link code");
    }
}

void SmartTradePartnerFinder::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    for (int i = 0; i < 4; i++)
    {
        A_YCommMenu.push_back(CaptureArea(394,98 + 91 * i,40,40));
    }
}

void SmartTradePartnerFinder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartTradePartnerFinder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        emit printLog("Looking for partner name: " + m_partnerName);
        m_substage = SS_CheckInternet;
        setState_frameAnalyzeRequest();

        m_videoManager->setAreas({A_Internet});
        m_retry = true;
        m_timer.restart();
        break;
    }
    case SS_Return:
    case SS_CheckInternet:
    {
        if (state == S_CaptureReady)
        {
            // not connected = 71.1, connected = 184.8
            if (checkBrightnessMeanTarget(A_Internet, C_Color_Internet, 150))
            {
                if (!m_retry)
                {
                    // done
                    setState_completed();
                    break;
                }

                m_substage = SS_GotoYComm;
                setState_runCommand(C_GotoYComm);

                m_pos = QPoint(0,0);
                m_code = "";

                m_videoManager->setAreas(A_YCommMenu);
            }
            else if (m_substage == SS_Return)
            {
                if (m_timer.elapsed() > 120000)
                {
                    setState_error("Unable to return to overworld for too long");
                }
                else
                {
                    setState_frameAnalyzeRequest();
                }
            }
            else
            {
                setState_error("Internet is not connected");
            }
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
                m_substage = SS_SetLinkCode;
                setState_runCommand(C_SetLinkCode);

                m_videoManager->clearCaptures();
                emit printLog("Entering link code...");
            }
            else
            {
                // wait until we are at y-comm
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_SetLinkCode:
    {
        if (state == S_CommandFinished)
        {
            if (m_code.size() < m_linkCode.size())
            {
                // append next character
                m_code.append(m_linkCode[m_code.size()]);
                runCommandToKey();
            }
            else
            {
                m_substage = SS_FindPartner;
                setState_runCommand(C_FindPartner);

                m_timer.restart();
                m_videoManager->setAreas({A_Box, A_Name});
            }
        }
        break;
    }
    case SS_FindPartner:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 180000)
            {
                setState_error("Unable to find any trading partner for too long");
            }
            else if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Box, 240))
            {
                emit printLog("Detected in Box menu, checking trade partner's name...");
                setState_ocrRequest(A_Name.m_rect, C_Color_Black);
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_OCRReady)
        {
            PokemonDatabase::OCREntries const entry =
            {
                {"Found", {m_partnerName}}
            };

            if (matchStringDatabase(entry).isEmpty())
            {
                emit printLog("Not the partner we want, disconnecting", LOG_WARNING);
                m_substage = SS_Return;
                setState_runCommand("B,1,Loop,1,A,3,Nothing,3,Loop,0",true);

                m_videoManager->setAreas({A_Internet});
                m_timer.restart();
            }
            else
            {
                if (m_spamA)
                {
                    emit printLog("Partner matched! Spamming A until we have returned to overworld", LOG_SUCCESS);
                    m_substage = SS_Return;
                    setState_runCommand("A,3,Nothing,3,Loop,0",true);

                    m_retry = false;
                    m_videoManager->setAreas({A_Internet});
                    m_timer.restart();
                }
                else
                {
                    emit printLog("Partner matched!", LOG_SUCCESS);
                    setState_completed();
                }
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartTradePartnerFinder::runCommandToKey()
{
    QPoint targetPos;
    QString command = "";

    bool valid = SmartCodeEntry::getNumPadLocation(m_code.back(), targetPos);
    bool isNumZero = m_code.back() == '0';

    if (!valid)
    {
        setState_error("Invalid character");
    }
    else if (m_pos != targetPos)
    {
        int vSteps = targetPos.y() - m_pos.y();
        if (vSteps != 0)
        {
            // can only go up to 3 steps
            switch (vSteps)
            {
            case -1:
                command += "LUp,1";
                break;
            case -2:
                command += "LUp,1,DUp,1";
                break;
            case -3:
                command += "LUp,1,DUp,1,LUp,1";
                break;
            case 1:
                command += "LDown,1";
                break;
            case 2:
                command += "LDown,1,DDown,1";
                break;
            case 3:
                command += "LDown,1,DDown,1,LDown,1";
                break;
            }

            command += ",Loop,1";
        }

        int hSteps = targetPos.x() - m_pos.x();
        if (hSteps != 0 && !isNumZero)
        {
            if (hSteps > 6)
            {
                hSteps -= 12;
            }
            else if (hSteps < -6)
            {
                hSteps += 12;
            }

            bool goRight = hSteps > 0;
            hSteps = qAbs(hSteps);

            if (!command.isEmpty())
            {
                command += ",";
            }

            command += goRight ? "LRight,1" : "LLeft,1";
            if (hSteps > 1)
            {
                command += goRight ? ",DRight,1" : ",DLeft,1";
                command += ",Loop," + QString::number(hSteps / 2);
                if (hSteps % 2 == 1)
                {
                    // extra single command for odd number
                    command += goRight ? ",LRight,1" : ",LLeft,1";
                }
            }
        }
    }

    // finally press A, and plus if finished
    if (!command.isEmpty())
    {
        command += ",";
    }
    command += "A,1";

    m_pos = targetPos;
    setState_runCommand(command);
}
