#ifndef SMARTPLADISTORTIONWAITER_H
#define SMARTPLADISTORTIONWAITER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"
#include "pokemondatabase.h"

class SmartPLADistortionWaiter : public SmartProgramBase
{
public:
    explicit SmartPLADistortionWaiter(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLA_DistortionWaiter; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    enum NotificationType : uint8_t
    {
        NT_Forming,
        NT_Appeared,
        NT_Faded,

        NT_COUNT,
    };

    // Command indices

    // List of test color
    HSVRange const C_Color_Text = HSVRange(0,0,160,359,80,255);

    // List of test point/area
    CaptureArea const A_Text = CaptureArea(390,98,500,32);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Request,
        SS_Analyze,
        SS_Found,
    };
    Substage m_substage;

    // Members
    QDateTime m_startTime;
    QElapsedTimer m_timer;
};

#endif // SMARTPLADISTORTIONWAITER_H
