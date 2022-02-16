#ifndef SMARTDELAYCALIBRATOR_H
#define SMARTDELAYCALIBRATOR_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

#define COUNT_MAX 4

class SmartDelayCalibrator : public SmartProgramBase
{
public:
    explicit SmartDelayCalibrator(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_DelayCalibrator; }

    int getCameraDelay() const { return m_delay; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_HomePress       = 0;
    Command const C_ToTestButton    = 1;
    Command const C_APress          = 2;
    Command const C_BackToGame      = 3;
    Command const C_COUNT           = 4;

    // List of test color
    QColor const C_Color_Home = QColor(44,44,44); // black theme
    QColor const C_Color_Home2 = QColor(234,234,234); // white theme
    HSVRange const C_Color_TestButton = HSVRange(150,180,0,180,255,255);  // black theme
    HSVRange const C_Color_TestButton2 = HSVRange(220,180,0,250,255,255);  // white theme

    // List of test point/area
    CapturePoint const P_Home = CapturePoint(1150,550);
    QVector<CaptureArea> A_TestButtons;

    // Substages
    enum Substage
    {
        SS_Init,
        SS_GotoHome,
        SS_GoToTestButton,
        SS_CalibrateDelay,
        SS_BackToGame,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    int m_delay;
    int m_count;
};

#endif // SMARTDELAYCALIBRATOR_H
