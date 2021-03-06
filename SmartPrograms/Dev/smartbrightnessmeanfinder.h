#ifndef SMARTBRIGHTNESSMEANFINDER_H
#define SMARTBRIGHTNESSMEANFINDER_H

#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartBrightnessMeanFinder : public SmartProgramBase
{
public:
    explicit SmartBrightnessMeanFinder(
            QVector<QSpinBox*> spinBoxes,
            QLabel* labelMean,
            QPushButton* imageMatchBtn,
            QLabel* labelImageMatch,
            QPushButton* ocrBtn,
            QLineEdit* ocrLE,
            QPushButton* ocrNumBtn,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BrightnessMeanFinder; }

public slots:
    void imageMatchAdd();
    void orcRequested() { m_ocrRequested = true; }
    void orcNumRequested() { m_ocrNumRequested = true; }

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

    QPushButton* m_imageMatchBtn;
    QLabel* m_imageMatchResult;
    bool m_imageMatchStarted;
    QImage m_imageMatchImage;

    bool m_ocrRequested;
    QPushButton* m_ocrBtn;
    QLineEdit* m_ocrLE;

    bool m_ocrNumRequested;
    QPushButton* m_ocrNumBtn;
};

#endif // SMARTBRIGHTNESSMEANFINDER_H
