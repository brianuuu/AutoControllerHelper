#ifndef SMARTPLARESETALPHAPOKEMON_H
#define SMARTPLARESETALPHAPOKEMON_H

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
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PLA_ResetAlphaPokemon; }

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
    HSVRange const C_Color_Alpha = HSVRange(300,100,140,60,255,255); // >70
    HSVRange const C_Color_UI = HSVRange(0,0,150,259,80,255); // >150
    HSVRange const C_Color_Shiny = HSVRange(0,0,130,359,60,255); // >0.3

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,420,520,100);
    CaptureArea const A_Alpha = CaptureArea(393,662,21,22);
    CaptureArea const A_UI = CaptureArea(1006,664,29,23);
    CaptureArea const A_Shiny = CaptureArea(419,629,20,20);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,

        SS_Title,
        SS_GameStart,
        SS_EnterCave,

        SS_Walk,
        SS_FindAlpha,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    AlphaType m_type;
    QImage m_imageMatch_Shiny;
    int m_locateAlphaAttempt;
    bool m_alphaFound;
    bool m_shinyFound;
    bool m_enteredFirstPerson;

    // Stats
    Stat m_statError;
    Stat m_statAttempts;
    Stat m_statAlphaFound;
};

#endif // SMARTPLARESETALPHAPOKEMON_H
