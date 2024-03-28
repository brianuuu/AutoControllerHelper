#ifndef SMARTMAXLAIR_H
#define SMARTMAXLAIR_H

#include <QComboBox>
#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartMaxLair : public SmartProgramBase
{
public:
    struct Settings
    {
        int m_legendIndex;
        int m_legendDownPress;
        BallType m_legendBall;
        BallType m_bossBall;
    };

public:
    explicit SmartMaxLair(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_MaxLair; }

public:
    static void populateMaxLairBoss(QComboBox* cb);
    static bool populateMaxLairMatchupData();
    static bool populateMaxLairRentalBossData();
    static bool populateMaxLairMoveData();

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void resetBattleParams(bool isBeginning);
    void calculateBestMove();

    // Command indices

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,120); // >180
    HSVRange const C_Color_TextB = HSVRange(0,0,0,359,30,150); // for black text OCR
    HSVRange const C_Color_TextW = HSVRange(0,0,150,359,30,255); // for white text OCR
    HSVRange const C_Color_RStick = HSVRange(0,0,180,359,30,255);
    HSVRange const C_Color_Fight = HSVRange(300,100,100,355,255,255); // >80
    HSVRange const C_Color_Cheer = HSVRange(160,100,100,220,255,255); // >60
    HSVRange const C_Color_Dynamax = HSVRange(0,0,220,359,100,255);
    HSVRange const C_Color_Caught = HSVRange(280,200,130,340,255,190); // >230
    HSVRange const C_Color_Shiny = HSVRange(300,100,150,359,255,255); // >30

    // List of test point/area
    CaptureArea const A_SelectionBase = CaptureArea(886,549,148,12);
    CaptureArea const A_Selection[4] =
    {
        CaptureArea(943,390,84,34),
        CaptureArea(943,430,84,34),
        CaptureArea(943,470,84,34),
        CaptureArea(943,510,84,34)
    };
    CaptureArea const A_RaidStart[3] =
    {
        CaptureArea(1166,497,32,32),
        CaptureArea(1166,554,32,32),
        CaptureArea(1166,611,32,32)
    };
    CaptureArea const A_RentalSelect[3] =
    {
        CaptureArea(818,240,32,32), // black
        CaptureArea(818,426,32,32), // white
        CaptureArea(818,612,32,32)  // white
    };
    CaptureArea const A_RentalName[4] =
    {
        CaptureArea(620,204,220,32), // black
        CaptureArea(620,390,220,32), // white
        CaptureArea(620,576,220,32), // white
        CaptureArea(620,432,220,32)  // swap
    };
    CaptureArea const A_RentalAbility[4] =
    {
        CaptureArea(620,238,220,32), // black
        CaptureArea(620,424,220,32), // white
        CaptureArea(620,610,220,32), // white
        CaptureArea(620,466,220,32)  // swap
    };
    CaptureArea const A_RentalMove[4] =
    {
        CaptureArea(906,111,250,32),
        CaptureArea(906,297,250,32),
        CaptureArea(906,483,250,32),
        CaptureArea(906,337,250,32)
    };
    CaptureArea const A_RStick = CaptureArea(1157,638,40,40);
    CaptureArea const A_Fight = CaptureArea(1188,488,63,63);
    CapturePoint const P_Pokemon = CapturePoint(1050,597);
    CapturePoint const P_Run = CapturePoint(1050,674);
    CapturePoint const A_Trainers[4] =
    {
        CapturePoint(318,460),
        CapturePoint(619,460),
        CapturePoint(920,460),
        CapturePoint(1221,460)
    };
    CaptureArea const A_Boss = CaptureArea(265,70,360,54);
    CaptureArea const A_BossTypes[2] =
    {
        CaptureArea(304,126,96,28),
        CaptureArea(448,126,96,28)
    };
    CaptureArea const A_Dynamax = CaptureArea(734,566,52,39);
    CaptureArea const A_SwapButtons[2] =
    {
        CaptureArea(1108,554,32,32),
        CaptureArea(1108,612,32,32)
    };
    CapturePoint const P_Catch[2] =
    {
        CapturePoint(1257,626,QColor(0,255,255)),
        CapturePoint(1257,682,QColor(0,255,255))
    };
    CaptureArea const A_Ball = CaptureArea(876,446,272,46);
    CaptureArea const A_Caught[2] =
    {
        CaptureArea(1202,46,72,72), // black
        CaptureArea(1182,452,72,72) // pink
    };
    CaptureArea const A_Shiny = CaptureArea(103,386,26,26);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Start,
        SS_BossSelect,
        SS_StartDA,

        SS_RentalSelect,
        SS_FindPath,
        SS_Battle,
        SS_Fight,
        SS_Target,
        SS_CheckBoss,

        SS_Catch,
        SS_RentalSwap,
        SS_Result,
        SS_CheckShiny,
        SS_TakeReward,
    };
    Substage m_substage;

    struct RentalData
    {
        QString m_name;
        QString m_ability;
        int m_stats[ST_COUNT];
        QVector<int> m_moves;
        QVector<int> m_maxMoves;
        MoveType m_types[2];
    };
    typedef QPair<QString, RentalData> IDRentalPair;
    static QVector<IDRentalPair> m_bossData;
    static QVector<IDRentalPair> m_rentalData;
    static QMap<QString, QVector<double>> m_matchupData;
    static PokemonDatabase::OCREntries m_allBossEntries;
    static PokemonDatabase::OCREntries m_allRentalEntries;

    struct MoveData
    {
        QString m_name;
        int m_pp;
        int m_power;
        MoveType m_type;
        double m_accuracy;
        double m_factor;
        bool m_isSpecial;
    };
    static QMap<int, MoveData> m_moveData;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    int m_ocrIndex;
    QImage m_imageMatch_RStick;
    QImage m_imageMatch_Dynamax;
    int m_bufferDetection;

    struct RentalSearch
    {
        QString m_name;
        QString m_ability;
        int m_firstMove;
        MoveType m_types[2];
    };
    QVector<RentalSearch> m_rentalSearch;
    QVector<int> m_rentalPPData;
    int m_rentalIndex;
    double m_rentalScore;

    typedef QPair<int, double> MoveIDScore;
    QVector<MoveIDScore> m_moveScoreList;
    bool m_ballFound[BT_COUNT];

    RentalSearch m_bossSearch;
    int m_bossIndex;
    bool m_bossChecked;

    // battle
    int m_battleCount;
    int m_turnCount;
    int m_dynamaxCount;
    int m_cursorPos;

    // Stats
    Stat m_statError;
    Stat m_statRuns;
    Stat m_statCaught;
    Stat m_statWins;
    Stat m_statShiny;
};

#endif // SMARTMAXLAIR_H
