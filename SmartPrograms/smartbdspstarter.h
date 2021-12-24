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

    // Command indices
    Command const C_Restart = 0;
    Command const C_Choose  = 1;
    Command const C_Capture = 2;
    Command const C_COUNT   = 3;

    // List of test color
    HSVRange const C_Color_Battle = HSVRange(339,130,180,359,190,220);
    HSVRange const C_Color_HP = HSVRange(90,60,150,130,180,255);

    // List of test point/area
    CaptureArea const A_Battle = CaptureArea(1128,422,48,48);
    CaptureArea const A_HP = CaptureArea(1100,82,96,12);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_Select,
        SS_Choose,
        SS_Calibrate,
        SS_TestShiny,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    int m_starterIndex;
    int m_encounter;
    double m_noShinyTimer;
    QElapsedTimer m_elapsedTimer;
};

#endif // SMARTBDSPSTARTER_H