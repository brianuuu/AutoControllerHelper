#ifndef SMARTSVTRADEPARTNERFINDER_H
#define SMARTSVTRADEPARTNERFINDER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVTradePartnerFinder : public SmartProgramBase
{
public:
    explicit SmartSVTradePartnerFinder(
            QString partnerName,
            bool spamA,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SV_TradePartnerFinder; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices

    // List of test color
    HSVRange const C_Color_Yellow = HSVRange(30,200,200,70,255,255); // >200
    HSVRange const C_Color_Black = HSVRange(0,0,0,359,255,150); // for OCR
    HSVRange const C_Color_Dialog = HSVRange(170,130,20,230,255,60); // >220

    // List of test point/area
    CaptureArea const A_LinkTrade = CaptureArea(306,346,70,36);
    CaptureArea const A_Name = CaptureArea(274,4,217,40);
    CaptureArea const A_Dialog = CaptureArea(800,596,100,40);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Restart,
        SS_FindPartner,
        SS_WrongPartner,
        SS_CorrectPartner,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    QString m_partnerName;
    bool m_spamA;

    // Stats
};

#endif // SMARTSVBOXRELEASE_H
