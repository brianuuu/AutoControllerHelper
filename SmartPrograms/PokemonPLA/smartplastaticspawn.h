#ifndef SMARTPLASTATICSPAWN_H
#define SMARTPLASTATICSPAWN_H

#include <QComboBox>
#include <QTimer>
#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"
#include "pokemondatabase.h"

class SmartPLAStaticSpawn : public SmartProgramBase
{
    Q_OBJECT
public:
    explicit SmartPLAStaticSpawn(
            QString const& staticPokemon,
            bool ignoreEarlyShiny,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PLA_StaticSpawn; }

public:
    static void populateStaticPokemon(QComboBox* cb);

private slots:
    void ignoreShinyTimeout();
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void runRestartCommand();

    // Command indices
    Command const C_TalkToLaventon  = 0;
    Command const C_COUNT           = 1;

    // List of test color
    HSVRange const C_Color_Loading = HSVRange(0,0,0,359,255,40);
    HSVRange const C_Color_Map = HSVRange(0,0,110,359,255,150);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,420,520,100);
    CaptureArea const A_Loading = CaptureArea(560,654,200,42);
    CaptureArea const A_Map = CaptureArea(490,692,200,24);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Restart,
        SS_Title,
        SS_GameStart,

        SS_DetectMap,
        SS_EnterArea,
        SS_LoadingToArea,
        SS_GotoCamp,

        SS_DetectShiny,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;

    QString m_staticPokemon;
    QString m_navigateCommands;
    PLAAreaType m_areaType;
    int m_campID;
    int m_ignoreShinyTimeMS;

    bool m_ignoreEarlyShiny;
    int m_shinySoundID;
    bool m_shinyDetected;

    // Stats
    Stat m_statAttempts;
    Stat m_statShiny;
    Stat m_statError;
};

#endif // SMARTPLASTATICSPAWN_H
