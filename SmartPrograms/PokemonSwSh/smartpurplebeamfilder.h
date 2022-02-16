#ifndef SMARTPURPLEBEAMFILDER_H
#define SMARTPURPLEBEAMFILDER_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartPurpleBeamFilder : public SmartProgramBase
{
public:
    explicit SmartPurpleBeamFilder(
            QPoint denHolePos,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_PurpleBeamFinder; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_CollectWatt         = 0;
    Command const C_GotoMenu            = 1;
    Command const C_SetTextSpeedSlow    = 2;
    Command const C_SetTextSpeedFast    = 3;
    Command const C_TalkToDen           = 4;
    Command const C_BeamCheck           = 5;
    Command const C_BeamFound           = 6;
    Command const C_RestartGame         = 7;
    Command const C_StartGameA          = 8;
    Command const C_COUNT               = 9;

    // List of test color
    QColor const C_Color_Home = QColor(44,44,44); // black theme
    QColor const C_Color_Home2 = QColor(234,234,234); // white theme
    QColor const C_Color_Menu = QColor(209,29,64);
    HSVRange const C_Color_Beam = HSVRange(290,50,240, 310,160,255);
    HSVRange const C_Color_Option = HSVRange(240,100,140, 250,255,255);
    HSVRange const C_Color_CursorOn = HSVRange(0,0,0, 359,255,60);

    // List of test point/area
    QVector<CaptureArea> A_MenuIcons;
    CapturePoint const P_Home = CapturePoint(1150,550);
    CapturePoint const P_Center = CapturePoint(640,0);
    CaptureArea const A_EnterGame = CaptureArea(600,0,80,80);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_CollectWatt,
        SS_GoToMenu,
        SS_GoToOption,
        SS_SetTextSpeed,

        SS_TalkToDen,
        SS_BeamCheck,
        SS_BeamFound,

        SS_RestartGame,   // Close and launch game
        SS_StartGame,   // Press A when intro starts
        SS_EnterGame    // Wait until black screen goes away
    };
    Substage m_substage;

    // Members
    QRect m_denHoleRect;
    bool m_beamFound;
    bool m_textSpeedSlow;
    int m_triesCount;

    QString getCommandToOption();
};


#endif // SMARTPURPLEBEAMFILDER_H
