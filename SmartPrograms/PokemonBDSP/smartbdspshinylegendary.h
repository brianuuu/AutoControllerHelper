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
        LT_Shaymin,
        LT_Arceus,
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
    Command const C_RestartShaymin      = 2;
    Command const C_Talk                = 3;
    Command const C_TalkDialgaPalkia    = 4;
    Command const C_TalkRegigigas       = 5;
    Command const C_TalkShaymin         = 6;
    Command const C_TalkArceus          = 7;
    Command const C_Capture             = 8;
    Command const C_COUNT               = 9;

    // List of test color
    HSVRange const C_Color_Dialog = HSVRange(0,0,230,359,30,255);
    HSVRange const C_Color_Battle = HSVRange(339,130,180,3,190,220);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Dialog = CaptureArea(890,620,300,48);
    CaptureArea const A_Battle = CaptureArea(1128,422,48,48);

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
        SS_DetectBattle,
        SS_RestartShaymin,
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
