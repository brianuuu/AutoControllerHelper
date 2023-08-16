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
    m_placeCardTries = 0;
    m_cardToUse = 0;
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
                setState_error("Unable to detect turn start for too long");
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
                    m_substage = SS_GameStart;
                    incrementStat(m_statWins);
                    emit printLog("You won!", LOG_SUCCESS);

                    m_videoManager->setAreas({A_CardName, A_TileCount[0], A_TileCount[1], A_TileCount[2], A_TileCount[3]});
                }
            }

            if (checkBrightnessMeanTarget(A_CardName.m_rect, C_Color_CardName, 180))
            {
                if (m_substage != SS_TurnWait)
                {
                    incrementStat(m_statBattles);
                    emit printLog("Battle " + QString::number(m_statBattles.first) + " started!");
                }

                emit printLog("Turn Left: " + QString::number(m_turn));

                // check the tile count for each card
                for (int i = 0; i < 4; i++)
                {
                    if (m_tileCount[i] == 0)
                    {
                        if (checkImageMatchTarget(A_TileCount[i].m_rect, C_Color_TileCount, m_imageMatchCount[i*2], 0.9))
                        {
                            emit printLog("Card " + QString::number(i) + ": 1 tile");
                            m_tileCount[i] = 1;
                        }
                        else if (checkImageMatchTarget(A_TileCount[i].m_rect, C_Color_TileCount, m_imageMatchCount[i*2+1], 0.9))
                        {
                            emit printLog("Card " + QString::number(i) + ": 2 tiles");
                            m_tileCount[i] = 2;
                        }
                        else
                        {
                            // assume the card has at least 3 tiles, always usable
                            emit printLog("Card " + QString::number(i) + ": >2 tiles");
                            m_tileCount[i] = 3;
                        }
                    }
                }

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

                emit printLog("Picking card " + QString::number(m_cardToUse));
                m_substage = SS_PickCard;
                switch (m_cardToUse)
                {
                default:
                    setState_runCommand("A,1,Nothing,20");
                    break;
                case 1:
                    setState_runCommand("DRight,1,A,1,Nothing,20");
                    break;
                case 2:
                    setState_runCommand("DDown,1,A,1,Nothing,20");
                    break;
                case 3:
                    setState_runCommand("DDown,1,LRight,1,A,1,Nothing,20");
                    break;
                }

                m_tileCount[cardToUse] = 0;
                m_placeCardTries = 0;
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand(m_substage == SS_TurnWait ? "Nothing,20" : "A,1,Nothing,20");
            }
        }
        break;
    }
    case SS_PickCard:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Button.m_rect, C_Color_Button, 130))
            {
                m_substage = SS_PlaceCard;
                setState_runCommand(C_PlaceCard1, true);

                m_videoManager->setAreas({A_Button});
            }
            else
            {
                setState_error("Unable to detect rotate buttons");
            }
        }
        break;
    }
    case SS_PlaceCard:
    case SS_PlaceCardCheck:
    {
        if (state == S_CaptureReady)
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
            }
            else
            {
                if (m_substage == SS_PlaceCardCheck)
                {
                    m_substage = SS_PlaceCard;
                    setState_runCommand(C_PlaceCard2);
                }
                else
                {
                    setState_frameAnalyzeRequest();
                }
            }
        }
        else if (state == S_CommandFinished)
        {
            m_placeCardTries++;
            if (m_placeCardTries >= 8)
            {
                emit printLog("Unable to place card, skipping turn...", LOG_WARNING);
                m_substage = SS_TurnSkip;
                switch (m_cardToUse)
                {
                default:
                    setState_runCommand("B,1,DUp,1,A,1,Nothing,2,A,1,Nothing,20");
                    break;
                case 1:
                    setState_runCommand("B,1,DLeft,1,LUp,1,A,1,Nothing,2,DRight,1,A,1,Nothing,20");
                    break;
                case 2:
                    setState_runCommand("B,1,DDown,1,A,1,Nothing,2,DDown,1,A,1,Nothing,20");
                    break;
                case 3:
                    setState_runCommand("B,1,DDown,1,LLeft,1,A,1,Nothing,2,DDown,1,LRight,1,A,1,Nothing,20");
                    break;
                }
            }
            else
            {
                m_substage = SS_PlaceCardCheck;
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_TurnSkip:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_PlaceCard;
            setState_frameAnalyzeRequest();
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

