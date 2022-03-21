#ifndef VLCWRAPPER_H
#define VLCWRAPPER_H

#include <vlc/vlc.h>

#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QWidget>
#include <QMutex>

#include "Audio/audiomanager.h"
#include "Video/videomanager.h"

struct contextVideo
{
    QMutex m_mutex;
    uchar *m_pixels;

    VideoManager* m_manager;
};

struct contextAudio
{
    AudioManager* m_manager;
};

class VLCWrapper : public QWidget
{
    Q_OBJECT

public:
    VLCWrapper(QWidget *parent = nullptr);
    ~VLCWrapper();

    // General
    bool isStarted() { return m_isStarted; }
    VideoManager* getVideoManager() const { return ctxVideo.m_manager; }
    AudioManager* getAudioManager() const { return ctxAudio.m_manager; }

    // Controls
    bool start(QString const& vdev, QString const& adev);
    void stop();
    bool takeSnapshot(QString const& path);

    // State
    enum VLCState
    {
        NONE,
        OPENING,
        BUFFERING,
        PLAYING,
        PAUSED,
        STOPPED,
        ENDED,
        ERROR
    };
    VLCState getState() { return m_state; }
    bool isPlaying() { return m_state == VLCState::PLAYING; }

private slots:
    void timeout();
    void setVolume(int volume);

public slots:
    void setHue(double value);
    void setSaturation(double value);
    void setGamma(double value);

signals:
    void stateChanged(VLCState);

private:
    // LibVLC
    libvlc_instance_t*      m_instance;
    libvlc_media_player_t*  m_mediaPlayer;
    libvlc_media_t*         m_media;

    // Custom video/audio context
    struct contextVideo ctxVideo;
    struct contextAudio ctxAudio;

    // Qt Members
    bool m_isStarted;
    VLCState m_state;
};

#endif // VLCWRAPPER_H
