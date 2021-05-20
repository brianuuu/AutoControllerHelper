#include "smartsurprisetrade.h"

SmartSurpriseTrade::SmartSurpriseTrade
(
    int boxToTrade,
    QPushButton* calibrateBtn,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_boxToTrade(boxToTrade)
    , m_calibrateBtn(calibrateBtn)
{
    init();
}

void SmartSurpriseTrade::tradeCompleteCalibrate()
{
    if (m_substage == SS_WaitForTrade)
    {
        C_Color_FinishTrade = getAverageColor(A_TradeDialog.m_rect);
        emit printLog("Calibrated color: (" + QString::number(C_Color_FinishTrade.red()) + "," + QString::number(C_Color_FinishTrade.green()) + "," + QString::number(C_Color_FinishTrade.blue()) + ")", QColor(0,255,0));
    }
    else
    {
        emit printLog("Please wait until trade started before calibrating", LOG_ERROR);
    }
}

void SmartSurpriseTrade::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    for (int i = 0; i < 4; i++)
    {
        A_YCommMenu.push_back(CaptureArea(394,98 + 91 * i,40,40));
    }

    connect(m_calibrateBtn, &QPushButton::clicked, this, &SmartSurpriseTrade::tradeCompleteCalibrate);
}

void SmartSurpriseTrade::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_viewStatsChecked = false;
    m_internetConnected = false;
    m_currentBox = 1;
    m_pos = QPoint(1,1);

    m_calibrateBtn->setEnabled(false);
}

void SmartSurpriseTrade::stop()
{
    SmartProgramBase::stop();

    m_calibrateBtn->setEnabled(false);
}

void SmartSurpriseTrade::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_CheckInternet;
        setState_frameAnalyzeRequest();

        m_parameters.vlcWrapper->clearCaptures();
        m_parameters.vlcWrapper->setAreas({A_YComm});
        break;
    }
    case SS_CheckInternet:
    {
        if (state == S_CaptureReady)
        {
            // not connected = 71.1, connected = 184.8
            m_internetConnected = checkBrightnessMeanTarget(A_YComm.m_rect, C_Color_Internet, 150);
            m_substage = SS_GotoYComm;
            setState_runCommand(C_GotoYComm);

            m_parameters.vlcWrapper->clearCaptures();
            m_parameters.vlcWrapper->setAreas(A_YCommMenu);
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
                    m_substage = SS_GotoPokemon;
                    setState_runCommand(getGotoPokemon());

                    m_parameters.vlcWrapper->clearCaptures();
                    m_parameters.vlcWrapper->setAreas({A_PokemonName, A_DynamaxLevel});
                }
                else
                {
                    m_substage = SS_Connect;
                    setState_runCommand(C_PlusPress);
                    emit printLog("Connecting to internet...");
                }
            }
            else
            {
                // We might be going into trade evolution, spam B, then back to spam A
                m_substage = SS_FinishTrade;
                m_pos.rx()--;
                setState_runCommand("BSpam,80");
                emit printLog("Not at Y-comm (trade evolution?), attemping recover...");

                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas({A_YComm});
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
                m_substage = SS_GotoPokemon;
                setState_runCommand(getGotoPokemon());

                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas({A_PokemonName, A_DynamaxLevel});
            }
            else
            {
                // Press B until "Link Trade" highlights again
                setState_runCommand(C_BPress);
            }
        }
        break;
    }
    case SS_GotoPokemon:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            QString location = "Box " + QString::number(m_currentBox) + " row " + QString::number(m_pos.y()) + " column " + QString::number(m_pos.x());

            bool viewStats = checkAverageColorMatch(A_PokemonName.m_rect, C_Color_Pokemon); // check if slot has a name
            if (!m_viewStatsChecked && !viewStats)
            {
                // We should always be able to view state with one press, unless it's an empty slot
                m_viewStatsChecked = true;
                setState_runCommand(C_PlusPress);
            }
            else if (m_viewStatsChecked && !viewStats)
            {
                // This is an empty slot, move to next
                m_viewStatsChecked = false;
                emit printLog(location + ": No Pokemon");
                if (isLastPokemon())
                {
                    setState_completed();
                }
                else
                {
                    setState_runCommand(getNextPokemon());
                }
            }
            else //if (viewStats)
            {
                m_viewStatsChecked = true;
                bool notEgg = checkAverageColorMatch(A_DynamaxLevel.m_rect, C_Color_Pokemon); // check if Dynamax Level is there
                if (notEgg)
                {
                    m_substage = SS_ConfirmTrade;
                    setState_runCommand(C_ConfirmTrade);
                    emit printLog(location + ": Trading...");

                    m_parameters.vlcWrapper->clearCaptures();
                    m_parameters.vlcWrapper->setAreas({A_TradeDialogBIG});
                }
                else
                {
                    setState_runCommand(getNextPokemon());
                    emit printLog(location + ": Cannot trade Egg");
                }
            }
        }
        break;
    }
    case SS_ConfirmTrade:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            /*bool validTrade = checkAverageColorMatch(A_TradeDialog.m_rect, C_Color_Searching);
            if (!validTrade)
            {
                if (isLastPokemon())
                {
                    setState_completed();
                }
                else
                {
                    // Set position to next, can exceed max row, but it will make it goto next box
                    if (m_pos.x() < 6)
                    {
                        m_pos.rx()++;
                    }
                    else
                    {
                        m_pos.setX(1);
                        m_pos.ry()++;
                    }

                    // Did we just try to trade a pokemon in Battle Box?
                    m_substage = SS_CheckInternet;
                    setState_frameAnalyzeRequest();
                    emit printLog("Pokemon could not be traded, Battle Box?");

                    m_parameters.vlcWrapper->clearCaptures();
                    m_parameters.vlcWrapper->setAreas({A_YComm});
                }
            }
            else*/
            {
                m_substage = SS_WaitForTrade;
                setState_frameAnalyzeRequest();

                m_calibrateBtn->setEnabled(true);
            }
        }
        break;
    }
    case SS_WaitForTrade:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            bool tradeDone = checkAverageColorMatch(A_TradeDialog.m_rect, C_Color_FinishTrade);
            if (tradeDone)
            {
                m_substage = SS_FinishTrade;
                setState_runCommand(C_FinishTrade);
                emit printLog("Trade complete!");

                m_parameters.vlcWrapper->clearCaptures();
                m_parameters.vlcWrapper->setAreas({A_YComm});
                m_calibrateBtn->setEnabled(false);
            }
            else
            {
                // Check every second or so
                setState_runCommand(C_BPress);
            }
        }
        break;
    }
    case SS_FinishTrade:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            // Check if y-comm logo appears again
            // Trade evolution may be falsely detected to blue background!
            bool animFinished = checkBrightnessMeanTarget(A_YComm.m_rect, C_Color_Internet, 40);
            if (animFinished)
            {
                if (isLastPokemon())
                {
                    setState_completed();
                }
                else
                {
                    // Set position to next, can exceed max row, but it will make it goto next box
                    if (m_pos.x() < 6)
                    {
                        m_pos.rx()++;
                    }
                    else
                    {
                        m_pos.setX(1);
                        m_pos.ry()++;
                    }

                    // Trade is fully complete
                    m_substage = SS_CheckInternet;
                    setState_frameAnalyzeRequest();
                }
            }
            else
            {
                // Spam A until we are at overworld again (this include disconnect, new dex entry)
                setState_runCommand(C_ASpam);
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

QString SmartSurpriseTrade::getGotoPokemon()
{
    // Goto surprise trade and go to the next pokemon to trade
    QString command = "LDown,1,Nothing,1,A,1,Nothing,40,Y,1";

    if (m_pos.y() > 5)
    {
        m_pos = QPoint(1,1);
        m_currentBox++;
        command += ",R,1";
    }
    else
    {
        for (int x = 1; x < m_pos.x(); x++)
        {
            command += ",Nothing,1,LRight,1";
        }
        for (int y = 1; y < m_pos.y(); y++)
        {
            command += ",Nothing,1,LDown,1";
        }
    }

    command += ",Nothing,20";
    return command;
}

QString SmartSurpriseTrade::getNextPokemon()
{
    // Get command to move to the next pokemon
    if (m_pos == QPoint(6,5))
    {
        m_pos = QPoint(1,1);
        m_currentBox++;
        return "LDown,1,Nothing,1,LDown,1,Nothing,1,LDown,1,Nothing,1,LRight,1,Nothing,1,LRight,1,Nothing,1,R,1,Nothing,20";
    }
    else if (m_pos.x() < 6)
    {
        m_pos.rx()++;
        return "LRight,1,Nothing,20";
    }
    else
    {
        m_pos.setX(1);
        m_pos.ry()++;
        return "LDown,1,Nothing,1,LRight,1,Nothing,1,LRight,1,Nothing,20";
    }
}
