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

#define VIDEO_WIDTH   1280
#define VIDEO_HEIGHT  720

#define USE_CUSTOM_BUFFER true
#define USE_CUSTOM_AUDIO true

#if USE_CUSTOM_BUFFER
struct context
{
    QMutex m_mutex;
    uchar *m_pixels;
    QLabel *m_label;
    QImage m_frame;
};
#endif

#if USE_CUSTOM_AUDIO
struct contextAudio
{
    QMutex m_mutex;
    AudioManager* m_manager;
    uint8_t m_volumeSet; // 0: stopped, 1: notify, 2: started

    void reset()
    {
        m_mutex.lock();
        m_volumeSet = 0;
        m_mutex.unlock();
    }
};
#endif

struct CapturePoint
{
    QPoint m_point;
    QColor m_color;

    CapturePoint(){}
    CapturePoint(QPoint point, QColor color = QColor(0,255,0))
        : m_point(point)
        , m_color(color)
    {}
    CapturePoint(int x, int y, QColor color = QColor(0,255,0))
        : m_point(QPoint(x,y))
        , m_color(color)
    {}
};

struct CaptureArea
{
    QRect m_rect;
    QColor m_color;

    CaptureArea(){}
    CaptureArea(QRect rect, QColor color = QColor(0,255,0))
        : m_rect(rect)
        , m_color(color)
    {}
    CaptureArea(int x, int y, int width, int height, QColor color = QColor(0,255,0))
        : m_rect(QRect(x,y,width,height))
        , m_color(color)
    {}
};

class VLCWrapper : public QWidget
{
    Q_OBJECT

public:
    VLCWrapper(QLabel* videoWidget, QSlider* volumeSlider = nullptr, QWidget *parent = nullptr);
    ~VLCWrapper();

    // General
    bool isStarted() { return m_isStarted; }

    // Camera
    bool start(QString const& vdev, QString const& adev);
    void stop();
    bool takeSnapshot(QString const& path);
    void getFrame(QImage& frame);

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

    void setDefaultAreaEnabled(bool enabled) { m_defaultAreaEnable = enabled; }
    void setDefaultArea(QRect const rect) { m_defaultArea.m_rect = rect; }

    void clearCaptures() { m_points.clear(); m_areas.clear(); }
    void clearPoints() { m_points.clear(); }
    void clearAreas() { m_areas.clear(); }
    void setPoints(QVector<CapturePoint> const& points);
    void setAreas(QVector<CaptureArea> const& areas);

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
    libvlc_instance_t* m_instance;
    libvlc_media_player_t* m_mediaPlayer;
    libvlc_media_t* m_media;

#if USE_CUSTOM_BUFFER
    // Custom buffer context
    struct context ctx;
#endif

    // Qt Members
    bool m_isStarted;
    QImage m_frame;
    QTimer m_timer;
    VLCState m_state;
    QSlider* m_volumeSlider;
    QLabel* m_videoWidget;

#if USE_CUSTOM_AUDIO
    struct contextAudio ctxAudio;
#endif

    // Capture areas/points
    bool m_defaultAreaEnable;
    CaptureArea m_defaultArea;
    QVector<CaptureArea> m_areas;
    QVector<CapturePoint> m_points;
};

#endif // VLCWRAPPER_H
