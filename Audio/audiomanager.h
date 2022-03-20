#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QAudioOutput>
#include <QDebug>
#include <QMessageBox>
#include <QMutex>
#include <QObject>
#include <QWidget>
#include <QtMath>

#include "audioconversionutils.h"
#include "audiodisplaywidget.h"

class AudioManager : public QWidget
{
    Q_OBJECT
public:
    explicit AudioManager(QWidget *parent = nullptr);

    // Const functions
    bool isStarted() const { return m_audioOutput && m_audioDevice; }
    QAudioFormat const getAudioFormat() const { return m_audioFormat; }
    AudioDisplayWidget* getDisplayWidget() const { return m_displayWidget; }

    // Controls
    void start();
    void stop();

    // Input
    void pushAudioData(const void *samples, unsigned int count, int64_t pts);
    void setVolume(int volume);

signals:

public slots:

private:
    // Playback
    QMutex          m_playbackMutex;
    QAudioFormat    m_audioFormat;
    QAudioOutput*   m_audioOutput;
    QIODevice*      m_audioDevice;
    double          m_volumeScaleDB;

    // Display
    AudioDisplayWidget* m_displayWidget;
};

#endif // AUDIOMANAGER_H
