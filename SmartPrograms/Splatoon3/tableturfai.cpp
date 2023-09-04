#include "tableturfai.h"

TableTurfAI::TableTurfAI()
{
    Restart();
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
    m_boardRect = QRect();

    for (int i = 0; i < 4; i++)
    {
        m_cards[i].Reset();
    }

    m_spCount = 0;
}

void TableTurfAI::UpdateFrame(const QImage &image)
{
    m_frame = image;

    AnalysisBoard();
    AnalysisHands();
    AnalysisSpecial();

    //ExportBoard();
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

QString TableTurfAI::GetNextMove(int& o_index, int turn, bool failedLast, int preferredCard)
{
    // TODO: send this to a thread
    if (!failedLast)
    {
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
            DoPlacement_LeastMoves(preferredCard);
            break;
        }
        }
    }

    if (m_placementResults.empty())
    {
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
        // in case command fails, we can go back here to pick next best
        std::sort(m_placementResults.begin(), m_placementResults.end(),
            [](PlacementResult const& a, PlacementResult const& b)
            {
                return a.m_score > b.m_score;
            }
        );

        // TODO: enable special
        PlacementResult const& best = m_placementResults[0];
        o_index = best.m_cardIndex;
        qDebug() << "Target cursor:" << best.m_cursorPoint;
        qDebug() << "Score:" << best.m_score;

        QString command;
        if (best.m_cardIndex % 2 == 1) // 1 or 3
        {
            command += "DRight,1,";
        }
        if (best.m_cardIndex >= 2) // 2 or 3
        {
            command += "LDown,1,";
        }
        command += "A,1,Nothing,1,Loop,1,";

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

        m_placementResults.pop_front();
        command += "A,1,Nothing,20";
        return command;
    }
}

void TableTurfAI::DoPlacement_LeastMoves(int preferredCard)
{
    for (int i = 0; i < 4; i++)
    {
        if (preferredCard != -1 && i != preferredCard)
        {
            // not preferred card
            continue;
        }

        if (m_mode == Mode::NoOneTwoTile && m_cards[i].m_tileCount <= 2)
        {
            // this strat won't use any 1/2 tile cards
            continue;
        }

        TestPlacement(i, false);
    }
}

void TableTurfAI::AnalysisBoard()
{
    m_boardRect = QRect(4,22,1,1);
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        QPointF topLeft = c_boardTopLefts[y];
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            QRect rect = QRect(topLeft.toPoint(), QSize(BOARD_TILE_SIZE, BOARD_TILE_SIZE));
            if (UpdateBoardTile(rect, m_board[x][y]))
            {
                if (m_board[x][y] == GT_InkOrange)
                {
                    m_boardRect.setTop(qMin(y, m_boardRect.top()));
                    m_boardRect.setLeft(qMin(x, m_boardRect.left()));
                    m_boardRect.setBottom(qMax(y, m_boardRect.bottom()));
                    m_boardRect.setRight(qMax(x, m_boardRect.right()));
                }
            }
            else
            {
                m_board[x][y] = GT_Neutral;
                qDebug() << "ERROR DETECTING TILE(" << x << "," << y << ")";
            }

            topLeft.rx() += c_boardXSeparations[y];
        }
    }
}

bool TableTurfAI::UpdateBoardTile(QRect rect, TableTurfAI::GridType &tileType)
{
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
    m_spCount = 0;
    QPointF topLeft = c_specialTopLeft;
    for (int i = 0; i < SPECIAL_COUNT; i++)
    {
        QRect rect = QRect(topLeft.toPoint(), QSize(SPECIAL_TILE_SIZE, SPECIAL_TILE_SIZE));
        if (GetColorPixelRadio(rect, c_hsvInkOrangeSp) > c_colorPixelRatio)
        {
            m_spCount++;
        }

        topLeft.rx() += c_specialStepX;
    }
}

void TableTurfAI::TestPlacement(int cardIndex, bool isSpecial)
{
    // TODO: some flag can stop card from being used
    // TODO: rotation flag

    Card const& card = m_cards[cardIndex];
    PlacementResult result;

    int cardHalfLeft = card.m_center.x() - card.m_rect.left();
    int cardHalfRight = card.m_rect.right() - card.m_center.x();
    int cardHalfTop = card.m_center.y() - card.m_rect.top();
    int cardHalfBottom = card.m_rect.bottom() - card.m_center.y();

    int boardXMin = qMax(cardHalfLeft, m_boardRect.left() - cardHalfRight - 1);
    int boardXMax = qMin(BOARD_SIZE_X - 1 - cardHalfRight, m_boardRect.right() + cardHalfLeft + 1);
    int boardYMin = qMax(cardHalfTop, m_boardRect.top() - cardHalfBottom - 1);
    int boardYMax = qMin(BOARD_SIZE_Y - 1 - cardHalfBottom, m_boardRect.bottom() + cardHalfTop + 1);

    // start from bottom to top
    for (int y = boardYMax; y >= boardYMin; y--)
    {
        for (int x = boardXMax; x >= boardXMin; x--)
        {
            QPoint cursorPoint(x,y);
            if (TestPlacementOnPoint(card, cursorPoint, isSpecial))
            {
                result.m_cardIndex = cardIndex;
                result.m_cursorPoint = cursorPoint;

                switch (m_mode)
                {
                case Mode::LeastMoves:
                case Mode::NoOneTwoTile:
                {
                    // rank base on number of moves
                    result.m_score = 1000;
                    result.m_score -= qAbs(cursorPoint.x() - 4);
                    result.m_score -= qAbs(cursorPoint.y() - 22);
                    break;
                }
                default:
                {
                    break;
                }
                }

                m_placementResults.push_back(result);
            }
        }
    }
}

bool TableTurfAI::TestPlacementOnPoint(const TableTurfAI::Card &card, QPoint cursorPoint, bool isSpecial)
{
    bool isNextToOrangeTile = false;
    for (int cy = card.m_rect.top(); cy <= card.m_rect.bottom(); cy++)
    {
        for (int cx = card.m_rect.left(); cx <= card.m_rect.right(); cx++)
        {
            if (card.m_tile[cx][cy] == GT_Empty)
            {
                // empty tile on card
                continue;
            }

            QPoint tilePoint(cursorPoint.x() - (card.m_center.x() - cx), cursorPoint.y() - (card.m_center.y() - cy));
            GridType const& boardTileType = m_board[tilePoint.x()][tilePoint.y()];
            if (boardTileType != GT_Empty)
            {
                // overlapping
                if (!isSpecial)
                {
                    return false;
                }

                // special ignore normal tiles
                if (isSpecial && boardTileType != GT_InkOrange && boardTileType != GT_InkBlue)
                {
                    return false;
                }
            }

            // test if there is any orange tile around
            auto nearbyTest = [this, &isSpecial] (int x, int y) -> bool
            {
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

    img.save(QString(SCREENSHOT_PATH) + "Board.png", "PNG");
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

                topLeft.rx() += CARD_TILE_SIZE;
            }

            topLeft.ry() += CARD_TILE_SIZE;
            topLeft.rx() = 0;
        }

        img.save(QString(SCREENSHOT_PATH) + "Card" + QString::number(i) + ".png", "PNG");
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
    // rotation center
    QPoint tempCenter = m_center;
    if (clockwise)
    {
        m_center.setX(CARD_SIZE - tempCenter.y() - 1);
        m_center.setY(tempCenter.x());
    }
    else
    {
        m_center.setX(tempCenter.y());
        m_center.setY(CARD_SIZE - tempCenter.x() - 1);
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
    m_center = QPoint(cardXMin + ((m_rect.width() - 1)/2), cardYMin + (m_rect.height()/2));
}
