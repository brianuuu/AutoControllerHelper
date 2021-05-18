#ifndef SMARTSURPRISETRADE_H
#define SMARTSURPRISETRADE_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartSurpriseTrade : public SmartProgramBase
{
public:
    explicit SmartSurpriseTrade(
            int boxToTrade,
            QPushButton* calibrateBtn,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SurpriseTrade; }

public slots:
    void tradeCompleteCalibrate();

private:
    virtual void init();
    virtual void reset();
    virtual void stop();
    virtual void runNextState();

    // Command indices
    Command const C_GotoYComm       = 0;
    Command const C_PlusPress       = 1;
    Command const C_BPress          = 2;
    Command const C_ConfirmTrade    = 3;
    Command const C_FinishTrade     = 4;
    Command const C_ASpam           = 5;
    Command const C_COUNT           = 6;

    // List of test color
    HSVRange const C_Color_Internet     = HSVRange(220,70,0, 250,255,255);
    QColor const C_Color_Pokemon        = QColor(217,217,215);
    QColor const C_Color_Searching      = QColor(236,240,249);
    QColor C_Color_FinishTrade          = QColor(58,82,238); // can be changed

    // List of test point/area
    QVector<CaptureArea> A_YCommMenu;
    CaptureArea const A_YComm         = CaptureArea(7,664,48,52);
    CaptureArea const A_PokemonName   = CaptureArea(1146,32,24,20);
    CaptureArea const A_DynamaxLevel  = CaptureArea(1088,374,20,20);
    CaptureArea const A_TradeDialog   = CaptureArea(326,676,18,28);
    CaptureArea const A_TradeDialogBIG= CaptureArea(78,659,345,58);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_CheckInternet,
        SS_GotoYComm,
        SS_Connect,
        SS_GotoPokemon,
        SS_ConfirmTrade,
        SS_WaitForTrade,
        SS_FinishTrade,
    };
    Substage m_substage;

    // Members
    bool m_internetConnected;
    bool m_viewStatsChecked;
    int m_currentBox;
    int m_boxToTrade;
    QPoint m_pos;
    QPushButton* m_calibrateBtn;

    bool isLastPokemon() { return (m_currentBox >= m_boxToTrade && m_pos == QPoint(6,5)) || m_currentBox > m_boxToTrade; }
    QString getGotoPokemon();
    QString getNextPokemon();
};

#endif // SMARTSURPRISETRADE_H
