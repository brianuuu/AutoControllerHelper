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
    //ExportBoard();

    AnalysisHands();
    //ExportCards();

    TestPlacement(m_cards[2], false);
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

        m_cards[i].UpdateRect();
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

void TableTurfAI::TestPlacement(TableTurfAI::Card card, bool isSpecial)
{
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
                qDebug() << cursorPoint;
            }
        }
    }

    // TODO: return some results?
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
    if (m_center == QPoint(3,3))
    {
        m_center = clockwise ? QPoint(4,3) : QPoint(3,4);
    }
    else if (m_center == QPoint(4,3))
    {
        m_center = clockwise ? QPoint(4,4) : QPoint(3,3);
    }
    else if (m_center == QPoint(4,4))
    {
        m_center = clockwise ? QPoint(3,4) : QPoint(4,3);
    }
    else
    {
        m_center = clockwise ? QPoint(3,3) : QPoint(4,4);
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

    UpdateRect();
}

void TableTurfAI::Card::UpdateRect()
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
}
