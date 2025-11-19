#ifndef SmartPLZAFossil_H
#define SmartPLZAFossil_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLZAFossil : public SmartProgramBase
{
    Q_OBJECT

public:
    enum class FossilType : uint8_t
    {
        FT_ShinyOnly,
        FT_AlphaOnly,
        FT_ShinyOrAlpha,
        FT_ShinyAndAlpha,
    };

    struct Settings
    {
        FossilType m_fossilType;
        int m_count;
        int m_index;
    };

public:
    explicit SmartPLZAFossil(
            Settings settings,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PLZA_Fossil; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void runRestartCommand();
    bool detect(CaptureArea const& area, QImage const& image, HSVRange const& hsvRange);

    // Command indices
    Command const C_Restart     = 0;
    Command const C_ToBox       = 1;
    Command const C_ScrollBox   = 2;
    Command const C_COUNT       = 3;

    // List of test color
    HSVRange const C_Color_Alpha = HSVRange(310,100,0,50,255,255);
    HSVRange const C_Color_Shiny = HSVRange(0,0,0,359,255,200);
    HSVRange const C_Color_White = HSVRange(0,0,220,359,30,255); // >240

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,150,520,100);
    CaptureArea const A_Detect = CaptureArea(1030,81,100,22);
    CaptureArea const A_Selection[3] =
    {
        CaptureArea(1040,330,100,10),
        CaptureArea(1040,382,100,10),
        CaptureArea(1040,434,100,10)
    };

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Restart,
        SS_Title,
        SS_GameStart,

        SS_Talk,
        SS_GetFossil,

        SS_ScrollBoxStart,
        SS_ScrollBox,

        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_elapsedTimer;
    Settings m_programSettings;
    QImage m_imageMatch_Alpha;
    QImage m_imageMatch_Shiny;
    int m_boxIndex;
    int m_fossilCount;
    int m_pressCount;

    // Stats
    Stat m_statFossils;
    Stat m_statFound;
    Stat m_statError;
};

#endif // SmartPLZAFossil_H
