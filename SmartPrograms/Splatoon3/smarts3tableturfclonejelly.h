#ifndef SMARTS3TABLETURFCLONEJELLY_H
#define SMARTS3TABLETURFCLONEJELLY_H

#include <QElapsedTimer>
#include <QThread>
#include <QtConcurrent>
#include <QWidget>
#include "../smartprogrambase.h"
#include "tableturfai.h"

class SmartS3TableturfCloneJelly : public SmartProgramBase
{
public:
    struct Settings
    {
        TableTurfAI::Mode m_mode;
    };

    explicit SmartS3TableturfCloneJelly(
            Settings settings,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_S3_TableturfCloneJelly; }

private slots:
    void CalculateNextMoveCompleted();

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    bool checkTurnEnd();

    // Command indices
    Command const C_GameStart       = 0;
    Command const C_COUNT           = 1;

    // List of test color
    HSVRange const C_Color_CardName = HSVRange(0,0,170,359,60,210); // >180
    HSVRange const C_Color_Button = HSVRange(0,0,200,359,30,255); // >130
    HSVRange const C_Color_Win = HSVRange(50,170,230,90,255,255); // >230

    // List of test point/area
    CaptureArea const A_CardName = CaptureArea(16,75,170,32);
    CaptureArea const A_Button = CaptureArea(1003,688,41,18);
    CaptureArea const A_Win = CaptureArea(39,528,23,44);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_GameStart,
        SS_TurnWait,
        SS_GetNextMove,

        SS_PlaceCard,
        SS_PlaceCardEnd,
        SS_TurnSkip,

        SS_CheckWin,
        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    int m_turn; // turns left
    int m_upCount;
    int m_cardToUse;

    qint64 m_startTimeStamp;
    int m_winCount;
    int m_battleCount;

    TableTurfAI m_ai;

    // Stats
    Stat m_statWins;
    Stat m_statBattles;
    Stat m_statErrors;
};

#endif // SMARTS3TABLETURFCLONEJELLY_H
