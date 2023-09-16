#include "smarts3tableturfclonejelly.h"

SmartS3TableturfCloneJelly::SmartS3TableturfCloneJelly
(
    Settings settings,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(settings)
{
    init();
}

void SmartS3TableturfCloneJelly::CalculateNextMoveCompleted()
{
    runNextStateContinue();
}

void SmartS3TableturfCloneJelly::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartS3TableturfCloneJelly::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;

    m_turn = 12;
    m_upCount = 0;
    m_cardToUse = 0;

    m_startTimeStamp = QDateTime::currentSecsSinceEpoch();
    m_winCount = 0;
    m_battleCount = 0;

    m_ai.SetMode(m_programSettings.m_mode);
    connect(&m_ai, &TableTurfAI::CalculateNextMoveCompleted, this, &SmartS3TableturfCloneJelly::CalculateNextMoveCompleted);
    connect(&m_ai, &TableTurfAI::printLog, this, &SmartS3TableturfCloneJelly::printLogExternal);
}

void SmartS3TableturfCloneJelly::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statWins, "Wins");
        initStat(m_statBattles, "Battles");
        initStat(m_statErrors, "Errors");

        m_substage = SS_GameStart;
        setState_runCommand(C_GameStart);

        m_timer.restart();
        m_videoManager->setAreas({A_CardName});
        break;
    }
    case SS_CheckWin:
    case SS_GameStart:
    case SS_TurnWait:
    {
        if (state == S_CommandFinished)
        {
            if (m_timer.elapsed() >= 60000)
            {
                emit printLog("Unable to detect turn start for too long", LOG_ERROR);
                incrementStat(m_statErrors);

                m_substage = SS_Finished;
                setState_runCommand("Home,1,Nothing,20");
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_substage == SS_CheckWin)
            {
                if (checkBrightnessMeanTarget(A_Win.m_rect, C_Color_Win, 180))
                {
                    emit printLog("You won!", LOG_SUCCESS);

                    m_substage = SS_GameStart;
                    incrementStat(m_statWins);
                    m_winCount++;

                    m_videoManager->setAreas({A_CardName});
                }
            }

            if (checkBrightnessMeanTarget(A_CardName.m_rect, C_Color_CardName, 180))
            {
                if (m_substage != SS_TurnWait)
                {
                    if (m_battleCount > 0)
                    {
                        QString str = "Runtime: " + QString::number(QDateTime::currentSecsSinceEpoch() - m_startTimeStamp);
                        str += "s, EXP gained: " + QString::number(m_winCount * 130 + (m_battleCount- m_winCount) * 40);
                        str += " (" + QString::number(m_winCount) + "/" + QString::number(m_battleCount) + ")";
                        emit printLog(str);
                    }

                    incrementStat(m_statBattles);
                    m_battleCount++;

                    emit printLog("-----------Battle " + QString::number(m_statBattles.first) + " started!-----------");
                }

                emit printLog("Turn Left: " + QString::number(m_turn));

                if (m_programSettings.m_mode == TableTurfAI::Mode::None)
                {
                    setState_error("Unsupported");
                }
                else
                {
                    if (m_turn == 12)
                    {
                        m_ai.Restart();
                    }

                    m_ai.UpdateFrame(m_capture);

                    // run calculation in a thread, wait for it to finish
                    QtConcurrent::run(&m_ai, &TableTurfAI::CalculateNextMove, m_turn);
                    m_substage = SS_GetNextMove;
                }

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand(m_substage == SS_TurnWait ? "Nothing,1" : "A,1,Nothing,20");
            }
        }
        break;
    }
    case SS_GetNextMove:
    {
        setState_runCommand(m_ai.GetNextMove());
        m_cardToUse = m_ai.GetNextCard();
        m_substage = SS_PlaceCard;
        break;
    }
    case SS_PlaceCard:
    case SS_PlaceCardEnd:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_PlaceCardEnd;
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() < 3000)
            {
                // too fast, wait longer
                setState_frameAnalyzeRequest();
            }
            else if (!checkTurnEnd())
            {
                if (m_substage == SS_PlaceCard)
                {
                    // still during place card command
                    setState_frameAnalyzeRequest();
                    break;
                }

                emit printLog("Unable to place card, skipping turn...", LOG_WARNING);
                m_substage = SS_TurnSkip;
                switch (m_cardToUse)
                {
                default:
                    setState_runCommand("B,1,DUp,1,A,1,Nothing,4,A,1,Nothing,20");
                    break;
                case 1:
                    setState_runCommand("B,1,DLeft,1,LUp,1,A,1,Nothing,4,DRight,1,A,1,Nothing,20");
                    break;
                case 2:
                    setState_runCommand("B,1,DDown,1,A,1,Nothing,4,DDown,1,A,1,Nothing,20");
                    break;
                case 3:
                    setState_runCommand("B,1,DDown,1,LLeft,1,A,1,Nothing,4,DDown,1,LRight,1,A,1,Nothing,20");
                    break;
                }
            }
        }
        break;
    }
    case SS_TurnSkip:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkTurnEnd())
            {
                emit printLog("Unable to skip turn, something went wrong, giving up battle", LOG_ERROR);
                incrementStat(m_statErrors);

                m_turn = 12;

                m_substage = SS_CheckWin;
                setState_runCommand("BSpam,20,Plus,20,LRight,1,Nothing,1,A,1,Nothing,20");

                m_timer.restart();
                m_videoManager->setAreas({A_CardName, A_Win});
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

bool SmartS3TableturfCloneJelly::checkTurnEnd()
{
    if (!checkBrightnessMeanTarget(A_Button.m_rect, C_Color_Button, 130))
    {
        m_turn--;

        if (m_turn > 0)
        {
            m_substage = SS_TurnWait;
            setState_runCommand("Nothing,20");

            m_timer.restart();
            m_videoManager->setAreas({A_CardName});
        }
        else
        {
            emit printLog("Battle finished!");

            m_turn = 12;

            m_substage = SS_CheckWin;
            setState_runCommand("Nothing,20");

            m_timer.restart();
            m_videoManager->setAreas({A_CardName, A_Win});
        }

        return true;
    }

    return false;
}

