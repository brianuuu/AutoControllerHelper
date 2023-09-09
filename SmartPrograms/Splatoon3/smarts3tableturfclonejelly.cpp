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

    setImageMatchFromResource("S3_CountTL1", m_imageMatchCount[0]);
    setImageMatchFromResource("S3_CountTL2", m_imageMatchCount[1]);
    setImageMatchFromResource("S3_CountTR1", m_imageMatchCount[2]);
    setImageMatchFromResource("S3_CountTR2", m_imageMatchCount[3]);
    setImageMatchFromResource("S3_CountBL1", m_imageMatchCount[4]);
    setImageMatchFromResource("S3_CountBL2", m_imageMatchCount[5]);
    setImageMatchFromResource("S3_CountBR1", m_imageMatchCount[6]);
    setImageMatchFromResource("S3_CountBR2", m_imageMatchCount[7]);

    m_substage = SS_Init;

    m_turn = 12;
    m_tileCount[0] = 0;
    m_tileCount[1] = 0;
    m_tileCount[2] = 0;
    m_tileCount[3] = 0;
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
        m_videoManager->setAreas({A_CardName, A_TileCount[0], A_TileCount[1], A_TileCount[2], A_TileCount[3]});
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

                    m_videoManager->setAreas({A_CardName, A_TileCount[0], A_TileCount[1], A_TileCount[2], A_TileCount[3]});
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
                    // check the tile count for each card
                    for (int i = 0; i < 4; i++)
                    {
                        if (m_tileCount[i] == 0)
                        {
                            if (checkImageMatchTarget(A_TileCount[i].m_rect, C_Color_TileCount, m_imageMatchCount[i*2], 0.85))
                            {
                                m_tileCount[i] = 1;
                            }
                            else if (checkImageMatchTarget(A_TileCount[i].m_rect, C_Color_TileCount, m_imageMatchCount[i*2+1], 0.85))
                            {
                                m_tileCount[i] = 2;
                            }
                            else
                            {
                                // assume the card has at least 3 tiles, always usable
                                m_tileCount[i] = 3;
                            }
                        }
                    }
                    printTileCounts();

                    // pick card to use (highest tile count)
                    m_cardToUse = 0;
                    int highestTileCount = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        if (m_tileCount[i] > highestTileCount)
                        {
                            m_cardToUse = i;
                            highestTileCount = m_tileCount[i];
                        }
                    }

                    if (m_tileCount[m_cardToUse] <= 2)
                    {
                        emit printLog("Unable to find card more than 2 tiles", LOG_ERROR);
                    }

                    emit printLog("Picking card " + QString::number(m_cardToUse + 1));
                    m_substage = SS_PickCard;
                    switch (m_cardToUse)
                    {
                    default:
                        setState_runCommand("A,1");
                        break;
                    case 1:
                        setState_runCommand("DRight,1,A,1");
                        break;
                    case 2:
                        setState_runCommand("DDown,1,A,1");
                        break;
                    case 3:
                        setState_runCommand("DDown,1,LRight,1,A,1");
                        break;
                    }

                    if (m_turn == 1)
                    {
                        // insanity check for last turn, should always have 3 1/2 tile cards
                        int count = 0;
                        for (int i = 0; i < 4; i++)
                        {
                            if (m_tileCount[i] <= 2)
                            {
                                count++;
                            }
                        }

                        if (count != 3)
                        {
                            emit printLog("Remaining cards does not contain 3 1/2 tile cards", LOG_ERROR);
                        }
                    }

                    m_tileCount[m_cardToUse] = 0;
                }
                else
                {
                    if (m_turn == 12)
                    {
                        m_ai.Restart();
                    }

                    m_ai.UpdateFrame(m_capture);
                    for (int i = 0; i < 4; i++)
                    {
                        m_tileCount[i] = m_ai.GetCardTileCount(i);
                    }

                    // run calculation in a thread, wait for it to finish
                    QtConcurrent::run(&m_ai, &TableTurfAI::CalculateNextMove, m_turn);
                    m_substage = SS_GetNextMove;
                }

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand(m_substage == SS_TurnWait ? "Nothing,10" : "A,1,Nothing,20");
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
    case SS_PickCard:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_PlaceCard;
            switch (m_turn)
            {
            case 12:
            case 11:
            {
                setState_runCommand("A,1,DUp,1,Loop,10,Nothing,20", true);
                break;
            }
            case 10:
            {
                m_upCount = 2;
                m_substage = SS_CountUp;
                setState_runCommand("DUp,1,LUp,1,A,1,Nothing,20");
                break;
            }
            case 9:
            case 8:
            {
                QString command;

                // move up
                for (int i = 0; i < m_upCount; i++)
                {
                    command += (i % 2 == 0) ? "DUp,1," : "LUp,1,";
                }
                command += "Loop,1,";

                // move left/right
                command += (m_turn == 9) ? "DLeft,1," : "DRight,1,";
                command += "A,1,Loop,5,Nothing,20";
                setState_runCommand(command, true);
                break;
            }
            default:
            {
                QString command = (m_turn > 4) ? "DLeft,1" : "DRight,1";
                command += ",A,1,Loop,5,DUp,1,A,1,Loop," + QString::number(m_upCount + 3);
                command += ",DDown,1,A,1,Loop," + QString::number(m_upCount + 8);
                command += (m_turn > 4) ? ",DRight,1" : ",DLeft,1";
                command += ",A,1,Loop,8,Nothing,20";
                setState_runCommand(command, true);
                break;
            }
            }

            m_timer.restart();
            m_videoManager->setAreas({A_Button});
        }

        break;
    }
    case SS_CountUp:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkTurnEnd())
            {
                emit printLog("Up count: " + QString::number(m_upCount));
            }
            else
            {
                if (m_upCount > 15)
                {
                    emit printLog("Unabled to count up, something went wrong, capture has been taken", LOG_ERROR);
                    m_substage = SS_Finished;
                    setState_runCommand("Capture,25,Home,1,Nothing,20");
                    break;
                }

                m_upCount++;
                setState_runCommand("DUp,1,A,1,Nothing,20");
            }
        }
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

                // TODO: AI pick next best move
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
                emit printLog("Unable to skip turn, something went wrong, capture has been taken", LOG_ERROR);
                m_substage = SS_Finished;
                setState_runCommand("Capture,25,Home,1,Nothing,20");
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

void SmartS3TableturfCloneJelly::printTileCounts()
{
    QString str = "Tile Counts: ";
    for (int i = 0; i < 4; i++)
    {
        if (m_tileCount[i] > 2)
        {
            str += "2+";
        }
        else
        {
            str += QString::number(m_tileCount[i]);
        }

        if (i < 3)
        {
            str += ", ";
        }
    }

    emit printLog(str);
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
            QVector<CaptureArea> areas = {A_CardName};
            for (int i = 0; i < 4; i++)
            {
                if (m_tileCount[i] == 0)
                {
                    areas.push_back(A_TileCount[i]);
                    break;
                }
            }
            m_videoManager->setAreas(areas);
        }
        else
        {
            emit printLog("Battle finished!");

            m_turn = 12;
            m_tileCount[0] = 0;
            m_tileCount[1] = 0;
            m_tileCount[2] = 0;
            m_tileCount[3] = 0;

            m_substage = SS_CheckWin;
            setState_runCommand("Nothing,20");

            m_timer.restart();
            m_videoManager->setAreas({A_CardName, A_Win, A_TileCount[0], A_TileCount[1], A_TileCount[2], A_TileCount[3]});
        }

        return true;
    }

    return false;
}

