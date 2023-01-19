#ifndef SMARTEGGOPERATION_H
#define SMARTEGGOPERATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartEggOperation : public SmartProgramBase
{
public:
    enum EggOperationType : uint8_t
    {
        EOT_Collector = 0,
        EOT_Hatcher,
        EOT_COUNT,
    };

    struct Settings
    {
        EggOperationType m_operation;
        int m_targetEggCount;
        int m_columnsToHatch;
    };

public:
    explicit SmartEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_EggOperation; }

    // static public functions
    static CaptureArea const GetPartyCaptureAreaOfPos(int y);
    static CapturePoint const GetPartyCapturePointOfPos(int y);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runKeepPokemonCommand(int yPos);

    // Command indices
    Command const C_CollectCycle    = 0;
    Command const C_CollectEgg      = 1;
    Command const C_ToHatch         = 2;
    Command const C_ToCollect       = 3;
    Command const C_ToBox           = 4;
    Command const C_HatchCycle      = 5;
    Command const C_HatchReturn     = 6;
    Command const C_FirstColumn     = 7;
    Command const C_COUNT           = 8;

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,100); // >200
    HSVRange const C_Color_White = HSVRange(0,0,220,359,30,255); // >240
    HSVRange const C_Color_Box = HSVRange(50,170,220,110,255,255); // >240
    HSVRange const C_Color_Dialog = HSVRange(0,0,20,359,40,80); // >240
    HSVRange const C_Color_Shiny = HSVRange(0,0,70,359,40,180); // >25

    // List of test point/area
    CaptureArea const A_Yes = CaptureArea(943,470,84,34);
    CaptureArea const A_No = CaptureArea(943,510,84,34);
    CaptureArea const A_YesNo = CaptureArea(886,549,148,12);
    CaptureArea const A_Box = CaptureArea(0,64,22,200);
    CaptureArea const A_Dialog = CaptureArea(534,650,400,40);
    CaptureArea const A_Shiny = CaptureArea(1242,104,28,30);
    CaptureArea const A_DialogBox = CaptureArea(670,632,200,40);

    // Substages
    enum Substage
    {
        SS_Init,

        // collect
        SS_CollectCycle,
        SS_CollectTalk,
        SS_CollectEgg,

        // hatch
        SS_ToHatch,
        SS_ToBox,
        SS_CheckEggCount,
        SS_HatchCycle,
        SS_Hatching,
        SS_CheckStats,
        SS_KeepPokemon,
        SS_NextColumn,

        // release
        SS_ReleaseHasPokemon,
        SS_ReleaseYesNo,
        SS_ReleaseConfirm,

        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    // collect
    int m_eggsCollected;
    int m_talkDialogAttempts;
    int m_eggCollectAttempts;

    // hatch
    int m_eggColumnsHatched;
    int m_eggsToHatchCount; // how many eggs we have hatched for the current column?
    int m_eggsToHatchColumn; // hatching for this loop? should always be 5 but can be fewer
    bool m_blackScreenDetected; // for detecting finishing hatch

    // final
    int m_shinyCount; // how many shiny pokemon found this session
    int m_keepCount; // how many pokemon we have kept (including shiny)

    // Stats
    Stat m_statError;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
    Stat m_statPokemonKept;
};

#endif // SMARTEGGOPERATION_H
