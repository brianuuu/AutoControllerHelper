#include "smartsvsurprisetrade.h"

SmartSVSurpriseTrade::SmartSVSurpriseTrade
(
    int boxCount,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_boxCount(boxCount)
{
    init();
}

void SmartSVSurpriseTrade::init()
{
    SmartProgramBase::init();
}

void SmartSVSurpriseTrade::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_box = 1;
    m_pos = QPoint(0,1); // x=0 is invalid, but we add it at SS_Init
}

void SmartSVSurpriseTrade::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_tradeCount, "Trades");
        initStat(m_error, "Errors");

        m_timer.restart();
        m_substage = SS_AtBox;
        m_videoManager->setAreas({A_Box, A_Pokemon});
        setState_runCommand("Nothing,5");
        break;
    }
    case SS_AtBox:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Box.m_rect, C_Color_Yellow, 200)
             && checkBrightnessMeanTarget(A_Pokemon.m_rect, C_Color_Yellow, 200))
            {
                m_substage = SS_ToPokemon;
                if (m_pos.x() < 6)
                {
                    m_pos.rx()++;
                }
                else if (m_pos.y() < 5)
                {
                    m_pos.rx() = 1;
                    m_pos.ry() ++;
                }
                else
                {
                    if (m_box < m_boxCount)
                    {
                        m_pos = QPoint(1,1);
                        m_box++;
                        setState_runCommand("R,5,Nothing,5");
                        emit printLog("Selecting box " + QString::number(m_box) + " pokemon at pos 1,1");
                        m_videoManager->clearCaptures();
                        break;
                    }
                    else
                    {
                        m_substage = SS_Finish;
                        setState_runCommand("Home,4,Nothing,10");
                        break;
                    }
                }

                QString command = "Nothing,2,Loop,1";
                if (m_pos.x() > 1)
                {
                    command += ",DRight,2,Nothing,2,Loop," + QString::number(m_pos.x() - 1);
                }
                if (m_pos.y() > 1)
                {
                    command += ",DDown,2,Nothing,2,Loop," + QString::number(m_pos.y() - 1);
                }
                setState_runCommand(command);
                emit printLog("Selecting pokemon at pos " + QString::number(m_pos.x()) + "," + QString::number(m_pos.y()));
                m_videoManager->clearCaptures();
            }
            else if (m_timer.elapsed() > 120000)
            {
                incrementStat(m_error);
                setState_error("Unable to detect being in Box menu for too long");
            }
            else
            {
                setState_runCommand("A,5,Nothing,80");
            }
        }
        break;
    }
    case SS_ToPokemon:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            m_substage = SS_ToPokePortal;
            setState_runCommand("ASpam,200,X,5,Nothing,20,A,5,Nothing,50");
            m_videoManager->setAreas({A_SurpriseTrade, A_Tick});
        }
        break;
    }
    case SS_ToPokePortal:
    case SS_WaitTrade:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_substage == SS_ToPokePortal)
            {
                if (m_timer.elapsed() > 30000)
                {
                    incrementStat(m_error);
                    setState_error("Unable to detect PokePortal for too long");
                    break;
                }
                else if (checkBrightnessMeanTarget(A_SurpriseTrade.m_rect, C_Color_Yellow, 200))
                {
                    emit printLog("At PokePortal, waiting for trade to complete...");
                    m_timer.restart();
                    m_substage = SS_WaitTrade;
                }
                setState_runCommand("Nothing,20");
            }
            else // if (m_substage == SS_WaitTrade)
            {
                if (m_timer.elapsed() > 120000)
                {
                    incrementStat(m_error);
                    setState_error("Unable to detect trade complete for too long");
                    break;
                }
                else if (checkBrightnessMeanTarget(A_Tick.m_rect, C_Color_Green, 120))
                {
                    incrementStat(m_tradeCount);
                    emit printLog("Trade completed!");
                    m_timer.restart();
                    m_substage = SS_AtBox;
                    setState_runCommand("ASpam,200");
                    m_videoManager->setAreas({A_Box, A_Pokemon});
                    break;
                }
                setState_runCommand("Nothing,20");
            }
        }
        break;
    }
    case SS_Finish:
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
