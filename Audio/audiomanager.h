#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QAudioOutput>
#include <QDebug>
#include <QMessageBox>
#include <QMutex>
#include <QObject>
#include <QWidget>

#include "audioconversionutils.h"

class AudioManager : public QWidget
{
    Q_OBJECT
public:
    explicit AudioManager(QWidget *parent = nullptr);

    // Const functions
    bool isStarted() const { return m_audioOutput && m_audioDevice; }
    QAudioFormat const getAudioFormat() const { return m_audioFormat; }

    // Controls
    void start();
    void stop();

    // Input
    void pushAudioData(const void *samples, unsigned int count, int64_t pts);

signals:

public slots:

private:
    // Playback
    QMutex          m_playbackMutex;
    QAudioFormat    m_audioFormat;
    QAudioOutput*   m_audioOutput;
    QIODevice*      m_audioDevice;
};

#endif // AUDIOMANAGER_H
