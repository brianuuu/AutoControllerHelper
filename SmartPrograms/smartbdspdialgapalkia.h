#ifndef SMARTBDSPDIALGAPALKIA_H
#define SMARTBDSPDIALGAPALKIA_H

#include <QWidget>
#include <QElapsedTimer>
#include "smartprogrambase.h"

class SmartBDSPDialgaPalkia : public SmartProgramBase
{
public:
    explicit SmartBDSPDialgaPalkia(
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_DialgaPalkia; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runRestartCommand();

    // Command indices
    Command const C_Restart = 0;
    Command const C_RestartNoUpdate = 1;
    Command const C_Talk    = 2;
    Command const C_Capture = 3;
    Command const C_COUNT   = 4;

    // List of test color
    HSVRange const C_Color_Battle = HSVRange(339,130,180,359,190,220);
    HSVRange const C_Color_Pokemon = HSVRange(100,130,180,120,190,220);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Battle = CaptureArea(1128,422,48,48);
    CaptureArea const A_Pokemon = CaptureArea(1128,500,48,48);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_Intro,
        SS_Title,
        SS_GameStart,
        SS_Talk,
        SS_Calibrate,
        SS_TestShiny,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    double m_noShinyTimer;
    QElapsedTimer m_elapsedTimer;

    // Stats
    Stat m_encounter;
    Stat m_error;
};

#endif // SMARTBDSPDIALGAPALKIA_H
