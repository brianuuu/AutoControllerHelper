#ifndef SMARTDAILYHIGHLIGHT_H
#define SMARTDAILYHIGHLIGHT_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartDailyHighlight : public SmartProgramBase
{
public:
    explicit SmartDailyHighlight(
            int skips,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_DailyHighlight; }

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
    Command const C_Start       = 5;
    Command const C_BSpam       = 6;
    Command const C_End         = 7;
    Command const C_COUNT       = 8;

    // List of test color
    QColor const C_Color_Dialog = QColor(50,50,50);

    // List of test point/area
    QRect const A_Dialog = QRect(805,646,80,32);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_SyncTime,
        SS_SkipYear,
        SS_BackToGame,
        SS_Start,
        SS_BSpam,
        SS_End
    };
    Substage m_substage;

    // Members
    int m_skipsLeft;
    int m_daySkipped;
};

#endif // SMARTDAILYHIGHLIGHT_H
