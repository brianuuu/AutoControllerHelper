#ifndef SMARTEGGOPERATION_H
#define SMARTEGGOPERATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"
#include "../Widgets/pokemonstattablewidget.h"

class SmartEggOperation : public SmartProgramBase
{
public:
    enum EggOperationType : uint8_t
    {
        EOT_Collector = 0,
        EOT_Hatcher,
        EOT_Shiny,
        EOT_COUNT,
    };

    enum ShinyDetectionType : uint8_t
    {
        SDT_Disable = 0,
        SDT_Sound,
        SDT_Delay,
    };

    struct Settings
    {
        EggOperationType m_operation;
        int m_targetEggCount;
        int m_columnsToHatch;
        bool m_isHatchExtra;
        ShinyDetectionType m_shinyDetection;
        int m_targetShinyCount;
        PokemonStatTableWidget const* m_statTable;
    };

public:
    explicit SmartEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_EggOperation; }

    // static public functions
    static QVector<CaptureArea> const GetPartyCaptureAreas();
    static QVector<CaptureArea> const GetPartyGenderCaptureAreas(int count = 5);
    static CaptureArea const GetPartyCaptureAreaOfPos(int y);
    static CapturePoint const GetPartyCapturePointOfPos(int y);
    static CaptureArea const GetPartyGenderCaptureAreaOfPos(int y);
    static CaptureArea const GetBoxStatNumArea(StatType type);
    static CaptureArea const GetBoxStatNameArea(StatType type);

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    typedef QPair<int,int> EggPokeCountPair;
    EggPokeCountPair checkPokemonCountInParty();
    QVector<GenderType> checkGenderInParty(int count = 5);
    QVector<CaptureArea> const GetCheckStatCaptureAreas();
    void resetCollectorModeMembers();
    void resetHatcherModeMembers();
    void updateKeepDummy();
    void runKeepPokemonCommand(int yPos = 2);

    // Command indices
    Command const C_CollectFirst    = 0;
    Command const C_CollectCycle    = 1;
    Command const C_CollectEgg      = 2;
    Command const C_ToParty         = 3;
    Command const C_ToBox           = 4;
    Command const C_PartyToBox      = 5;
    Command const C_HatchCycle      = 6;
    Command const C_HatchReturn     = 7;
    Command const C_FirstColumn     = 8;
    Command const C_BoxFiller       = 9;
    Command const C_TakeFiller      = 10;
    Command const C_TakeParent      = 11;
    Command const C_COUNT           = 12;

    // List of test color
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,100); // >180
    HSVRange const C_Color_Grey = HSVRange(0,0,200,359,20,240); // >190
    HSVRange const C_Color_White = HSVRange(0,0,220,359,30,255); // >240
    HSVRange const C_Color_Box = HSVRange(50,170,220,110,255,255); // >240
    HSVRange const C_Color_Dialog = HSVRange(0,0,20,359,40,80); // >240
    HSVRange const C_Color_Shiny = HSVRange(0,0,70,359,40,180); // >25
    HSVRange const C_Color_Text = HSVRange(0,0,0,359,200,200);
    HSVRange const C_Color_Male = HSVRange(200,80,0,260,255,255); // >130
    HSVRange const C_Color_Female = HSVRange(330,80,0,30,255,255); // >130
    HSVRange const C_Color_NatureGood = HSVRange(200,80,0,260,255,255); // >10
    HSVRange const C_Color_NatureBad = HSVRange(300,80,0,255,255,255); // >10

    // List of test point/area
    CaptureArea const A_Nursery1st = CaptureArea(943,390,84,34);
    CaptureArea const A_Nursery2nd = CaptureArea(943,430,84,34);
    CaptureArea const A_Yes = CaptureArea(943,470,84,34);
    CaptureArea const A_No = CaptureArea(943,510,84,34);
    CaptureArea const A_YesNo = CaptureArea(886,549,148,12);
    CaptureArea const A_Box = CaptureArea(0,64,22,200);
    CaptureArea const A_Dialog = CaptureArea(534,650,400,40);
    CaptureArea const A_Shiny = CaptureArea(1242,104,28,30);
    CaptureArea const A_DialogBox = CaptureArea(670,632,200,40);
    CaptureArea const A_Level = CaptureArea(1178,22,100,40);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_InitCheckCount,
        SS_InitBoxView,

        // collect
        SS_CollectCycle,
        SS_CollectTalk,
        SS_CollectEgg,

        // hatch
        SS_ToBox,
        SS_CheckEggCount,
        SS_HatchCycle,
        SS_Hatching,
        SS_CheckStats,
        SS_KeepPokemon,
        SS_NextColumn,
        SS_HatchComplete,

        // check stats
        SS_CheckGender,
        SS_CheckHP,
        SS_CheckAttack,
        SS_CheckDefense,
        SS_CheckSpAtk,
        SS_CheckSpDef,
        SS_CheckSpeed,
        SS_CheckNature,
        SS_CheckShiny,

        // release
        SS_ReleaseHasPokemon,
        SS_ReleaseYesNo,
        SS_ReleaseConfirm,

        // shiny
        SS_BoxFiller,
        SS_TakeFiller,

        SS_Finished,
    };
    Substage m_substage;
    void goToNextCheckStatState(Substage target);

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    bool m_boxViewChecked = false;

    // collect
    bool m_firstCollectCycle;
    int m_eggsCollected;
    int m_talkDialogAttempts;
    int m_eggCollectAttempts;

    // hatch
    int m_eggColumnsHatched;
    int m_eggsToHatchCount; // how many eggs we have hatched for the current column?
    int m_eggsToHatchColumn; // hatching for this loop? should always be 5 but can be fewer
    bool m_blackScreenDetected; // for detecting finishing hatch
    qint64 m_fadeOutDelayTime; // how long does it take for hatch screen to fade to black
    bool m_hatchExtraEgg; // is it time to take remaining egg from nursery and hatch it?
    QVector<GenderType> m_hatchedGenders; // genders of current hatched column
    PokemonStatTable m_hatchedStat; // stats of current pokemon
    PokemonStatTable m_keepDummy; // dummy table to look up which stat we need to check
    PokemonStatTableList m_keepList; // list of extra pokemon we want to keep

    // final
    bool m_videoCaptured; // have we taken a capture for this shiny yet?
    int m_shinySoundID;
    bool m_shinyDetected; // the current hatched egg is a shiny
    int m_shinyWasDetected; // how many shiny there was for the current column of eggs
    int m_shinySingleCount; // how many shiny pokemon found in 30 eggs (shiny mode only)
    int m_shinyCount; // how many shiny pokemon found overall
    int m_keepCount; // how many pokemon we have kept (including shiny)

    // Stats
    Stat m_statError;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
    Stat m_statPokemonKept;
};

#endif // SMARTEGGOPERATION_H
