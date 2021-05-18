#ifndef SMARTYCOMMGLITCH_H
#define SMARTYCOMMGLITCH_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartYCommGlitch : public SmartProgramBase
{
public:
    explicit SmartYCommGlitch(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_YCommGlitch; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_GotoYComm   = 0;
    Command const C_Connect     = 1;
    Command const C_BPress      = 2;
    Command const C_LinkBattle  = 3;
    Command const C_Disconnect  = 4;
    Command const C_COUNT       = 5;

    // List of test color
    HSVRange const C_Color_Internet = HSVRange(220,70,0, 250,255,255);
    QColor const C_Color_Dialog = QColor(50,50,50);

    // List of test point/area
    QVector<CaptureArea> A_YCommMenu;
    QRect const A_Internet = QRect(7,664,48,52);
    QRect const A_Dialog = QRect(805,646,80,32);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_CheckInternet,
        SS_GotoYComm,
        SS_Connect,
        SS_LinkBattle,
        SS_Disconnect,
    };
    Substage m_substage;

    // Members
    bool m_internetConnected;
};

#endif // SMARTYCOMMGLITCH_H
