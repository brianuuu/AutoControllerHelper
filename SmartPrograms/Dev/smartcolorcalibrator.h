#ifndef SMARTCOLORCALIBRATOR_H
#define SMARTCOLORCALIBRATOR_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartColorCalibrator : public SmartProgramBase
{
public:
    explicit SmartColorCalibrator(
            QLabel* labelBattle,
            QLabel* labelPokemon,
            QLabel* labelBag,
            QLabel* labelRun,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_ColorCalibrator; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void testColor(CapturePoint const& cp, HSVRange const& range, QLabel* label);

    // Command indices

    // List of test color
    HSVRange const C_Color_Battle = HSVRange(343,168,184,3,188,204);
    HSVRange const C_Color_Pokemon = HSVRange(101,144,191,121,164,211);
    HSVRange const C_Color_Bag = HSVRange(41,176,193,61,196,213);
    HSVRange const C_Color_Run = HSVRange(229,113,204,249,133,224);

    // List of test point/area
    CapturePoint P_Battle   = CapturePoint(1171,444);
    CapturePoint P_Pokemon  = CapturePoint(1171,522);
    CapturePoint P_Bag      = CapturePoint(1171,600);
    CapturePoint P_Run      = CapturePoint(1171,678);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Wait,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    QLabel* m_battle;
    QLabel* m_pokemon;
    QLabel* m_bag;
    QLabel* m_run;
};

#endif // SMARTCOLORCALIBRATOR_H
