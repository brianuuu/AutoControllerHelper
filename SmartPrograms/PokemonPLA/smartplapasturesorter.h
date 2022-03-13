#ifndef SMARTPLAPASTURESORTER_H
#define SMARTPLAPASTURESORTER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLAPastureSorter : public SmartProgramBase
{
public:
    explicit SmartPLAPastureSorter(SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLA_PastureSorter; }

public:
    struct Settings
    {
        int m_pastureCount;
    };

    struct PokemonData
    {
        int m_dexNum; // 0 = no pokemon
        bool m_isShiny;
        bool m_isAlpha;
        bool m_isUsed; // used for sorting only

        PokemonData() :
            m_dexNum(0),
            m_isShiny(false),
            m_isAlpha(false),
            m_isUsed(false)
        {}
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
        SS_Scan,

        SS_SortStart,
        SS_SortPokemon,

        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_settings;
    bool m_readyNextCheck;

    // Pasture data/functions
    Position m_position;
    QVector<PokemonData> m_pokemonData;
    QVector<PokemonData> m_pokemonDataSorted;
    Position m_positionTemp;
    PokemonData m_dataTemp;
    int findUnusedResult(QVector<PokemonData>& dataAll, PokemonData const& dataQuery);
    void gotoNextPokemon(Position& pos, bool addDelay);
    QString gotoPosition(Position from, Position to, bool addDelay);

    static int getIDFromPosition(Position pos);
    static Position getPositionFromID(int id);
    static QString getPositionString(Position pos);
    static QString getPokemonDataString(PokemonData const& data);

    struct PastureSort
    {
        bool operator() (PokemonData const& a, PokemonData const& b)
        {
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