#ifndef SMARTPLAOUTBREAKFINDER_H
#define SMARTPLAOUTBREAKFINDER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"
#include "pokemondatabase.h"

class SmartPLAOutbreakFinder : public SmartProgramBase
{
public:
    explicit SmartPLAOutbreakFinder(
            QString const& pokemonString,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PLA_OutbreakFinder; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_TalkToLaventon  = 0;
    Command const C_COUNT           = 1;

    // List of test color
    HSVRange const C_Color_Loading = HSVRange(0,0,0,359,255,40);
    HSVRange const C_Color_Text = HSVRange(0,0,160,359,80,255);
    HSVRange const C_Color_Map = HSVRange(0,0,110,359,255,150);

    // List of test point/area
    CaptureArea const A_Loading = CaptureArea(560,654,200,42);
    CaptureArea const A_Text = CaptureArea(70,126,200,28);
    CaptureArea const A_Map = CaptureArea(490,692,200,24);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_WalkToLaventon,
        SS_TalkToLaventon,

        SS_LoadingToVillage,
        SS_LoadingToObsidian,
        SS_DetectMap,

        SS_GotoMap,
        SS_EnterObsidian,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    QStringList m_outbreakPokemonList;  // desired pokemon
    PokemonDatabase::OCREntries m_allOutbreakEntries;   // cached OCR entries for all outbreak pokemon
    bool m_firstCheck;      // for extra up cursor movement when first entering map
    PokemonDatabase::PLAAreaType m_areaType;    // current area being detected for OCR
    bool m_readyNextCheck;  // for when it's ready to do OCR for next area

    // Stats
    Stat m_statChecks;
    Stat m_statError;
};

#endif // SMARTPLAOUTBREAKFINDER_H
