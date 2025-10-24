#ifndef SmartPLZARespawnReset_H
#define SmartPLZARespawnReset_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLZARespawnReset : public SmartProgramBase
{
    Q_OBJECT
public:
    explicit SmartPLZARespawnReset(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLZA_RespawnReset; }

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void runRestartCommand();

    // Command indices
    Command const C_Restart     = 0;
    Command const C_COUNT       = 1;

    // List of test color

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,150,520,100);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Restart,
        SS_Title,
        SS_GameStart,

        SS_DetectShiny,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_elapsedTimer;
    int m_shinySoundID;
    bool m_shinyDetected;

    // Stats
    Stat m_statAttempts;
    Stat m_statShiny;
    Stat m_statError;
};

#endif // SmartPLZARespawnReset_H
