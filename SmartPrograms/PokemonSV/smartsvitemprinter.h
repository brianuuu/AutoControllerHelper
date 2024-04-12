#ifndef SMARTSVITEMPRINTER_H
#define SMARTSVITEMPRINTER_H

#include <QComboBox>
#include <QDateTime>
#include <QTimeZone>
#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartSVItemPrinter : public SmartProgramBase
{
public:
    struct Settings
    {
        qint64 m_seed;
        int m_jobs;
        double m_delay;
        bool m_syncTime;
    };

public:
    explicit SmartSVItemPrinter(
            Settings settings,
            SmartProgramParameter parameter
            );
    virtual SmartProgram getProgramEnum() { return SP_SV_ItemPrinter; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    QString getCommandFromDiff(int diff);

    // Command indices
    Command const C_Restart     = 0;
    Command const C_Talk        = 1;
    Command const C_ToTime      = 2;
    Command const C_SyncTime    = 3;
    Command const C_Print       = 4;
    Command const C_COUNT       = 5;

    // List of test color
    HSVRange const C_Color_TextW = HSVRange(0,0,150,359,30,255); // for white text OCR
    HSVRange const C_Color_Blue = HSVRange(180,190,100,240,255,255); // >230

    // List of test point/area
    CaptureArea const A_Jobs = CaptureArea(1104,245,48,30);
    CaptureArea const A_JobsBonus = CaptureArea(1104,306,48,30);
    CaptureArea const A_Blue = CaptureArea(10,674,400,40);

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Talk,

        SS_ToTime,

        SS_ChangeYear,
        SS_ChangeMonth,
        SS_ChangeDay,
        SS_ChangeHour,
        SS_ChangeMin,
        SS_ChangeAP,
        SS_Delay,

        SS_Jobs,
        SS_JobsStart,
        SS_Print,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;

    QDateTime m_currentDateTime;
    QDateTime m_targetDateTime;
    bool m_addMinute;
    bool m_bonusActive;

    // Stats
};

#endif // SMARTSVITEMPRINTER_H
