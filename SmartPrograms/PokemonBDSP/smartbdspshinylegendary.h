#ifndef SMARTBDSPSHINYLEGENDARY_H
#define SMARTBDSPSHINYLEGENDARY_H

#include <QWidget>
#include <QElapsedTimer>
#include "../smartprogrambase.h"

class SmartBDSPShinyLegendary : public SmartProgramBase
{
public:
    enum LegendaryType : uint8_t
    {
        LT_DialgaPalkia = 0,
        LT_Regigigas,
        LT_Others,
        LT_COUNT,
    };

public:
    explicit SmartBDSPShinyLegendary(
            LegendaryType type,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_ShinyLegendary; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runRestartCommand();

    // Command indices
    Command const C_Restart             = 0;
    Command const C_RestartNoUpdate     = 1;
    Command const C_Talk                = 2;
    Command const C_TalkDialgaPalkia    = 3;
    Command const C_TalkRegigigas       = 4;
    Command const C_Capture             = 5;
    Command const C_COUNT               = 6;

    // List of test color
    HSVRange const C_Color_Dialog = HSVRange(0,0,230,359,30,255);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Dialog = CaptureArea(990,620,200,48);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_Intro,
        SS_Title,
        SS_GameStart,
        SS_Talk,
        SS_Detect1,
        SS_Detect2,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    LegendaryType m_type;
    double m_noShinyTimer;
    bool m_dialogWasFound;
    QElapsedTimer m_elapsedTimer;

    // Stats
    Stat m_encounter;
    Stat m_error;
};

#endif // SMARTBDSPSHINYLEGENDARY_H
