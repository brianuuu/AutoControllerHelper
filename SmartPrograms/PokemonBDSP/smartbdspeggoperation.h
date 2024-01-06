#ifndef SMARTBDSPEGGOPERATION_H
#define SMARTBDSPEGGOPERATION_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

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
    };

public:
    explicit SmartBDSPEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_EggOperation; }

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
    Command const C_COUNT               = 5;

    // List of test color
    HSVRange const C_Color_Dialog = HSVRange(0,0,230,359,30,255); // >150

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,380,520,100);
    CaptureArea const A_Dialog = CaptureArea(718,650,300,40);
    CaptureArea const A_Watch = CaptureArea(1241,142,26,48);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Start,
        SS_Watch,

        SS_CycleCollect,
        SS_Collect,
        SS_CollectSuccess,

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

    int m_resetCountdown;
    QColor m_watchColor;

    bool m_dialogDetected;
    int m_eggsCollected;

    // Stats
    Stat m_error;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
};

#endif // SMARTBDSPEGGOPERATION_H
