#ifndef SMARTPLARESETALPHAPOKEMON_H
#define SMARTPLARESETALPHAPOKEMON_H

#include <QTimer>
#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLAResetAlphaPokemon : public SmartProgramBase
{
public:
    enum AlphaType : uint8_t
    {
        AT_Gallade = 0,
        AT_Crobat,
        AT_COUNT,
    };

public:
    explicit SmartPLAResetAlphaPokemon(
            AlphaType type,
            bool ignoreNonAlpha,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PLA_ResetAlphaPokemon; }

private slots:
    void ignoreShinyTimeout();
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Restart     = 0;
    Command const C_WalkGallade = 1;
    Command const C_WalkCrobat  = 2;
    Command const C_COUNT       = 3;

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
        SS_EnterCave,

        SS_Walk,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    AlphaType m_type;
    bool m_ignoreNonAlpha;
    int m_shinySoundID;
    bool m_shinyDetected;

    // Stats
    Stat m_statError;
    Stat m_statAttempts;
    Stat m_statShiny;
};

#endif // SMARTPLARESETALPHAPOKEMON_H
