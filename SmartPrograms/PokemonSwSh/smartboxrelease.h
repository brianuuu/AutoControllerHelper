#ifndef SMARTBOXRELEASE_H
#define SMARTBOXRELEASE_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartBoxRelease : public SmartProgramBase
{

public:
    explicit SmartBoxRelease(
            int targetBoxCount,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BoxRelease; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    static QPoint GetCurrentBoxPosFromReleaseCount(int count);
    static QString GetCurrentBoxPosString(int count);

    // Command indices

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,100); // >200
    HSVRange const C_Color_Dialog = HSVRange(0,0,20,359,40,80); // >240
    HSVRange const C_Color_Shiny = HSVRange(0,0,70,359,40,180); // >25

    // List of test point/area
    CaptureArea const A_No = CaptureArea(943,510,84,34);
    CaptureArea const A_Shiny = CaptureArea(1242,104,28,30);
    CaptureArea const A_DialogBox = CaptureArea(670,632,200,40);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_ReleaseHasPokemon,
        SS_ReleaseYesNo,
        SS_ReleaseConfirm,

        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    int m_targetBoxCount;
    int m_releaseCount;
};

#endif // SMARTBOXRELEASE_H
