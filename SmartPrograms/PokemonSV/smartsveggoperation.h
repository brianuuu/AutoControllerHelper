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
        bool m_isHatchWithSandwich;
        bool m_isShinyDetection;
    };

public:
    explicit SmartSVEggOperation(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_EggOperation; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Restart     = 0;
    Command const C_Picnic      = 1;
    Command const C_Fly         = 2;
    Command const C_Sandwich    = 3;
    Command const C_COUNT       = 4;

    // List of test color
    QColor const C_Color_Sandwich = QColor(239,236,221);
    HSVRange const C_Color_Dialog = HSVRange(170,130,20,230,190,60); // >220
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(480,110,320,100);
    CaptureArea const A_Sandwich = CaptureArea(1080,260,160,100);
    CaptureArea const A_Dialog = CaptureArea(700,596,200,40);
    CaptureArea const A_Yes = CaptureArea(1076,406,80,40);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_Title,

        SS_Picnic,
        SS_ToSandwich,
        SS_MakeSandwich,

        SS_CollectEggs,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    int m_sandwichCount; // collector mode only
    int m_eggsCollected; // for counting how many eggs actually collected
    int m_eggCollectCount; // 15 times, every 2 mins in 30 mins
    bool m_eggCollectDialog; // for checking if we are able to talk to egg basket
    bool m_eggCollectDetected; // for detecting Yes/No when collecting eggs

    // Stats
    Stat m_statError;
    Stat m_statEggCollected;
    Stat m_statEggHatched;
    Stat m_statShinyHatched;
};

#endif // SMARTSVEGGOPERATION_H
