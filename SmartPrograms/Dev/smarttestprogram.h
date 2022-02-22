#ifndef SMARTTESTPROGRAM_H
#define SMARTTESTPROGRAM_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartTestProgram : public SmartProgramBase
{
public:
    explicit SmartTestProgram(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_Test; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Loop,
        SS_Interrupt,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
};

#endif // SMARTTESTPROGRAM_H
