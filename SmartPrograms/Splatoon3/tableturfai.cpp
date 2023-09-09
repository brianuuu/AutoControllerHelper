#include "tableturfai.h"

TableTurfAI::TableTurfAI()
{
    Restart();
    m_previewWidget.resize(BOARD_SIZE_X * BOARD_TILE_SIZE + 1, BOARD_SIZE_Y * BOARD_TILE_SIZE + 1);
    m_previewWidget.setWindowTitle("Board Preview");
    m_previewWidget.show();
}

void TableTurfAI::Restart()
{
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            m_board[x][y] = GT_Empty;
        }
    }
    m_boardStat.m_rect = QRect();
    m_boardStat.m_spCount = 0;
    m_boardStat.m_spPoints.clear();
    m_boardStat.m_enemyTurfPrev = 0;
    m_boardStat.m_enemyTurf = 0;

    for (int i = 0; i < 4; i++)
    {
        m_cards[i].Reset();
    }
}

void TableTurfAI::UpdateFrame(const QImage &image)
{
    m_frame = image;
    m_boardStat.m_enemyTurfPrev = m_boardStat.m_enemyTurf;

    AnalysisBoard();
    AnalysisHands();
    AnalysisSpecial();

    m_boardStat.m_enemyTurf = m_boardStat.m_gridTypeCount[GT_InkBlue] + m_boardStat.m_gridTypeCount[GT_InkBlueSp];
    int orangeTurf = m_boardStat.m_gridTypeCount[GT_InkOrange] + m_boardStat.m_gridTypeCount[GT_InkOrangeSp];
    emit printLog("Board Status: " + QString::number(orangeTurf) + "vs" + QString::number(m_boardStat.m_enemyTurf) + ", Specials: " + QString::number(m_boardStat.m_spCount));

    ExportBoard();
    //ExportCards();
}

int TableTurfAI::GetCardTileCount(int index)
{
    return m_cards[index].m_tileCount;
}

QVector<int> TableTurfAI::GetCardTileCounts()
{
    return { m_cards[0].m_tileCount, m_cards[1].m_tileCount, m_cards[2].m_tileCount, m_cards[3].m_tileCount };
}

void TableTurfAI::CalculateNextMove(int turn)
{
    // calculate current special score
    m_boardStat.m_spScore = CalculateScore_BuildSpecial(m_board);

    // get next results
    m_placementResults.clear();
    switch (m_mode)
    {
    case Mode::None:
    case Mode::SkipTurns:
    {
        // nothing to do
        break;
    }
    case Mode::LeastMoves:
    case Mode::NoOneTwoTile:
    {
        for (int i = 0; i < 4; i++)
        {
            if (m_mode == Mode::NoOneTwoTile && m_cards[i].m_tileCount <= 2)
            {
                // this strat won't use any 1/2 tile cards
                continue;
            }

            // test all cards without rotation
            TestPlacement(i, turn, false, false);
        }
        break;
    }
    case Mode::ThreeTwelveSp:
    {
        // last turn, use special on 3-12 tile card
        if (turn == 1)
        {
            if (m_boardStat.m_spCount >= 3)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (m_cards[i].m_tileCount != 12)
                    {
                        // must be 12 tile card
                        continue;
                    }

                    TestPlacement(i, turn, true, true);
                }

                if (m_placementResults.empty())
                {
                    emit printLog("Unable to place 3-12 tile card with special...", LOG_WARNING);
                }
                else
                {
                    break;
                }
            }
            else
            {
                emit printLog("Couldn't build enough special...", LOG_WARNING);
            }
        }
        /* Disabled, this will waste turn covering enemy 3-12 tile card instead of expanding turf
        else if (m_boardStat.m_spCount >= 5)
        {
            // have enough special, use it
            for (int i = 0; i < 4; i++)
            {
                if (m_cards[i].m_tileCount == 12)
                {
                    // only use 3-12 tile card last turn
                    continue;
                }

                TestPlacement(i, turn, true, true);
            }

            if (!m_placementResults.empty())
            {
                break;
            }
        }*/

        // sort card by tile count
        QVector<QPair<int, int>> tileCountToIndices;
        for (int i = 0; i < 4; i++)
        {
            tileCountToIndices.push_back( QPair<int, int>(m_cards[i].m_tileCount, i) );
        }
        std::sort(tileCountToIndices.begin(), tileCountToIndices.end(),
            [] (QPair<int, int> const& a, QPair<int, int> const& b)
            {
                return a.first > b.first;
            }
        );

        // use highest tile count card first
        for (auto const& pair : tileCountToIndices)
        {
            if (turn > 1 && pair.first > 4)
            {
                // don't use any cards above 4 tiles unless last turn
                continue;
            }

            if (m_boardStat.m_spCount >= 3)
            {
                // enough special, just use least move
                TestPlacement(pair.second, turn, false, false);
                if (!m_placementResults.empty())
                {
                    // already have solution for higher tile card, quit
                    break;
                }
            }
            else
            {
                // test all cards with rotation
                TestPlacement(pair.second, turn, false, true);
            }
        }
        break;
    }
    }

    // score results
    std::sort(m_placementResults.begin(), m_placementResults.end(),
        [](PlacementResult const& a, PlacementResult const& b)
        {
            return a.m_score > b.m_score;
        }
    );

    emit CalculateNextMoveCompleted();
}

QString TableTurfAI::GetNextMove(bool failedLast)
{
    // next best move
    if (failedLast && !m_placementResults.empty())
    {
        // TODO: cursor is not the origin
        m_placementResults.pop_front();
    }

    if (m_placementResults.empty())
    {
        emit printLog("No card to use, skipping turn");
        // no card to use, skip turn
        int index = 0;
        if (m_mode != Mode::SkipTurns)
        {
            // don't want to place any card, skip card with least tiles
            int lowestTileCount = INT_MAX;
            for (int i = 0; i < 4; i++)
            {
                if (m_cards[i].m_tileCount < lowestTileCount)
                {
                    index = i;
                    lowestTileCount = m_cards[i].m_tileCount;
                }
            }
        }

        switch (index)
        {
        default:
            return "DUp,1,A,1,Nothing,2,A,1,Nothing,20";
        case 1:
            return "DUp,1,A,1,Nothing,2,DRight,1,A,1,Nothing,20";
        case 2:
            return "DUp,1,A,1,Nothing,2,DDown,1,A,1,Nothing,20";
        case 3:
            return "DUp,1,A,1,Nothing,2,DDown,1,LRight,1,A,1,Nothing,20";
        }
    }
    else
    {
        PlacementResult const& best = m_placementResults[0];
        emit printLog("Using card " + QString::number(best.m_cardIndex)
                      + " with rotation " + QString::number(best.m_rotationCount)
                      + " at (" + QString::number(best.m_cursorPoint.x()) + "," + QString::number(best.m_cursorPoint.y())
                      + "), Score = " + QString::number(best.m_score)
                      + ", Special: " + (best.m_isSpecial ? "Yes" : "No"));

        // hard record special points on board to avoid false detection
        if (!m_boardStat.m_spPoints.contains(best.m_spPointOnBoard))
        {
            m_boardStat.m_spPoints.push_back(best.m_spPointOnBoard);
        }

        // force special tile to be special, if replaced by enemy it will be updated in AnalysisBoard()
        m_board[best.m_spPointOnBoard.x()][best.m_spPointOnBoard.y()] = GT_InkOrangeSp;

        QString command;

        // enable special
        if (best.m_isSpecial)
        {
            command += "DUp,1,LRight,1,A,1,Nothing,4,";
        }

        // move to card
        if (best.m_cardIndex % 2 == 1) // 1 or 3
        {
            command += "DRight,1,";
        }
        if (best.m_cardIndex >= 2) // 2 or 3
        {
            command += "LDown,1,";
        }
        command += "A,1,Nothing,1,";

        // rotation
        if (best.m_rotationCount == 1)
        {
            command += "X,1,";
        }
        else if (best.m_rotationCount == 2)
        {
            command += "X,1,Nothing,1,X,1,";
        }
        else if (best.m_rotationCount == 3)
        {
            command += "Y,1,";
        }

        command += "Loop,1,";

        // TODO: default cursor pos change if failed last time
        int xCount = 4 - best.m_cursorPoint.x();
        if (xCount != 0)
        {
            command += (xCount > 0 ? "DLeft" : "DRight");
            command += ",1,Nothing,1,Loop," + QString::number(qAbs(xCount)) + ",";
        }

        int yCount = 22 - best.m_cursorPoint.y();
        if (yCount != 0)
        {
            command += (yCount > 0 ? "DUp" : "DDown");
            command += ",1,Nothing,1,Loop," + QString::number(qAbs(yCount)) + ",";
        }

        command += "A,1,Nothing,20";
        m_placementResults.pop_front();
        return command;
    }
}

int TableTurfAI::GetNextCard()
{
    if (!m_placementResults.empty())
    {
        return m_placementResults[0].m_cardIndex;
    }

    return -1;
}

void TableTurfAI::AnalysisBoard()
{
    m_boardStat.m_rect = QRect(4,22,1,1);
    for (int i = 0; i < GT_Count; i++)
    {
        m_boardStat.m_gridTypeCount[i] = 0;
    }

    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        QPointF topLeft = c_boardTopLefts[y];
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            QRect rect = QRect(topLeft.toPoint(), QSize(BOARD_TILE_SIZE, BOARD_TILE_SIZE));
            if (UpdateBoardTile(rect, QPoint(x,y)))
            {
                m_boardStat.m_gridTypeCount[ m_board[x][y] ]++;
                if (m_board[x][y] == GT_InkOrange || m_board[x][y] == GT_InkOrangeSp)
                {
                    m_boardStat.m_rect.setTop(qMin(y, m_boardStat.m_rect.top()));
                    m_boardStat.m_rect.setLeft(qMin(x, m_boardStat.m_rect.left()));
                    m_boardStat.m_rect.setBottom(qMax(y, m_boardStat.m_rect.bottom()));
                    m_boardStat.m_rect.setRight(qMax(x, m_boardStat.m_rect.right()));
                }
            }
            else
            {
                m_board[x][y] = GT_Neutral;
                emit printLog("ERROR DETECTING TILE(" + QString::number(x) + "," + QString::number(y) + ")", LOG_WARNING);
            }

            topLeft.rx() += c_boardXSeparations[y];
        }
    }
}

bool TableTurfAI::UpdateBoardTile(QRect rect, QPoint boardPos)
{
    GridType& tileType = m_board[boardPos.x()][boardPos.y()];
    if (GetColorPixelRadio(rect, c_hsvEmpty) > c_colorPixelRatio)
    {
        if (tileType != GT_Empty)
        {
            // cannot become empty
            return false;
        }

        tileType = GT_Empty;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvNeutral) > c_colorPixelRatio)
    {
        tileType = GT_Neutral;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvInkBlue) > c_colorPixelRatio)
    {
        tileType = GT_InkBlue;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvInkBlueSpFire) > c_colorPixelRatio || GetColorPixelRadio(rect, c_hsvInkBlueSp) > c_colorPixelRatio)
    {
        tileType = GT_InkBlueSp;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvInkOrangeSp) > c_colorPixelRatio)
    {
        // hard record special points on board to avoid false detection
        if (!m_boardStat.m_spPoints.contains(boardPos))
        {
            m_boardStat.m_spPoints.push_back(boardPos);
        }

        tileType = GT_InkOrangeSp;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvInkOrange) > c_colorPixelRatio)
    {
        if (tileType == GT_InkOrangeSp)
        {
            // becoming fiery
            return true;
        }

        tileType = GT_InkOrange;
        return true;
    }

    // if it was blue ink tile or our special, assume it is the same
    if (tileType == GT_InkBlue || tileType == GT_InkBlueSp || tileType == GT_InkOrangeSp)
    {
        return true;
    }

    return false;
}

void TableTurfAI::AnalysisHands()
{
    for (int i = 0; i < 4; i++)
    {
        if (m_cards[i].m_init)
        {
            // already know what this card is
            continue;
        }

        if (!m_cards[i].m_usable)
        {
            // unusable card won't become usable
        }

        m_cards[i].m_tileCount = 0;
        QPointF topLeft = c_handTopLefts[i];
        QPointF offset = QPointF(0,0);
        for (int y = 0; y < CARD_SIZE; y++)
        {
            for (int x = 0; x < CARD_SIZE; x++)
            {
                QRect rect = QRect((topLeft + offset).toPoint(), QSize(CARD_TILE_SIZE, CARD_TILE_SIZE));
                if (UpdateCardTile(rect, m_cards[i].m_tile[x][y]))
                {
                    m_cards[i].m_tileCount++;
                }

                if (m_cards[i].m_usable && m_cards[i].m_tile[x][y] == GT_InkOrange)
                {
                    // check usability
                    if (GetColorPixelRadio(rect, c_hsvCardDark) > c_colorPixelRatio)
                    {
                        m_cards[i].m_usable = false;
                    }
                }

                // next column
                topLeft.rx() += c_handSteps[i].x();
                offset.ry() += c_handOffsets[i].y();
            }

            // return to far left
            topLeft.rx() = c_handTopLefts[i].x();
            offset.ry() = 0;

            // next row
            topLeft.ry() += c_handSteps[i].y();
            offset.rx() += c_handOffsets[i].x();
        }

        if (m_cards[i].m_tileCount == 0)
        {
            qDebug() << "ERROR DETECTING ANY TILE FOR CARD" << i;
            m_cards[i].m_usable = false;
        }

        m_cards[i].UpdateRectCenter();
    }
}

bool TableTurfAI::UpdateCardTile(QRect rect, TableTurfAI::GridType &tileType)
{
    if (GetColorPixelRadio(rect, c_hsvCardInk) > c_colorPixelRatio)
    {
        tileType = GT_InkOrange;
        return true;
    }

    if (GetColorPixelRadio(rect, c_hsvCardInkSp) > c_colorPixelRatio)
    {
        tileType = GT_InkOrangeSp;
        return true;
    }

    tileType = GT_Empty;
    return false;
}

void TableTurfAI::AnalysisSpecial()
{
    m_boardStat.m_spCount = 0;
    QPointF topLeft = c_specialTopLeft;
    for (int i = 0; i < SPECIAL_COUNT; i++)
    {
        QRect rect = QRect(topLeft.toPoint(), QSize(SPECIAL_TILE_SIZE, SPECIAL_TILE_SIZE));
        if (GetColorPixelRadio(rect, c_hsvInkOrangeSp) > c_colorPixelRatio)
        {
            m_boardStat.m_spCount++;
        }

        topLeft.rx() += c_specialStepX;
    }
}

void TableTurfAI::TestPlacement(int cardIndex, int turn, bool isSpecial, bool testRotation)
{
    Card card = m_cards[cardIndex];

    for (int r = 0; r < (testRotation ? 4 : 1); r++)
    {
        int cardHalfLeft = card.m_center.x() - card.m_rect.left();
        int cardHalfRight = card.m_rect.right() - card.m_center.x();
        int cardHalfTop = card.m_center.y() - card.m_rect.top();
        int cardHalfBottom = card.m_rect.bottom() - card.m_center.y();

        int boardXMin = qMax(cardHalfLeft, m_boardStat.m_rect.left() - cardHalfRight - 1);
        int boardXMax = qMin(BOARD_SIZE_X - 1 - cardHalfRight, m_boardStat.m_rect.right() + cardHalfLeft + 1);
        int boardYMin = qMax(cardHalfTop, m_boardStat.m_rect.top() - cardHalfBottom - 1);
        int boardYMax = qMin(BOARD_SIZE_Y - 1 - cardHalfBottom, m_boardStat.m_rect.bottom() + cardHalfTop + 1);

        // start from bottom to top
        for (int y = boardYMax; y >= boardYMin; y--)
        {
            for (int x = boardXMax; x >= boardXMin; x--)
            {
                RefreshBoardPreview();

                PlacementResult result;
                QPoint cursorPoint(x,y);
                if (TestPlacementOnPoint(card, result, cursorPoint, isSpecial))
                {
                    result.m_cardIndex = cardIndex;
                    result.m_cursorPoint = cursorPoint;
                    result.m_rotationCount = r;
                    result.m_isSpecial = isSpecial;

                    if (isSpecial)
                    {
                        CalculateScore_CoverEnemyTurf(result);
                    }
                    else
                    {
                        switch (m_mode)
                        {
                        case Mode::LeastMoves:
                        case Mode::NoOneTwoTile:
                        {
                            CalculateScore_LeastMoves(result);
                            break;
                        }
                        case Mode::ThreeTwelveSp:
                        {
                            if (turn >= c_lastTurnExpandTurf)
                            {
                                CalculateScore_ExpandTurf(result);
                            }
                            else if (m_boardStat.m_spCount >= 3)
                            {
                                // have enough special, just do least moves
                                CalculateScore_LeastMoves(result);
                            }
                            else
                            {
                                // build special is increase in spScore
                                result.m_score = CalculateScore_BuildSpecial(m_boardPreview) - m_boardStat.m_spScore;
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }

                    m_placementResults.push_back(result);
                }
            }
        }

        if (testRotation && r < 3)
        {
            // rotate for next check
            card.Rotate(true);
        }
    }
}

bool TableTurfAI::TestPlacementOnPoint(const TableTurfAI::Card &card, PlacementResult &result, QPoint cursorPoint, bool isSpecial)
{
    bool isNextToOrangeTile = false;
    for (int cy = card.m_rect.top(); cy <= card.m_rect.bottom(); cy++)
    {
        for (int cx = card.m_rect.left(); cx <= card.m_rect.right(); cx++)
        {
            GridType const& cardTileType = card.m_tile[cx][cy];
            if (cardTileType == GT_Empty)
            {
                // empty tile on card
                continue;
            }

            QPoint tilePoint(cursorPoint.x() - (card.m_center.x() - cx), cursorPoint.y() - (card.m_center.y() - cy));
            GridType const& boardTileType = m_board[tilePoint.x()][tilePoint.y()];
            if (boardTileType != GT_Empty)
            {
                if (!isSpecial)
                {
                    // overlapping
                    return false;
                }
                else if ((boardTileType != GT_InkOrange && boardTileType != GT_InkBlue) || m_boardStat.m_spPoints.contains(tilePoint))
                {
                    // special ignore normal tiles
                    return false;
                }
            }

            // record where the special point is
            if (cardTileType == GT_InkOrangeSp)
            {
                result.m_spPointOnBoard = tilePoint;
                if (boardTileType == GT_InkBlue)
                {
                    // special point should not cover enemy ink (to avoid covering enemy 12 tile card)
                    return false;
                }
            }

            // record the point closest to enemy turf
            if (tilePoint.y() < result.m_yMinOnBoard)
            {
                result.m_yMinOnBoard = tilePoint.y();
            }

            // put card on preview
            m_boardPreview[tilePoint.x()][tilePoint.y()] = cardTileType;

            // test if there is any orange tile around
            auto nearbyTest = [this, &isSpecial] (int x, int y) -> bool
            {
                if (x < 0 || y < 0 || x >= BOARD_SIZE_X || y >= BOARD_SIZE_Y)
                {
                    return false;
                }

                bool result = m_board[x][y] == GT_InkOrangeSp;
                if (!isSpecial)
                {
                    result |= m_board[x][y] == GT_InkOrange;
                }
                return result;
            };

            isNextToOrangeTile |=
                    nearbyTest(tilePoint.x() - 1,   tilePoint.y() - 1)  ||
                    nearbyTest(tilePoint.x(),       tilePoint.y() - 1)  ||
                    nearbyTest(tilePoint.x() + 1,   tilePoint.y() - 1)  ||
                    nearbyTest(tilePoint.x() - 1,   tilePoint.y())      ||
                    nearbyTest(tilePoint.x() + 1,   tilePoint.y())      ||
                    nearbyTest(tilePoint.x() - 1,   tilePoint.y() + 1)  ||
                    nearbyTest(tilePoint.x(),       tilePoint.y() + 1)  ||
                    nearbyTest(tilePoint.x() + 1,   tilePoint.y() + 1);
        }
    }

    return isNextToOrangeTile;
}

void TableTurfAI::CalculateScore_LeastMoves(PlacementResult &result)
{
    // rank base on number of moves
    result.m_score += 10;
    result.m_score -= qAbs(result.m_cursorPoint.x() - 4);
    result.m_score -= qAbs(result.m_cursorPoint.y() - 22);
    result.m_score -= (result.m_rotationCount % 2 == 0) ? result.m_rotationCount : 1;
}

void TableTurfAI::CalculateScore_ExpandTurf(TableTurfAI::PlacementResult &result)
{
    // rank base on the special tile closest to enemy turf
    result.m_score += 30;
    result.m_score -= result.m_spPointOnBoard.y();

    // panelty on x-movement
    result.m_score -= qAbs(result.m_cursorPoint.x() - 4);
}

int TableTurfAI::CalculateScore_BuildSpecial(TableTurfAI::GridType ppBoard[][BOARD_SIZE_Y])
{
    QVector<QPoint> spPoints;
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            if (ppBoard[x][y] == GT_InkOrangeSp)
            {
                spPoints.push_back(QPoint(x,y));
            }
        }
    }

    auto nearbyTest = [&ppBoard] (int x, int y, int& surroundCount)
    {
        // border counts as surrounded
        if (x < 0 || y < 0 || x >= BOARD_SIZE_X || y >= BOARD_SIZE_Y || ppBoard[x][y] != GT_Empty)
        {
            surroundCount++;
        }
    };

    int score = 0;
    for (QPoint const& spPoint : spPoints)
    {
        int surroundCount = 0;
        nearbyTest(spPoint.x() - 1,   spPoint.y() - 1,  surroundCount);
        nearbyTest(spPoint.x(),       spPoint.y() - 1,  surroundCount);
        nearbyTest(spPoint.x() + 1,   spPoint.y() - 1,  surroundCount);
        nearbyTest(spPoint.x() - 1,   spPoint.y(),      surroundCount);
        nearbyTest(spPoint.x() + 1,   spPoint.y(),      surroundCount);
        nearbyTest(spPoint.x() - 1,   spPoint.y() + 1,  surroundCount);
        nearbyTest(spPoint.x(),       spPoint.y() + 1,  surroundCount);
        nearbyTest(spPoint.x() + 1,   spPoint.y() + 1,  surroundCount);

        // each surrounding point adds incremental score, 1st add 1, 2nd add 2 etc., max score = 36
        for (int i = 0; i < surroundCount; i++)
        {
            score += i;
        }
    }

    return score;
}

void TableTurfAI::CalculateScore_CoverEnemyTurf(TableTurfAI::PlacementResult &result)
{
    int enemyTurf = 0;
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            if (m_boardPreview[x][y] == GT_InkBlue || m_boardPreview[x][y] == GT_InkBlueSp)
            {
                enemyTurf++;
            }
        }
    }

    // score on replacing the most enemy turf
    result.m_score += m_boardStat.m_enemyTurf - enemyTurf;
}

void TableTurfAI::ExportBoard()
{
    QImage img(BOARD_SIZE_X * BOARD_TILE_SIZE + 1, BOARD_SIZE_Y * BOARD_TILE_SIZE + 1, QImage::Format_RGB32);
    img.fill(0);

    QPainter painter(&img);
    painter.setPen(c_debugColors[GT_Neutral]); // border drawing

    QPoint topLeft(0,0);
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            GridType type = m_board[x][y];

            QRect rect(topLeft.x(), topLeft.y(), BOARD_TILE_SIZE, BOARD_TILE_SIZE);

            painter.fillRect(rect, c_debugColors[type]);
            painter.drawRect(rect);

            topLeft.rx() += BOARD_TILE_SIZE;
        }

        topLeft.ry() += BOARD_TILE_SIZE;
        topLeft.rx() = 0;
    }

    m_previewWidget.setPixmap(QPixmap::fromImage(img));
    //img.save(QString(SCREENSHOT_PATH) + "Board.png", "PNG");
}

void TableTurfAI::ExportCards()
{
    QImage img(CARD_SIZE * CARD_TILE_SIZE + 1, CARD_SIZE * CARD_TILE_SIZE + 1, QImage::Format_RGB32);
    QPainter painter(&img);
    painter.setPen(c_debugColors[GT_Neutral]); // border drawing

    for (int i = 0; i < 4; i++)
    {
        img.fill(0);

        QPoint topLeft(0,0);
        for (int y = 0; y < CARD_SIZE; y++)
        {
            for (int x = 0; x < CARD_SIZE; x++)
            {
                GridType type = m_cards[i].m_tile[x][y];

                QRect rect(topLeft.x(), topLeft.y(), CARD_TILE_SIZE, CARD_TILE_SIZE);

                painter.fillRect(rect, c_debugColors[type]);
                painter.drawRect(rect);

                if (m_cards[i].m_center == QPoint(x,y))
                {
                    painter.fillRect(QRect(topLeft.x() + 5, topLeft.y() + 5, CARD_TILE_SIZE - 9, CARD_TILE_SIZE - 9), QColor(255,0,0));
                }

                topLeft.rx() += CARD_TILE_SIZE;
            }

            topLeft.ry() += CARD_TILE_SIZE;
            topLeft.rx() = 0;
        }

        img.save(QString(SCREENSHOT_PATH) + "Card" + QString::number(i) + ".png", "PNG");
    }
}

void TableTurfAI::RefreshBoardPreview()
{
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            m_boardPreview[x][y] = m_board[x][y];
        }
    }
}

qreal TableTurfAI::GetColorPixelRadio(QRect rect, HSVRange hsvRange)
{
    QImage cropped = m_frame.copy(rect);
    QImage masked = QImage(cropped.size(), QImage::Format_MonoLSB);
    masked.setColorTable({0xFF000000,0xFFFFFFFF});

    qreal mean = 0;
    for (int y = 0; y < cropped.height(); y++)
    {
        QRgb *rowData = (QRgb*)cropped.scanLine(y);
        uint8_t *rowMaskedData = (uint8_t*)masked.scanLine(y);
        for (int x = 0; x < cropped.width(); x++)
        {
            // Mask the target color
            bool matched = SmartProgramBase::checkColorMatchHSV(QColor::fromRgb(rowData[x]), hsvRange);
            if (matched)
            {
                mean += 255;
            }
        }
    }

    // Get average value of brightness
    mean /= (cropped.height() * cropped.width());
    return mean;
}

void TableTurfAI::Card::Reset()
{
    m_init = false;
    m_usable = true;
    m_rotation = 0;
    m_center = QPoint(3,3);
    m_rect = QRect();
    m_tileCount = 0;
    for (int y = 0; y < CARD_SIZE; y++)
    {
        for (int x = 0; x < CARD_SIZE; x++)
        {
            m_tile[x][y] = GT_Empty;
        }
    }
}

void TableTurfAI::Card::Rotate(bool clockwise)
{
    if (clockwise)
    {
        m_rotation++;
        if (m_rotation == 4)
        {
            m_rotation = 0;
        }
    }
    else
    {
        m_rotation--;
        if (m_rotation == -1)
        {
            m_rotation = 3;
        }
    }

    // copy to temp
    GridType temp[CARD_SIZE][CARD_SIZE];
    for (int y = 0; y < CARD_SIZE; y++)
    {
        for (int x = 0; x < CARD_SIZE; x++)
        {
            temp[x][y] = m_tile[x][y];
        }
    }

    // rotate
    for (int y = 0; y < CARD_SIZE; y++)
    {
        for (int x = 0; x < CARD_SIZE; x++)
        {
            if (clockwise)
            {
                m_tile[x][y] = temp[y][CARD_SIZE - x - 1];
            }
            else
            {
                m_tile[y][CARD_SIZE - x - 1] = temp[x][y];
            }
        }
    }

    UpdateRectCenter();
}

void TableTurfAI::Card::UpdateRectCenter()
{
    if (m_tileCount == 0)
    {
        return;
    }

    int cardXMin = CARD_SIZE;
    int cardXMax = -1;
    int cardYMin = CARD_SIZE;
    int cardYMax = -1;

    for (int y = 0; y < CARD_SIZE; y++)
    {
        for (int x = 0; x < CARD_SIZE; x++)
        {
            if (m_tile[x][y] != GT_Empty)
            {
                cardXMin = qMin(x, cardXMin);
                cardXMax = qMax(x, cardXMax);
                cardYMin = qMin(y, cardYMin);
                cardYMax = qMax(y, cardYMax);
            }
        }
    }

    m_rect = QRect(QPoint(cardXMin,cardYMin), QPoint(cardXMax,cardYMax));

    if (m_rect.width() % 2 == 0 && m_rect.height() % 2 == 0)
    {
        m_center = QPoint(cardXMin + ((m_rect.width() - 1)/2), cardYMin + (m_rect.height()/2));
    }
    else
    {
        switch (m_rotation)
        {
        case 0:
            m_center = QPoint(cardXMin + ((m_rect.width() - 1)/2), cardYMin + (m_rect.height()/2));
            break;
        case 1:
            m_center = QPoint(cardXMin + ((m_rect.width() - 1)/2), cardYMin + ((m_rect.height() - 1)/2));
            break;
        case 2:
            m_center = QPoint(cardXMin + (m_rect.width()/2), cardYMin + ((m_rect.height() - 1)/2));
            break;
        case 3:
            m_center = QPoint(cardXMin + (m_rect.width()/2), cardYMin + (m_rect.height()/2));
            break;
        }
    }
}
