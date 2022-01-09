#ifndef SMARTBDSPSTARTER_H
#define SMARTBDSPSTARTER_H

#include <QWidget>
#include <QElapsedTimer>
#include "smartprogrambase.h"

class SmartBDSPStarter : public SmartProgramBase
{
public:
    explicit SmartBDSPStarter(
            int starterIndex,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_Starter; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runRestartCommand();

    // Command indices
    Command const C_Restart = 0;
    Command const C_RestartNoUpdate = 1;
    Command const C_Talk    = 2;
    Command const C_Choose  = 3;
    Command const C_Capture = 4;
    Command const C_COUNT   = 5;

    // List of test color
    HSVRange const C_Color_Battle = HSVRange(339,130,180,359,190,220);
    HSVRange const C_Color_Dialog = HSVRange(0,0,230,359,30,255);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Battle = CaptureArea(1128,422,48,48);
    CaptureArea const A_Dialog = CaptureArea(990,620,200,48);
    CaptureArea const A_DialogFalse = CaptureArea(990,520,200,48);

    // Substages
    enum Substage : int
    {
        SS_Init,
        SS_Restart,
        SS_Intro,
        SS_Title,
        SS_GameStart,
        SS_Talk,
        SS_Select,
        SS_Choose,
        SS_Detect1, // wild starly appeared
        SS_Detect2, // Go! Starter!
        SS_Detect3, // Battle UI
        SS_Finish,
    };
    Substage m_substage;

    // Members
    int m_starterIndex;
    int m_encounter;
    bool m_dialogWasFound;
    QElapsedTimer m_elapsedTimer;
};

#endif // SMARTBDSPSTARTER_H
