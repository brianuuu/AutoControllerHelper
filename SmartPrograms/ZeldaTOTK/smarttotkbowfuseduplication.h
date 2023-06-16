#ifndef SMARTTOTKBOWFUSEDUPLICATION_H
#define SMARTTOTKBOWFUSEDUPLICATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartTOTKBowFuseDuplication : public SmartProgramBase
{
public:
    explicit SmartTOTKBowFuseDuplication(
            int loopCount,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_TOTK_BowFuseDuplication; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Attach      = 0;
    Command const C_DropFirst   = 1;
    Command const C_DropSecond  = 2;
    Command const C_COUNT       = 3;

    // List of test color
    HSVRange const C_Color_Menu = HSVRange(25,0,200,105,50,255); // >230

    // List of test point/area
    CaptureArea const A_Menu = CaptureArea(380,682,300,30);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_CheckColor,
        SS_Start,

        SS_Attach,
        SS_DropFirst,
        SS_QuickMenu,

        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;

    int m_loopLeft;
    bool m_menuDetected;
};

#endif // SMARTTOTKBOWFUSEDUPLICATION_H
