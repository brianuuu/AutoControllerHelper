#ifndef SMARTS3TABLETURFCLONEJELLY_H
#define SMARTS3TABLETURFCLONEJELLY_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartS3TableturfCloneJelly : public SmartProgramBase
{
public:
    struct Settings
    {
    };

    explicit SmartS3TableturfCloneJelly(
            Settings settings,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_S3_TableturfCloneJelly; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void printTileCounts();
    bool checkTurnEnd();

    // Command indices
    Command const C_GameStart       = 0;
    Command const C_COUNT           = 1;

    // List of test color
    HSVRange const C_Color_CardName = HSVRange(0,0,170,359,60,210); // >180
    HSVRange const C_Color_TileCount = HSVRange(0,0,200,295,255,255);
    HSVRange const C_Color_Button = HSVRange(0,0,200,359,30,255); // >130
    HSVRange const C_Color_Win = HSVRange(50,170,230,90,255,255); // >230

    // List of test point/area
    CaptureArea const A_CardName = CaptureArea(16,75,170,32);
    CaptureArea const A_TileCount[4] =
    {
        CaptureArea(29,269,49,45), // TL
        CaptureArea(199,267,45,43), // TR
        CaptureArea(30,479,49,44), // BL
        CaptureArea(199,476,45,43) // BR
    };
    CaptureArea const A_Button = CaptureArea(1003,688,41,18);
    CaptureArea const A_Win = CaptureArea(39,528,23,44);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_GameStart,
        SS_TurnWait,
        SS_PickCard,

        SS_PlaceCard,
        SS_PlaceCardEnd,

        SS_CountUp,
        SS_TurnSkip,

        SS_CheckWin,
        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    QImage m_imageMatchCount[8];
    int m_turn; // turns left
    int m_tileCount[4]; // tile count for each cards
    int m_upCount;
    int m_cardToUse;

    // Stats
    Stat m_statWins;
    Stat m_statBattles;
};

#endif // SMARTS3TABLETURFCLONEJELLY_H
