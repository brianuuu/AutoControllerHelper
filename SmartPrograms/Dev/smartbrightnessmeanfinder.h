#ifndef SMARTBRIGHTNESSMEANFINDER_H
#define SMARTBRIGHTNESSMEANFINDER_H

#include <QCheckBox>
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
            QCheckBox* matchScale,
            QPushButton* ocrBtn,
            QLineEdit* ocrLE,
            QPushButton* ocrNumBtn,
            QPushButton* fixedImageBtn,
            QCheckBox* fixedImageCheck,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BrightnessMeanFinder; }

public slots:
    void imageMatchAdd();
    void orcRequested() { m_ocrRequested = true; }
    void orcNumRequested() { m_ocrNumRequested = true; }
    void fixedImageAdd();
    void fixImageStateChanged(int state);

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
    QCheckBox* m_imageMatchScale;
    bool m_imageMatchStarted;
    QImage m_imageMatchImage;

    bool m_ocrRequested;
    QPushButton* m_ocrBtn;
    QLineEdit* m_ocrLE;

    bool m_ocrNumRequested;
    QPushButton* m_ocrNumBtn;

    QPushButton* m_fixedImageBtn;
    QCheckBox* m_fixedImageCheck;
};

#endif // SMARTBRIGHTNESSMEANFINDER_H
