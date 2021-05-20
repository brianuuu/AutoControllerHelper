#include "smartbrightnessmeanfinder.h"

SmartBrightnessMeanFinder::SmartBrightnessMeanFinder
(
    QVector<QSpinBox*> spinBoxes,
    QLabel* label,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_spinBoxes(spinBoxes)
    , m_meanOutput(label)
{
    init();

    if (m_spinBoxes.size() != 10)
    {
        setState_error("Spinbox argument is expected to be 10");
    }
    else
    {
        for (QSpinBox* spinBox : m_spinBoxes)
        {
            if (!spinBox)
            {
                setState_error("Null spinbox reference provided");
                break;
            }
        }
    }
}

void SmartBrightnessMeanFinder::init()
{
    SmartProgramBase::init();
}

void SmartBrightnessMeanFinder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartBrightnessMeanFinder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Capture;
        setState_frameAnalyzeRequest();
        break;
    }
    case SS_Capture:
    {
        if (state == S_CaptureReady)
        {
            QRect rect(m_spinBoxes[0]->value(), m_spinBoxes[1]->value(), m_spinBoxes[2]->value(), m_spinBoxes[3]->value());
            QColor minHSV, maxHSV;
            minHSV.setHsv(m_spinBoxes[4]->value(), m_spinBoxes[5]->value(), m_spinBoxes[6]->value());
            maxHSV.setHsv(m_spinBoxes[7]->value(), m_spinBoxes[8]->value(), m_spinBoxes[9]->value());

            double mean = getBrightnessMean(rect, HSVRange(minHSV,maxHSV));
            m_meanOutput->setText("Brightness Mean = " + QString::number(mean));

            m_substage = SS_Init;
            runNextStateDelay(20);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
