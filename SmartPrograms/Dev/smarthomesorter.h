#ifndef SMARTHOMESORTER_H
#define SMARTHOMESORTER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartHomeSorter : public SmartProgramBase
{
public:
    struct Settings
    {
        int m_count;
        bool m_livingDex;
    };

public:
    explicit SmartHomeSorter(Settings settings, SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_HomeSorter; }

    // static public functions
    static CaptureArea const GetCaptureAreaOfPos(int x, int y);

private:
    // structs
    struct PokemonData
    {
        int m_dexNum; // 0 = no pokemon
        bool m_isShiny;
        bool m_isSorted; // used for sorting only

        PokemonData(int dexNum = 0, bool isShiny = false) :
            m_dexNum(dexNum),
            m_isShiny(isShiny),
            m_isSorted(false)
        {}

        bool operator==(PokemonData const& other) const
        {
            return m_dexNum == other.m_dexNum && m_isShiny == other.m_isShiny;
        }
    };

    struct Position
    {
        int m_box; // first box is 0
        QPoint m_point;

        Position(int box = 0, int x = 0, int y = 0) :
            m_box(box),
            m_point(QPoint(x,y))
        {}

        bool operator==(Position const& other) const
        {
            return m_box == other.m_box && m_point == other.m_point;
        }
    };

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    int findClosestUnsortedID();
    int findClosestTargetID();
    static QString gotoPosition(Position from, Position to, bool addDelay);
    static int getIDFromPosition(Position pos);
    static Position getPositionFromID(int id);
    static QString getPositionString(Position pos);

    // Command indices

    // List of test color
    HSVRange const C_Color_Empty = HSVRange(0,0,220,359,40,255); // >250
    HSVRange const C_Color_Summary = HSVRange(150,130,170,190,255,210); // >180
    HSVRange const C_Color_Text = HSVRange(0,0,220,359,255,255);
    HSVRange const C_Color_Shiny = HSVRange(0,0,50,359,70,220); // >50

    // List of test point/area
    CaptureArea const A_All = CaptureArea(52,126,552,380, QColor(255,0,0));
    CaptureArea const A_Summary = CaptureArea(513,176,274,36, QColor(255,0,0));
    CaptureArea const A_Number = CaptureArea(575,183,51,20, QColor(255,0,0));
    CaptureArea const A_Shiny = CaptureArea(912,73,26,28, QColor(255,0,0));

    // Substages
    enum Substage
    {
        SS_Init,

        SS_ScanBox,
        SS_Summary,
        SS_ScanDex,

        SS_Next,
        SS_Swap,

        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    int const m_maxDexNum = 1080;
    int m_pokemonCount;
    int m_currentID;
    int m_checkCount;
    Position m_position;
    QVector<PokemonData> m_pokemonData;
    QVector<PokemonData> m_pokemonDataSorted;

    struct BoxSort
    {
        bool operator() (PokemonData const& a, PokemonData const& b)
        {
            // Empty slot
            if (a.m_dexNum != 0 && b.m_dexNum == 0) return true;
            if (a.m_dexNum == 0 && b.m_dexNum != 0) return false;

            // Shiny
            if (!a.m_isShiny && b.m_isShiny) return true;
            if (a.m_isShiny && !b.m_isShiny) return false;

            // Sort by dex no.
            return (a.m_dexNum < b.m_dexNum);
        }
    };
};

#endif // SMARTHOMESORTER_H
