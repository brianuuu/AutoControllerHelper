#include "smartsounddetection.h"

SmartSoundDetection::SmartSoundDetection
(
    const QString &fileName,
    float minScore,
    int lowPassFilter,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_fileName(fileName)
    , m_minScore(minScore)
    , m_lowPassFilter(lowPassFilter)
{
    init();
}

void SmartSoundDetection::init()
{
    SmartProgramBase::init();
}

void SmartSoundDetection::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartSoundDetection::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        int id = m_audioManager->addDetection(m_fileName, m_minScore, m_lowPassFilter);
        if (id == 0)
        {
            setState_error("Error occured loading sound file");
        }
        else
        {
            m_audioManager->startDetection(id);
            m_substage = SS_Loop;
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_Loop:
    {
        break;
    }
    }

    SmartProgramBase::runNextState();
}
