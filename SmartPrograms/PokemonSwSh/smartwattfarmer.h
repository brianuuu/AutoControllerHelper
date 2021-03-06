#ifndef SMARTWATTFARMER_H
#define SMARTWATTFARMER_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartWattFarmer : public SmartProgramBase
{
public:
    explicit SmartWattFarmer(
            int skips,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_WattFarmer; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_SyncTime    = 0;
    Command const C_SkipYearJP  = 1;
    Command const C_SkipYearEU  = 2;
    Command const C_SkipYearUS  = 3;
    Command const C_BackToGame  = 4;
    Command const C_Loop        = 5;
    Command const C_COUNT       = 6;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_SyncTime,
        SS_SkipYear,
        SS_BackToGame,
        SS_Loop,
    };
    Substage m_substage;

    // Members
    int m_skipsLeft;
    int m_daySkipped;
};

#endif // SMARTWATTFARMER_H
