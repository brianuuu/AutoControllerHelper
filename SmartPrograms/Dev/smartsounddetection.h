#ifndef SMARTSOUNDDETECTION_H
#define SMARTSOUNDDETECTION_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartSoundDetection : public SmartProgramBase
{
public:
    explicit SmartSoundDetection(
            QString const& fileName,
            float minScore,
            int lowPassFilter,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_SoundDetection; }

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
        SS_Loop,
    };
    Substage m_substage;

    // Members
    QString m_fileName;
    float m_minScore;
    int m_lowPassFilter;
};

#endif // SMARTSOUNDDETECTION_H
