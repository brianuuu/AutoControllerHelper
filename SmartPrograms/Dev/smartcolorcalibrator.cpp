#include "smartcolorcalibrator.h"

SmartColorCalibrator::SmartColorCalibrator
(
    QLabel* labelBattle,
    QLabel* labelPokemon,
    QLabel* labelBag,
    QLabel* labelRun,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_battle(labelBattle)
    , m_pokemon(labelPokemon)
    , m_bag(labelBag)
    , m_run(labelRun)
{
    init();
}

void SmartColorCalibrator::init()
{
    SmartProgramBase::init();
}

void SmartColorCalibrator::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartColorCalibrator::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Capture;
        m_parameters.vlcWrapper->clearPoints();
        m_parameters.vlcWrapper->setPoints({P_Battle,P_Pokemon,P_Bag,P_Run});
        setState_frameAnalyzeRequest();
        break;
    }
    case SS_Wait:
    {
        m_substage = SS_Capture;
        setState_frameAnalyzeRequest();
        break;
    }
    case SS_Capture:
    {
        if (state == S_CaptureReady)
        {
            testColor(P_Battle, C_Color_Battle, m_battle);
            testColor(P_Pokemon, C_Color_Pokemon, m_pokemon);
            testColor(P_Bag, C_Color_Bag, m_bag);
            testColor(P_Run, C_Color_Run, m_run);

            m_substage = SS_Init;
            runNextStateDelay(20);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartColorCalibrator::testColor(const CapturePoint &cp, const SmartProgramBase::HSVRange &range, QLabel *label)
{
    QColor color = m_capture.pixelColor(cp.m_point);
    QColor hsv = color.toHsv();
    QString textColor = "FFFF0000";
    if (checkColorMatchHSV(color, range))
    {
        textColor = "FF00AA00";
    }

    label->setText("<font color=\"#" + textColor + "\">" + QString::number(hsv.hue()) + "," + QString::number(hsv.saturation()) + "," + QString::number(hsv.value()) + "</font>");
}
