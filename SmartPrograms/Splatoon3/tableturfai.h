#ifndef TABLETURFAI_H
#define TABLETURFAI_H

#include <QDebug>
#include <QObject>
#include <QImage>

#include "../smartprogrambase.h"

class TableTurfAI : public QObject
{
    Q_OBJECT
public:
    TableTurfAI();

    void Restart();
    void UpdateFrame(QImage const& image);

    enum class Mode
    {
        None,
        SkipTurns,
        LeastMoves,
        NoOneTwoTile,
        ThreeTwelveSp,
    };
    void SetMode(Mode mode) { m_mode = mode; }

    // TODO: flags, don't use <tile count, allow rotation

    int GetCardTileCount(int index);
    QVector<int> GetCardTileCounts();

    void CalculateNextMove(int turn);
    QString GetNextMove(bool failedLast = false);
    int GetNextCard();

signals:
    void CalculateNextMoveCompleted();
    void printLog(QString const log, QColor color = QColor(0,0,0));

private:
    enum GridType : uint8_t
    {
        GT_Empty,
        GT_InkOrange,
        GT_InkOrangeSp,
        GT_InkBlue,
        GT_InkBlueSp,
        GT_Neutral,

        GT_Unknown,
        GT_Count
    };

    #define CARD_SIZE 8
    #define CARD_TILE_SIZE 14
    struct Card
    {
        Card() { Reset(); }

        void Reset();
        void Rotate(bool clockwise = true);
        void UpdateRectCenter();

        int m_rotation;
        QPoint m_center;
        QRect m_rect; // top left and size

        int m_tileCount;
        GridType m_tile[CARD_SIZE][CARD_SIZE];
    };

    struct PlacementResult
    {
        PlacementResult()
            : m_cardIndex(-1)
            , m_rotationCount(0)
            , m_cursorPoint(4,22)
            , m_spPointOnBoard(4,22)
            , m_yMinOnBoard(26)
            , m_isSpecial(false)
            , m_score(0)
        {}

        int m_cardIndex;
        int m_rotationCount;
        QPoint m_cursorPoint;
        QPoint m_spPointOnBoard;
        int m_yMinOnBoard;
        bool m_isSpecial;

        int m_score;
    };

    #define BOARD_SIZE_X 9
    #define BOARD_SIZE_Y 26
    #define BOARD_TILE_SIZE 25
    struct BoardStat
    {
        int m_gridTypeCount[GT_Count];
        int m_enemyTurfPrev;
        int m_enemyTurf;
        int m_spCount;
        int m_spScore; // set in CalculateNextMove()
        int m_predictScore; // current score if used 3-12 card
        QVector<QPoint> m_spPoints;
        QRect m_rect;
    };

private:
    void AnalysisBoard();
    void AnalysisHands();
    void AnalysisSpecial();

    bool UpdateBoardTile(QRect rect, QPoint boardPos);
    bool UpdateCardTile(QRect rect, GridType& tileType);

    void ExportBoard();
    void ExportCards();

    void RefreshBoardPreview(GridType const ppBoardRef[][BOARD_SIZE_Y], GridType ppBoard[][BOARD_SIZE_Y]);
    void TestPlacement(int cardIndex, int turn, bool isSpecial, bool testRotation, bool isPrediction = false);
    bool TestPlacementOnPoint(Card const& card, PlacementResult& result, QPoint cursorPoint, bool isSpecial, GridType const ppBoardBefore[][BOARD_SIZE_Y], GridType ppBoardAfter[][BOARD_SIZE_Y]);

    void CalculateScore_LeastMoves(PlacementResult& result);
    void CalculateScore_ExpandTurf(PlacementResult& result);
    int CalculateScore_BuildSpecial(GridType ppBoard[][BOARD_SIZE_Y]);
    void CalculateScore_CoverEnemyTurf(PlacementResult& result, GridType const ppBoardBefore[][BOARD_SIZE_Y], GridType const ppBoardAfter[][BOARD_SIZE_Y]);

    qreal GetColorPixelRadio(QRect rect, HSVRange hsvRange);

private:
    Mode m_mode;
    QImage m_frame;

    GridType m_board[BOARD_SIZE_X][BOARD_SIZE_Y];
    GridType m_boardPreview[BOARD_SIZE_X][BOARD_SIZE_Y];
    GridType m_boardPredict[BOARD_SIZE_X][BOARD_SIZE_Y];
    BoardStat m_boardStat;
    QLabel m_previewWidget;

    QVector<PlacementResult> m_placementResults;
    Card m_cards[4];

    #define LEAST_REPLACE_TO_WIN (20+100)
    Card m_cardPredict;
    int m_predictScoreAdd;

    #define SPECIAL_COUNT 5
    #define SPECIAL_TILE_SIZE 20
    qreal const c_specialStepX = 21.5;
    QPointF const c_specialTopLeft = QPointF(37,654);

    HSVRange const c_hsvEmpty = HSVRange(0,0,0,359,255,40);
    HSVRange const c_hsvNeutral = HSVRange(0,0,160,359,50,200);
    HSVRange const c_hsvInkOrange = HSVRange(50,100,220,90,255,255);
    HSVRange const c_hsvInkOrangeSp = HSVRange(10,100,220,50,255,255);
    HSVRange const c_hsvInkBlue = HSVRange(220,100,220,260,255,255);
    HSVRange const c_hsvInkBlueSp = HSVRange(160,100,220,200,255,255);
    HSVRange const c_hsvInkBlueSpFire = HSVRange(160,0,220,200,100,255);

    QVector<QColor> const c_debugColors =
    {
        QColor(0,0,0),
        QColor(255,255,0),
        QColor(255,128,64),
        QColor(0,128,255),
        QColor(0,255,255),
        QColor(128,128,128),
        QColor(255,0,255)
    };

    qreal const c_colorPixelRatio = 120;
    QPointF const c_boardTopLefts[BOARD_SIZE_Y]
    {
        QPointF(650,8),
        QPointF(650,35),
        QPointF(650,60),
        QPointF(650,87),
        QPointF(650,113),
        QPointF(650,138),
        QPointF(650,164),
        QPointF(650,190),
        QPointF(650,217),
        QPointF(650,244),
        QPointF(650,270),
        QPointF(650,296),
        QPointF(650,323),
        QPointF(650,349),
        QPointF(650,376),
        QPointF(650,404),
        QPointF(650,430),
        QPointF(650,456),
        QPointF(650,483),
        QPointF(650,510),
        QPointF(650,538),
        QPointF(650,565),
        QPointF(650,592),
        QPointF(650,619),
        QPointF(650,647),
        QPointF(650,674)
    };
    qreal const c_boardXSeparations[BOARD_SIZE_Y] =
    {
        26.25,
        26.25,
        26.375,
        26.375,
        26.375,
        26.375,
        26.375,
        26.5,
        26.5,
        26.5,
        26.5,
        26.625,
        26.625,
        26.625,
        26.625,
        26.75,
        26.75,
        26.75,
        26.75,
        26.75,
        26.875,
        26.875,
        26.875,
        26.875,
        27,
        27
    };

    QPointF const c_handTopLefts[4] =
    {
        QPointF(24,123),
        QPointF(201,132),
        QPointF(32,341),
        QPointF(202,342)
    };
    QPointF const c_handSteps[4] =
    {
        QPointF(17.875,16.75),
        QPointF(16,15.375),
        QPointF(17,15.875),
        QPointF(15.875,15.25)
    };
    QPointF const c_handOffsets[4] =
    {
        QPointF(0.625,0.125),
        QPointF(0,0.75),
        QPointF(0,0.125),
        QPointF(0,0),
    };

    HSVRange const c_hsvCardInk = HSVRange(60,170,100,70,255,255);
    HSVRange const c_hsvCardInkSp = HSVRange(45,170,100,55,255,255);
    HSVRange const c_hsvCardDark = HSVRange(60,170,100,70,255,180);

    int const c_lastTurnExpandTurf = 9;
};

#endif // TABLETURFAI_H
