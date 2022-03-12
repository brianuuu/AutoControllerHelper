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

    enum AreaType : uint8_t
    {
        AT_ObsidianFieldlands,
        AT_CrimsonMirelands,
        AT_CobaltCoastlands,
        AT_CoronetHighlands,
        AT_AlabasterIcelands,
    };

    static QString AreaTypeToString(AreaType type)
    {
        switch (type)
        {
        case AT_ObsidianFieldlands: return "Obsidian Fieldlands";
        case AT_CrimsonMirelands:   return "Crimson Mirelands";
        case AT_CobaltCoastlands:   return "Cobalt Coastlands";
        case AT_CoronetHighlands:   return "Coronet Highlands";
        case AT_AlabasterIcelands:  return "Alabaster Icelands";
        }

        return "INVALID AREA";
    }

    // Command indices
    Command const C_TalkToLaventon  = 0;
    Command const C_COUNT           = 1;

    // List of test color
    HSVRange const C_Color_Loading = HSVRange(0,0,0,359,255,40);
    HSVRange const C_Color_Text = HSVRange(0,0,160,359,80,255);

    // List of test point/area
    CaptureArea const A_Loading = CaptureArea(560,654,200,42);
    CaptureArea const A_Text = CaptureArea(70,126,200,28);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_WalkToLaventon,
        SS_TalkToLaventon,

        SS_LoadingToVillage,
        SS_LoadingToObsidian,

        SS_GotoMap,
        SS_EnterObsidian,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    QStringList m_outbreakPokemonList;  // desired pokemon
    PokemonDatabase::OCREntries m_allOutbreakEntries;   // cached OCR entries for all outbreak pokemon
    bool m_firstCheck;      // for extra up cursor movement when first entering map
    AreaType m_areaType;    // current area being detected for OCR
    bool m_readyNextCheck;  // for when it's ready to do OCR for next area

    // Stats
    Stat m_statChecks;
    Stat m_statError;
};

#endif // SMARTPLAOUTBREAKFINDER_H
