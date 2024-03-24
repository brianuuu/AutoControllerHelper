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
        int m_bossIndex;
        int m_bossDownPress;
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

    // Command indices

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,120); // >180
    HSVRange const C_Color_TextB = HSVRange(0,0,0,359,30,150); // for black text OCR
    HSVRange const C_Color_TextW = HSVRange(0,0,150,359,30,255); // for white text OCR

    // List of test point/area
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
    CaptureArea const A_RentalName[3] =
    {
        CaptureArea(620,204,220,32), // black
        CaptureArea(620,390,220,32), // white
        CaptureArea(620,576,220,32)  // white
    };
    CaptureArea const A_RentalAbility[3] =
    {
        CaptureArea(620,238,220,32), // black
        CaptureArea(620,424,220,32), // white
        CaptureArea(620,610,220,32)  // white
    };
    CaptureArea const A_RentalMove[3] =
    {
        CaptureArea(906,111,250,32),
        CaptureArea(906,297,250,32),
        CaptureArea(906,483,250,32)
    };

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Start,
        SS_BossSelect,
        SS_StartDA,

        SS_RentalSelect,
        SS_FindPath,
    };
    Substage m_substage;

    struct RentalData
    {
        QString m_name;
        QString m_ability;
        int m_stats[ST_COUNT];
        QVector<int> m_moves;
        QVector<int> m_maxMoves;
    };
    typedef QPair<QString, RentalData> IDRentalPair;
    static QVector<IDRentalPair> m_bossData;
    static QVector<IDRentalPair> m_rentalData;
    static QMap<QString, QVector<double>> m_matchupData;
    static PokemonDatabase::OCREntries m_allBossEntries;
    static PokemonDatabase::OCREntries m_allRentalEntries;

    struct MoveData
    {
        int m_pp;
        int m_power;
        double m_accuracy;
        double m_factor;
        bool m_isSpecial;
    };
    static QMap<int, MoveData> m_moveData;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    int m_ocrIndex;

    struct RentalSearch
    {
        QString m_name;
        QString m_ability;
        int m_firstMove;
    };
    QVector<RentalSearch> m_rentalSearch;

    // Stats
    Stat m_statError;
};

#endif // SMARTMAXLAIR_H
