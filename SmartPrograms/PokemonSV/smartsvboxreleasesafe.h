#ifndef SMARTSVBOXRELEASESAFE_H
#define SMARTSVBOXRELEASESAFE_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVBoxReleaseSafe : public SmartProgramBase
{
public:
    explicit SmartSVBoxReleaseSafe(
            int boxTarget,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_BoxReleaseSafe; }

    static CaptureArea const GetReleaseCaptureAreaOfPos(int x, bool hasItem);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void setAreaAllPosition();
    void runCommandToTargetPosition(QPoint current, QPoint target);
    void runCommandToNextTarget();

    // Command indices
    Command const C_Release1    = 0;
    Command const C_Release2    = 1;
    Command const C_Release3    = 2;
    Command const C_COUNT       = 3;

    // List of test color
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200 (box: >130)
    HSVRange const C_Color_Shiny = HSVRange(0,0,200,359,255,255); // >25

    // List of test point/area
    CaptureArea const A_Pokemon = CaptureArea(836,4,21,46);
    CaptureArea const A_Yes = CaptureArea(1098,430,60,30);
    CaptureArea const A_Box = CaptureArea(1110,226,100,30);
    CaptureArea const A_Shiny = CaptureArea(1126,60,30,30);

    // Substages
    enum Substage : uint8_t
    {
        SS_Init = 0,

        SS_CheckPosColumn1,
        SS_CheckPosColumn2,
        SS_CheckPosColumn3,
        SS_CheckPosColumn4,
        SS_CheckPosColumn5,
        SS_CheckPosColumn6,
        SS_CheckPosParty,

        SS_Release1,
        SS_Release2,
        SS_Release3,

        SS_MainMenu,

        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    int m_boxTarget;
    int m_currentBox;
    QPoint m_posToRelease;
    int m_fixAttempt;

    // Stats
};

#endif // SMARTSVBOXRELEASESAFE_H
