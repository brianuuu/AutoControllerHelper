#ifndef SMARTSVEGGOPERATION_H
#define SMARTSVEGGOPERATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVEggOperation : public SmartProgramBase
{
public:
    enum EggOperationType : uint8_t
    {
        EOT_Collector = 0,
        EOT_Hatcher,
        EOT_Shiny,
        EOT_COUNT,
    };

    struct Settings
    {
        EggOperationType m_operation;
        int m_sandwichCount;
        int m_sandwichPosX;
        int m_sandwichPosY;
        int m_columnsToHatch;
        bool m_isHatchWithSandwich;
        bool m_isShinyDetection;
        bool m_isErrorCapture;
        bool m_isUseBackupSave;
    };

public:
    explicit SmartSVEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_EggOperation; }

    // static public functions
    static CaptureArea const GetBoxCaptureAreaOfPos(int x, int y);
    static CaptureArea const GetPartyCaptureAreaOfPos(int y);

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void runRestartCommand(QString error = "", bool capture = false);
    void runPicnicCommand();
    void runToBoxCommand(QString command = "", QString commandAdter = "");

    // Command indices
    Command const C_Restart     = 0;
    Command const C_Picnic      = 1;
    Command const C_Picnic2     = 2;
    Command const C_Fly         = 3;
    Command const C_Fly2        = 4;
    Command const C_Sandwich    = 5;
    Command const C_PackUp      = 6;
    Command const C_PackUp2     = 7;
    Command const C_HatchInit   = 8;
    Command const C_MultiSelect = 9;
    Command const C_SwapToHatch = 10;
    Command const C_COUNT       = 11;

    // List of test color
    HSVRange const C_Color_Sandwich = HSVRange(0,0,220,359,40,255); // >220
    HSVRange const C_Color_Dialog = HSVRange(170,130,20,230,255,60); // >200
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200 (box: >130) (battle: >180)
    HSVRange const C_Color_Green = HSVRange(100,170,200,140,255,255); // >200
    HSVRange const C_Color_Shiny = HSVRange(0,0,200,359,255,255); // >25

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(480,110,320,100);
    CaptureArea const A_Sandwich = CaptureArea(1080,260,160,100);
    CaptureArea const A_Dialog = CaptureArea(846,600,100,40);
    CaptureArea const A_Yes = CaptureArea(1076,406,80,40);
    CaptureArea const A_No = CaptureArea(1076,459,80,40);
    CaptureArea const A_Pokemon = CaptureArea(836,4,21,46);
    CaptureArea const A_Box = CaptureArea(1110,226,100,30);
    CaptureArea const A_Picnic = CaptureArea(1110,280,100,30);
    CaptureArea const A_PartyFirst = CaptureArea(84,140,134,12);
    CaptureArea const A_Battle = CaptureArea(1148,466,100,48);
    CaptureArea const A_Health = CaptureArea(69,634,80,20);
    CaptureArea const A_Shiny = CaptureArea(1126,60,30,30);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_Title,
        SS_GameStart,

        SS_MainMenu,

        SS_Picnic,
        SS_ToSandwich,
        SS_MakeSandwich,

        SS_CollectEggs,
        SS_PackUp,
        SS_HatchInit,
        SS_ToBox,
        SS_CheckHasEgg,
        SS_ExitBox,
        SS_HatchEggs,
        SS_ConfirmHatching,
        SS_Battle,
        SS_Hatching,
        SS_CheckShiny,
        SS_NextColumn,
        SS_Fly,

        SS_Finished,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    QElapsedTimer m_sandwichTimer;
    Settings m_programSettings;

    Substage m_substageAfterMainMenu;
    QString m_commandAfterMainMenu;
    bool m_isToBoxAfterMainMenu;

    int m_missedInputRetryCount;

    int m_sandwichCount; // collector & shiny mode only
    int m_eggsCollected; // for counting how many eggs actually collected
    int m_eggsRejected; // when rejecting egg, but twice means we get one anyway
    int m_eggCollectCount; // 15 times, every 2 mins in 30 mins
    bool m_eggDialogDetected; // for checking if we are able to talk to egg basket
    bool m_eggRejectDetected; // for detecting No when collecting eggs

    int m_eggColumnsHatched;
    int m_eggsToHatch; // how many eggs we still need to hatch for the current column?
    int m_eggsToHatchColumn; // hatching for this loop? mostly 5 except last column (does not change)
    int m_eggsHatched; // how many eggs we have hatched this session
    int m_hasPokemonCount; // counter for check if pokemon exist in party
    int m_checkShinyCount; // counter for checking shiny
    int m_flyAttempts;
    bool m_flySuccess; // check if we were able to fly
    typedef QPair<QPoint,int> PosEggCountMap;
    QVector<PosEggCountMap> m_shinyPositions; // how many shiny we found this session (pos,egg no)

    int m_shinySoundID;
    bool m_shinySoundDetected;

    // Stats
    Stat m_statError;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
};

#endif // SMARTSVEGGOPERATION_H
