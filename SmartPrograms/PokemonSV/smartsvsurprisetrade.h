#ifndef SMARTSVSURPRISETRADE_H
#define SMARTSVSURPRISETRADE_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVSurpriseTrade : public SmartProgramBase
{
public:
    explicit SmartSVSurpriseTrade(
            int tradeTarget,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_SurpriseTrade; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices

    // List of test color
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200
    HSVRange const C_Color_Green = HSVRange(70,70,150,110,255,255); // >120
    HSVRange const C_Color_Black = HSVRange(0,0,20,359,255,60); // >220

    // List of test point/area
    CaptureArea const A_Box = CaptureArea(444,80,70,38);
    CaptureArea const A_Pokemon = CaptureArea(1072,8,70,38);
    CaptureArea const A_SurpriseTrade = CaptureArea(306,392,70,38);
    CaptureArea const A_Tick = CaptureArea(82,395,30,30);
    CaptureArea const A_Yes = CaptureArea(1080,406,70,38);
    CaptureArea const A_Dialog = CaptureArea(870,600,70,38);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_AtBox,
        SS_ToPokemon,
        SS_ToPokePortal,
        SS_WaitTrade,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    int m_tradeTarget;
    int m_tradeCount;
    QPoint m_pos;

    // Stats
    Stat m_statTrade;
    Stat m_statError;
};

#endif // SMARTSVSURPRISETRADE_H
