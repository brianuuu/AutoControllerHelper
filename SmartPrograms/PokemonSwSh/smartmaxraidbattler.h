#ifndef SMARTMAXRAIDBATTLER_H
#define SMARTMAXRAIDBATTLER_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartMaxRaidBattler : public SmartProgramBase
{
public:
    explicit SmartMaxRaidBattler(
            int maxBattle,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_MaxRaidBattler; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_ASpam       = 0;
    Command const C_APress      = 1;
    Command const C_StartRaid   = 2;
    Command const C_QuitRaid    = 3;
    Command const C_COUNT       = 4;

    // List of test color
    /*
    HSVRange const C_Color_Raid = HSVRange(0,0,230, 359,50,255);
    HSVRange const C_Color_Common = HSVRange(350,160,230, 359,180,255);
    HSVRange const C_Color_Rare = HSVRange(30,190,230, 40,210,255);
    */

    QColor const C_Color_Black = QColor(0,0,0);
    QColor const C_Color_Dialog = QColor(50,50,50);
    QColor const C_Color_White = QColor(253,253,253);

    // List of test point/area
    /*
    CaptureArea const A_RaidWhite = CaptureArea(1180,120,100,100);
    CaptureArea const A_RaidRare = CaptureArea(642,36,100,100);
    */
    QVector<CaptureArea> A_RaidMenu;

    // Substages
    enum Substage
    {
        SS_Init,
        SS_ASpam,
        SS_APress,
        SS_StartRaid,
        SS_QuitRaid,
    };
    Substage m_substage;

    // Members
    int m_curBattle;
    int m_maxBattle;
};

#endif // SMARTMAXRAIDBATTLER_H
