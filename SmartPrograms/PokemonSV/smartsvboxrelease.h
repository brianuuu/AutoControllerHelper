#ifndef SMARTSVBOXRELEASE_H
#define SMARTSVBOXRELEASE_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVBoxRelease : public SmartProgramBase
{
public:
    explicit SmartSVBoxRelease(
            int releaseTarget,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_BoxRelease; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Release = 0;
    Command const C_Next    = 1;
    Command const C_NextRow = 2;
    Command const C_NextBox = 3;
    Command const C_COUNT   = 4;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Release,
        SS_Next,
    };
    Substage m_substage;

    // Members
    int m_releaseTarget;
    int m_releaseCount;
    QPoint m_pos;

    // Stats
};

#endif // SMARTSVBOXRELEASE_H
