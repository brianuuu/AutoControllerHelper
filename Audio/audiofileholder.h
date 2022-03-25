#ifndef AUDIOFILEHOLDER_H
#define AUDIOFILEHOLDER_H

#include <QAudioFormat>
#include <QDebug>
#include <QFile>
#include <QObject>

#include "autocontrollerdefines.h"
#include "audioconversionutils.h"
#include "peakfinder.h"
#include "wavfile.h"

// This class is used to load .wav file and ultimately used for sound dection
class AudioFileHolder : public QObject
{
    Q_OBJECT
public:
    explicit AudioFileHolder(QObject *parent = nullptr);
    ~AudioFileHolder();

    // Interface
    bool loadWaveFile(QString const & filename, QAudioFormat const& audioFormat, int lowFreqFilter, QString& errorStr);

signals:

public slots:

private:
    WavFile*    m_wavFile;
    QString     m_fileName;
    float       m_freqStart;
    float       m_freqEnd;
};

#endif // AUDIOFILEHOLDER_H
