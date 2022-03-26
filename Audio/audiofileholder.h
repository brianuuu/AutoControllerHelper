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
    bool loadWaveFile(QString const & filename, QAudioFormat const& audioFormat, float minScore, int lowFreqFilter, QString& errorStr);
    void setID(int id) { m_id = id; }

    // Getters
    int getID() { return m_id; }
    void getFrequencyRange(int& start, int& end);
    int getWindowCount() const;
    QString const& getFileName() { return m_fileName; }
    float getMinScore() { return m_minScore; }
    int& getWindowSkipCounter() { return m_windowSkipCounter; }
    QVector<SpikeIDScore> const& getSpikesCollection();

signals:

public slots:

private:
    WavFile*    m_wavFile;
    QString     m_fileName;
    int         m_id;
    int         m_freqStart;
    int         m_freqEnd;

    float       m_minScore;
    int         m_windowSkipCounter;
    QVector<SpikeIDScore> m_spikesCollection;
};

#endif // AUDIOFILEHOLDER_H
