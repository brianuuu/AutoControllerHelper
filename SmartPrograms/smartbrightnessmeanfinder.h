#ifndef SMARTBRIGHTNESSMEANFINDER_H
#define SMARTBRIGHTNESSMEANFINDER_H

#include <QSpinBox>
#include <QWidget>
#include "smartprogrambase.h"

class SmartBrightnessMeanFinder : public SmartProgramBase
{
public:
    explicit SmartBrightnessMeanFinder(
            QVector<QSpinBox*> spinBoxes,
            QLabel* label,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BrightnessMeanFinder; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Capture,
    };
    Substage m_substage;

    // Members
    QVector<QSpinBox*> m_spinBoxes; // 0-3:Rect, 4-6: MinHSV, 7-9: MaxHSV
    QLabel* m_meanOutput;
};

#endif // SMARTBRIGHTNESSMEANFINDER_H
