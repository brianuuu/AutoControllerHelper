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
    ExportBoard();

    AnalysisHands();
    ExportCards();
}

void TableTurfAI::AnalysisBoard()
{
    for (int y = 0; y < BOARD_SIZE_Y; y++)
    {
        QPointF topLeft = c_boardTopLefts[y];
        for (int x = 0; x < BOARD_SIZE_X; x++)
        {
            QRect rect = QRect(topLeft.toPoint(), QSize(BOARD_TILE_SIZE, BOARD_TILE_SIZE));
            if (!UpdateBoardTile(rect, m_board[x][y]))
            {
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
            // unsable card won't become usable
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
}
