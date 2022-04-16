#ifndef SMARTPLAMMORESPAWN_H
#define SMARTPLAMMORESPAWN_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLAMMORespawn : public SmartProgramBase
{
    Q_OBJECT
public:
    explicit SmartPLAMMORespawn(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLA_MMORespawn; }

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void runRestartCommand();

    // Command indices

    // List of test color

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,420,520,100);

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

#endif // SMARTPLAMMORESPAWN_H
