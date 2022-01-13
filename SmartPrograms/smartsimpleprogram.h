#ifndef SMARTSIMPLEPROGRAM_H
#define SMARTSIMPLEPROGRAM_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartSimpleProgram : public SmartProgramBase
{
public:
    explicit SmartSimpleProgram(
            SmartProgram programEnum,
            int loopCount,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return m_programEnum; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Loop  = 0;
    Command const C_COUNT = 1;

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
    SmartProgram m_programEnum;
    int m_loopCount;
};

#endif // SMARTSIMPLEPROGRAM_H
