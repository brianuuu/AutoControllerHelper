#ifndef SMARTPLAPASTURESORTER_H
#define SMARTPLAPASTURESORTER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLAPastureSorter : public SmartProgramBase
{
public:
    struct Settings
    {
        int m_pastureCount;

        bool m_livingDex;
        bool m_livingDexShiny;
        bool m_livingDexAlpha;
    };

    struct PokemonData
    {
        int m_dexNum; // 0 = no pokemon
        bool m_isShiny;
        bool m_isAlpha;
        bool m_isSorted; // used for sorting only

        PokemonData(int dexNum = 0, bool isShiny = false, bool isAlpha = false) :
            m_dexNum(dexNum),
            m_isShiny(isShiny),
            m_isAlpha(isAlpha),
            m_isSorted(false)
        {}

        bool operator==(PokemonData const& other) const
        {
            return m_dexNum == other.m_dexNum && m_isShiny == other.m_isShiny && m_isAlpha == other.m_isAlpha;
        }
    };

    struct Position
    {
        int m_pasture;
        QPoint m_point;

        Position(int pasture = 1, int x = 1, int y = 1) :
            m_pasture(pasture),
            m_point(QPoint(x,y))
        {}
    };

public:
    explicit SmartPLAPastureSorter(Settings setting, SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLA_PastureSorter; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices

    // List of test color
    HSVRange const C_Color_Text = HSVRange(0,0,140,359,80,255);
    HSVRange const C_Color_Stat = HSVRange(0,0,220,359,80,255); // >230

    // List of test point/area
    CaptureArea const A_Level = CaptureArea(940,114,76,24);
    CaptureArea const A_Stat = CaptureArea(1185,72,20,24);
    CaptureArea const A_Shiny = CaptureArea(1206,72,20,24);
    CaptureArea const A_Alpha = CaptureArea(1227,72,20,24);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_MoveMode,
        SS_Scan,
        SS_Sort,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    bool m_readyNextCheck;

    // Pasture data/functions
    Position m_position;
    Position m_positionTemp;
    PokemonData m_dataTemp;
    QVector<PokemonData> m_pokemonData;
    QVector<PokemonData> m_pokemonDataSorted;

    struct SearchResult
    {
        int m_idCurrent;
        int m_idResult;
        bool m_isNearResult;

        SearchResult() :
            m_idCurrent(-1),
            m_idResult(-1),
            m_isNearResult(false)
        {}
    } m_searchResult;

    int findUnsortedResult(QVector<PokemonData> const& dataAll, PokemonData const& dataQuery);
    QString gotoNextPokemon(Position& pos, bool addDelay);
    QString gotoPosition(Position from, Position to, bool addDelay);

    static int getIDFromPosition(Position pos);
    static Position getPositionFromID(int id);
    static QString getPositionString(Position pos);
    static QString getPokemonDataString(PokemonData const& data);

    struct PastureSort
    {
        bool operator() (PokemonData const& a, PokemonData const& b)
        {
            // Empty slot
            if (a.m_dexNum != 0 && b.m_dexNum == 0) return true;
            if (a.m_dexNum == 0 && b.m_dexNum != 0) return false;

            // Shiny alpha
            if (!a.m_isAlpha && !a.m_isShiny && b.m_isAlpha && b.m_isShiny) return true;
            if (a.m_isAlpha && a.m_isShiny && !b.m_isAlpha && !b.m_isShiny) return false;

            // Shiny
            if (!a.m_isAlpha && !a.m_isShiny && !b.m_isAlpha && b.m_isShiny) return true;
            if (!a.m_isAlpha && a.m_isShiny && !b.m_isAlpha && !b.m_isShiny) return false;

            // Alpha
            if (!a.m_isAlpha && !a.m_isShiny && b.m_isAlpha && !b.m_isShiny) return true;
            if (a.m_isAlpha && !a.m_isShiny && !b.m_isAlpha && !b.m_isShiny) return false;

            // Sort by dex no.
            return (a.m_dexNum < b.m_dexNum);
        }
    };

    static void PastureDebug(QVector<PokemonData> const& dataAll);
};

#endif // SMARTPLAPASTURESORTER_H
