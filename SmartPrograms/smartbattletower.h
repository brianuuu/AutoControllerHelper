#ifndef SMARTBATTLETOWER_H
#define SMARTBATTLETOWER_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartBattleTower : public SmartProgramBase
{
public:
    explicit SmartBattleTower(
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BattleTower; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Loop  = 0;
    Command const C_COUNT   = 1;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Loop,
    };
    Substage m_substage;

    // Members
};

#endif // SMARTBATTLETOWER_H
