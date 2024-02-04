#ifndef SMARTDAYSKIPPER_H
#define SMARTDAYSKIPPER_H

#include <QWidget>
#include <QtConcurrent>
#include "../smartprogrambase.h"

class SmartDaySkipper : public SmartProgramBase
{
public:
    explicit SmartDaySkipper(
            int skips,
            bool raidMode,
            QString const& pokemonList,
            QLabel* estimateLabel,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_DaySkipper; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void loadImages();
    void testImages();

    // Command indices
    Command const C_StartOK     = 0;
    Command const C_SyncTime    = 1;
    Command const C_SyncTimeWait= 2;
    Command const C_Back2000JP  = 3;
    Command const C_Back2000EU  = 4;
    Command const C_Back2000US  = 5;
    Command const C_SkipEU_JPDay= 6;
    Command const C_SkipUS      = 7;
    Command const C_BackToGame  = 8;
    Command const C_SkipJPMonth = 9;
    Command const C_SkipJPYear  = 10;
    Command const C_ToSyncTime  = 11;
    Command const C_SkipJPYearRaid  = 12;
    Command const C_SkipEUYearRaid  = 13;
    Command const C_SkipUSYearRaid  = 14;
    Command const C_RestartGame = 15;
    Command const C_StartGameA  = 16;
    Command const C_DaylightJPEU    = 17;
    Command const C_DaylightUS      = 18;
    Command const C_COUNT           = 19;

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,255,30);

    // List of test point/area
    CaptureArea const A_Invite = CaptureArea(1166,440,32,32);
    CaptureArea const A_Switch = CaptureArea(1166,554,32,32);
    CaptureArea const A_Quit = CaptureArea(1166,611,32,32);
    CapturePoint const P_Center = CapturePoint(640,0);
    CaptureArea const A_EnterGame = CaptureArea(600,0,80,80);
    CaptureArea const A_Sprite = CaptureArea(126,161,364,300);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_ToOK,
        SS_Back2000,
        SS_Skip,

        SS_StartRaid,
        SS_Invite,
        SS_CheckPokemon,
        SS_QuitRaid,
        SS_ToSyncTime,
        SS_SkipYearRaid,

        SS_RestartGame,   // Close and launch game
        SS_StartGame,   // Press A when intro starts
        SS_EnterGame,    // Wait until black screen goes away

        SS_BackToGame,
    };
    Substage m_substage;

    // Members
    QDate m_date;
    int m_skipsLeft;
    int m_skippedDays;
    bool m_raidMode;
    DateArrangement m_dateArrangement;
    QLabel* m_estimateLabel;

    QStringList m_pokemonList;
    static QMap<QString, QImage> m_imageTests; // cached only once
    bool m_isFound;
    int m_imageTestIndex;

    QDateTime m_startDateTime;

    bool is1159PM();
    bool isDaylightSaving();
    void runSyncTime();
    QString secondsToString(double time);
    QString getToJanuary();
};

#endif // SMARTDAYSKIPPER_H
