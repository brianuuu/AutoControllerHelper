#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QAudioOutput>
#include <QDebug>
#include <QMessageBox>
#include <QMutex>
#include <QObject>
#include <QWidget>

class AudioManager : public QWidget
{
    Q_OBJECT
public:
    explicit AudioManager(QWidget *parent = nullptr);

    // Const functions
    bool isInitialized() const { return m_audioOutput; }
    QAudioFormat const getAudioFormat() const { return m_audioFormat; }

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
