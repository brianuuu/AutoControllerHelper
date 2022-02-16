#ifndef SMARTDAYSKIPPER_H
#define SMARTDAYSKIPPER_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartDaySkipper : public SmartProgramBase
{
public:
    explicit SmartDaySkipper(
            int skips,
            QLabel* estimateLabel,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_DaySkipper; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_StartOK     = 0;
    Command const C_SyncTime    = 1;
    Command const C_SyncTimeWait= 2;
    Command const C_Back2000JP  = 3;
    Command const C_Back2000EU  = 4;
    Command const C_Back2000US  = 5;
    Command const C_SkipEU_JPDay= 6;
    Command const C_SkipUS      = 7;
    Command const C_BackToGame  = 8;
    Command const C_SkipJPMonth = 9;
    Command const C_SkipJPYear  = 10;
    Command const C_COUNT       = 11;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_ToOK,
        SS_Back2000,
        SS_Skip,
        SS_BackToGame,
    };
    Substage m_substage;

    // Members
    QDate m_date;
    int m_skipsLeft;
    int m_skippedDays;
    DateArrangement m_dateArrangement;
    QLabel* m_estimateLabel;

    QDateTime m_startDateTime;

    bool is1159PM();
    void runSyncTime();
    QString secondsToString(double time);
    QString getToJanuary();
};

#endif // SMARTDAYSKIPPER_H
