#ifndef SMARTBDSPEGGOPERATION_H
#define SMARTBDSPEGGOPERATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"
#include "../Widgets/pokemonstattablewidget.h"

class SmartBDSPEggOperation : public SmartProgramBase
{
public:
    enum class EggOperationType : uint8_t
    {
        EOT_Collector = 0,
        EOT_Hatcher,
        EOT_Shiny,
        EOT_COUNT,
    };

    enum class ShinyDetectionType : uint8_t
    {
        SDT_Disable = 0,
        SDT_Sound,
    };

    struct Settings
    {
        EggOperationType m_operation;
        int m_targetEggCount;
        int m_columnsToHatch;
        ShinyDetectionType m_shinyDetection;
        PokemonStatTableWidget const* m_statTable;
    };

public:
    explicit SmartBDSPEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_EggOperation; }

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Restart             = 0;
    Command const C_RestartNoUpdate     = 1;
    Command const C_CycleCollect        = 2;
    Command const C_Collect             = 3;
    Command const C_CycleReturn         = 4;
    Command const C_ToBox               = 5;
    Command const C_QuitBox             = 6;
    Command const C_CycleHatch          = 7;
    Command const C_Release             = 8;
    Command const C_PartyKeep           = 9;
    Command const C_PartyBack           = 10;
    Command const C_QuitBoxSave         = 11;
    Command const C_COUNT               = 12;

    // List of test color
    HSVRange const C_Color_Dialog = HSVRange(0,0,230,359,30,255); // >150
    HSVRange const C_Color_Shiny = HSVRange(320,0,150,20,255,255); // >60
    QColor const C_Color_Watch = QColor(147,0,49);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Dialog = CaptureArea(718,650,300,40);
    CaptureArea const A_Watch = CaptureArea(1241,142,26,48);
    CaptureArea const A_Pokemon = CaptureArea(900,184,100,180);
    CaptureArea const A_Stat = CaptureArea(1170,184,100,180);
    CaptureArea const A_Shiny = CaptureArea(1234,106,33,33);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Start,
        SS_Watch,

        SS_CycleCollect,
        SS_Collect,
        SS_CollectSuccess,

        SS_PartyKeep,
        SS_PartyBack,

        SS_ToBox,
        SS_CheckView,
        SS_CheckShiny,
        SS_PickEggs,
        SS_QuitBox,
        SS_HatchEggs,

        SS_Restart,
        SS_Intro,
        SS_Title,
        SS_GameStart,

        SS_Finished
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    bool m_watchEnabled;

    // collect
    bool m_dialogDetected;
    int m_eggsCollected;

    // hatch
    int m_eggColumnsHatched;
    int m_eggsToHatchCount; // how many eggs we have hatched for the current column?
    int m_eggsToHatchColumn; // hatching for this loop? should always be 5 but can be fewer
    bool m_isStatView;
    int m_hatchingDialog; // how many times white detected during hatch (should be 3)

    // shiny
    bool m_shinyWasFound; // there was a shiny in the current 5 eggs
    int m_shinyCount; // how many shiny pokemon found overall
    int m_shinySoundID;
    bool m_shinyDetected; // the current hatched egg is a shiny
    int m_loopDone;

    // Stats
    Stat m_error;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
};

#endif // SMARTBDSPEGGOPERATION_H
