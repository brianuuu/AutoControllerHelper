#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QAudioOutput>
#include <QDebug>
#include <QMessageBox>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QWidget>
#include <QtMath>

#include "audioconversionutils.h"

enum AudioDisplayMode : uint8_t
{
    ADM_None,
    ADM_RawWave,
    ADM_FreqBars,
    ADM_Spectrogram,

    ADM_COUNT
};

class AudioManager : public QWidget
{
    Q_OBJECT
public:
    explicit AudioManager(QWidget *parent = nullptr);

    // Const functions
    bool isStarted() const { return m_audioOutput && m_audioDevice; }
    QAudioFormat const getAudioFormat() const { return m_audioFormat; }
    AudioDisplayMode getDisplayMode() { return m_displayMode; }

    // Controls
    void start();
    void stop();

    // Input
    void pushAudioData(const void *samples, unsigned int count, int64_t pts);

protected:
    virtual void paintEvent(QPaintEvent* event) override;

signals:
    void drawSignal();

public slots:
    void setVolume(int volume);
    void displayModeChanged(int index);
    void displaySampleChanged(int count);
    void drawSlot();

private:
    // Recieve data
    void sendData_rawWave(QAudioFormat const& format, const char* samples, size_t sampleSize);

    // Display
    void paintImage();

private:
    // Playback
    QMutex          m_playbackMutex;
    QAudioFormat    m_audioFormat;
    QAudioOutput*   m_audioOutput;
    QIODevice*      m_audioDevice;
    double          m_volumeScaleDB;

    // Display
    QMutex              m_displayMutex;
    AudioDisplayMode    m_displayMode;
    QImage              m_displayImage;

    // Raw Wave data
    int                 m_displaySamples;
    QVector<float>      m_rawWaveData;
    int                 m_rawWaveDataSize;

    // Spectrogram data
};

#endif // AUDIOMANAGER_H
