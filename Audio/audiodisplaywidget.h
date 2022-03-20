#ifndef AUDIODISPLAYWIDGET_H
#define AUDIODISPLAYWIDGET_H

#include <QAudioFormat>
#include <QDebug>
#include <QPainter>
#include <QMutex>
#include <QWidget>

#include "audioconversionutils.h"

enum AudioDisplayMode : uint8_t
{
    ADM_None,
    ADM_RawWave,
    ADM_FreqBars,
    ADM_Spectrogram,

    ADM_COUNT
};

class AudioDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioDisplayWidget(QWidget *parent = nullptr);

    // Enable dispaly
    void start();
    void stop();

    // Const functions
    AudioDisplayMode getDisplayMode() { return m_mode; }

    // Recieve data
    void sendData_rawWave(QAudioFormat const& format, const char* samples, size_t sampleSize);

protected:
    virtual void paintEvent(QPaintEvent* event);

signals:

public slots:
    void displayModeChanged(int index);
    void displaySampleChanged(int count);

private:
    void paintImage();

private:
    QMutex              m_displayMutex;
    bool                m_started;
    AudioDisplayMode    m_mode;

    // Raw Wave data
    int                 m_displaySamples;
    QVector<float>      m_dataRawWave;

    // Spectrogram data
    QImage              m_image;
};

#endif // AUDIODISPLAYWIDGET_H
