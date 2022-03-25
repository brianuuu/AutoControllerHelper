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

#include "autocontrollerdefines.h"
#include "audioconversionutils.h"
#include "audiofileholder.h"

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
    ~AudioManager() override;

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
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void drawSignal();
    void newFFTBufferDataSignal();

public slots:
    void setVolume(int volume);
    void displayModeChanged(int index);
    void displaySampleChanged(int count);
    void drawSlot();

    void freqLowChanged(int value);
    void freqHighChanged(int value);
    void newFFTBufferDataSlot();

private:
    // Raw wave
    void resetRawWaveData();
    void resetRawWaveData_NonTS();
    void writeRawWaveData(QVector<float> const& newData);

    // Spectrogram
    void resetFFTBufferData();
    void resetFFTBufferData_NonTS();
    void writeFFTBufferData(QVector<float> const& newData);

    // Display
    void paintEvent_NonTS();
    static QColor getMagnitudeColor(float v);

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
    QMutex              m_fftDataMutex;
    QVector<float>      m_fftBufferData;
    int                 m_fftNewDataStart;
    int                 m_fftAnalysisStart;

    fftwf_complex*      m_fftDataIn;
    fftwf_complex*      m_fftDataOut;
    QVector<float>      m_spectrogramData;
    int                 m_freqLow;
    int                 m_freqHigh;
};

#endif // AUDIOMANAGER_H
