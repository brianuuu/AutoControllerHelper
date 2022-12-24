#ifndef SMARTSVGIMMIGHOULFARMER_H
#define SMARTSVGIMMIGHOULFARMER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVGimmighoulFarmer : public SmartProgramBase
{
public:
    explicit SmartSVGimmighoulFarmer(SmartProgramParameter parameter);
    virtual SmartProgram getProgramEnum() { return SP_SV_GimmighoulFarmer; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_ToTime      = 0;
    Command const C_SkipJPDay   = 1;
    Command const C_SkipEUDay   = 2;
    Command const C_SkipUSDay   = 3;
    Command const C_Restart     = 4;
    Command const C_Farm        = 5;
    Command const C_Save        = 6;
    Command const C_COUNT       = 7;

    // List of test color
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(480,110,320,100);
    CaptureArea const A_PartyFirst = CaptureArea(84,140,134,12);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_ToTime,
        SS_SkipDay,
        SS_Restart,
        SS_Title,
        SS_GameStart,
        SS_MainMenu,
        SS_Save,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;

    // Stats
};

#endif // SMARTSVGIMMIGHOULFARMER_H
