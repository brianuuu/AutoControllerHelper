#ifndef SMARTTRADEPARTNERFINDER_H
#define SMARTTRADEPARTNERFINDER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartTradePartnerFinder : public SmartProgramBase
{
public:
    explicit SmartTradePartnerFinder(
            QString partnerName,
            QString linkCode,
            bool spamA,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_TradePartnerFinder; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    void runCommandToKey();

    // Command indices
    Command const C_GotoYComm   = 0;
    Command const C_SetLinkCode = 1;
    Command const C_FindPartner = 2;
    Command const C_COUNT       = 3;

    // List of test color
    HSVRange const C_Color_Internet = HSVRange(220,70,0, 250,255,255);
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,30,150); // for OCR
    HSVRange const C_Color_Box = HSVRange(50,170,220,110,255,255); // >240

    // List of test point/area
    QVector<CaptureArea> A_YCommMenu;
    QRect const A_Internet = QRect(7,664,48,52);
    CaptureArea const A_Name = CaptureArea(932,72,308,48);
    CaptureArea const A_Box = CaptureArea(0,64,22,200);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_CheckInternet,
        SS_GotoYComm,
        SS_SetLinkCode,
        SS_FindPartner,
        SS_Return,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    QString m_partnerName;
	QString m_linkCode;
    bool m_spamA;

    QPoint m_pos;
    QString m_code;
    bool m_retry;

    // Stats
};

#endif // SMARTBOXRELEASE_H
