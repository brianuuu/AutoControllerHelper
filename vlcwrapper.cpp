#include "vlcwrapper.h"

#if USE_CUSTOM_BUFFER
static void *lock(void *opaque, void **planes)
{
    struct context *ctx = (context *)opaque;
    ctx->m_mutex.lock();

    // tell VLC to put the decoded data in the buffer
    *planes = ctx->m_pixels;

    return nullptr;
}

 // get the argb image and save it to a file
static void unlock(void *opaque, void *picture, void *const *planes)
{
    Q_UNUSED(picture)

    struct context *ctx = (context *)opaque;
    unsigned char *data = (unsigned char *)*planes;

    ctx->m_frame = QImage(data, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_ARGB32);
    ctx->m_mutex.unlock();
}

static void display(void *opaque, void *picture)
{
    Q_UNUSED(picture)

    (void)opaque;
}
#endif

#if USE_CUSTOM_AUDIO
static void cbAudioPlay(void* p_audio_data, const void *samples, unsigned int count, int64_t pts)
{
    struct contextAudio *ctx = (contextAudio *)p_audio_data;
    if (!ctx->m_manager) return;

    // Pass new raw data to manager
    ctx->m_manager->pushAudioData(samples, count, pts);
}
#endif

VLCWrapper::VLCWrapper(QLabel* videoWidget, QSlider* volumeSlider, QWidget *parent)
    : QWidget(parent)
    , m_volumeSlider(volumeSlider)
    , m_videoWidget(videoWidget)
{
    const char* const vlc_args[] = {
                "--intf", "dummy",
                "--vout", "dummy",
                };
    m_instance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

    //m_instance = libvlc_new(0, nullptr);
    m_mediaPlayer = libvlc_media_player_new(m_instance);

    if (volumeSlider)
    {
        connect(volumeSlider, &QSlider::valueChanged, this, &VLCWrapper::setVolume);
    }

    m_isStarted = false;
    m_state = VLCState::NONE;
    connect(&m_timer, &QTimer::timeout, this, &VLCWrapper::timeout);

#if USE_CUSTOM_BUFFER
    ctx.m_label = m_videoWidget;
    ctx.m_pixels = new uchar[VIDEO_WIDTH * VIDEO_HEIGHT * 4];
    memset(ctx.m_pixels, 0, VIDEO_WIDTH * VIDEO_HEIGHT * 4);
#endif

#if USE_CUSTOM_AUDIO
    ctxAudio.m_manager = new AudioManager(this);
#endif

    m_defaultAreaEnable = false;
    m_defaultArea = CaptureArea(QRect(0,0,VIDEO_WIDTH,VIDEO_HEIGHT), QColor(255,0,0));
}

VLCWrapper::~VLCWrapper()
{
#if USE_CUSTOM_BUFFER
    delete[] ctx.m_pixels;
#endif

    stop();
    libvlc_media_player_release(m_mediaPlayer);
    libvlc_release(m_instance);
}

bool VLCWrapper::start(const QString &vdev, const QString &adev)
{
    if (vdev.isEmpty() && adev.isEmpty()) return false;

    m_media = libvlc_media_new_location(m_instance, "dshow://");

    // Video
    QString vdevOption = ":dshow-vdev=";
    vdevOption += vdev.isEmpty() ? "none" : vdev;
    libvlc_media_add_option(m_media, vdevOption.toStdString().c_str());

    // Audio
    QString adevOption = ":dshow-adev=";
    adevOption += adev.isEmpty() ? "none" : adev;
    libvlc_media_add_option(m_media, adevOption.toStdString().c_str());

    // Aspect ratio, resolution
    libvlc_media_add_option(m_media, ":dshow-aspect-ratio=16:9");

    // Caching
    libvlc_media_add_option(m_media, ":live-caching=0");

    // Pass to player and release media
    libvlc_media_player_set_media(m_mediaPlayer, m_media);
    libvlc_media_release(m_media);

#if USE_CUSTOM_BUFFER
    // Set the callback to extract the frame or display it on the screen
    ctx.m_frame = QImage(VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_ARGB32);
    ctx.m_frame.fill(QColor(0,0,0));
    libvlc_video_set_callbacks(m_mediaPlayer, lock, unlock, display, &ctx);
    libvlc_video_set_format(m_mediaPlayer, "BGRA", VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH * 4);
#else
    libvlc_media_player_set_hwnd(m_mediaPlayer, reinterpret_cast<HWND*>(m_videoWidget->winId()));
#endif

#if USE_CUSTOM_AUDIO
    QAudioFormat const format = ctxAudio.m_manager->getAudioFormat();
    libvlc_audio_set_callbacks(m_mediaPlayer, cbAudioPlay, nullptr, nullptr, nullptr, nullptr, &ctxAudio);
    libvlc_audio_set_format(m_mediaPlayer, "s16l", static_cast<unsigned>(format.sampleRate()), static_cast<unsigned>(format.channelCount()));
#endif

    // Play media
    int result = libvlc_media_player_play(m_mediaPlayer);
    if (result == -1)
    {
        return false;
    }
    else
    {
        // ~60fps (actual: 62.5fps)
        m_timer.start(16);
        m_isStarted = true;

#if USE_CUSTOM_AUDIO
        ctxAudio.m_manager->start();
        setVolume(m_volumeSlider ? m_volumeSlider->value() : 100);
#endif

        libvlc_video_set_adjust_int(m_mediaPlayer, libvlc_video_adjust_option_t::libvlc_adjust_Enable, true);
        return true;
    }
}

void VLCWrapper::stop()
{
    if (m_isStarted)
    {
        m_isStarted = false;
        libvlc_media_player_stop(m_mediaPlayer);

#if USE_CUSTOM_AUDIO
        ctxAudio.m_manager->stop();
#endif
    }
}

bool VLCWrapper::takeSnapshot(const QString &path)
{
    if (!isPlaying()) return false;

    // 0 on success, -1 if the video was not found
    int result = libvlc_video_take_snapshot(m_mediaPlayer, 0, path.toStdString().c_str(), 1280, 720);
    return result == 0;
}

void VLCWrapper::getFrame(QImage &frame)
{
#if USE_CUSTOM_BUFFER
    ctx.m_mutex.lock();
    frame = ctx.m_frame;
    ctx.m_mutex.unlock();
#else
    qDebug() << "NOT SUPPORTED!";
#endif
}

void VLCWrapper::setPoints(const QVector<CapturePoint> &points)
{
    m_points.clear();
    m_points = points;
}

void VLCWrapper::setAreas(QVector<CaptureArea> const& areas)
{
    m_areas.clear();
    m_areas = areas;
}

void VLCWrapper::timeout()
{
    // Update drawn frame, lock and only do copy inside
    QImage frame;
    getFrame(frame);

    // Draw overlay boxes
    QPainter painter(&frame);
    QPen pen;
    pen.setWidth(4);
    if (m_defaultAreaEnable)
    {
        pen.setColor(m_defaultArea.m_color);
        painter.setPen(pen);
        painter.drawRect(m_defaultArea.m_rect);
    }
    for (CaptureArea const& area : m_areas)
    {
        pen.setColor(area.m_color);
        painter.setPen(pen);
        painter.drawRect(area.m_rect);
    }
    for (CapturePoint const& points : m_points)
    {
        pen.setColor(points.m_color);
        painter.setPen(pen);
        painter.drawLine(points.m_point + QPoint(15,15), points.m_point + QPoint(-15,-15));
        painter.drawLine(points.m_point + QPoint(-15,15), points.m_point + QPoint(15,-15));
    }
    m_videoWidget->setPixmap(QPixmap::fromImage(frame.scaled(640, 360)));

    // Check state
    VLCState newState = static_cast<VLCState>(libvlc_media_player_get_state(m_mediaPlayer));
    if (m_state != newState)
    {
        m_state = newState;
        emit stateChanged(m_state);
    }

    // Force update volume
#if !USE_CUSTOM_AUDIO
    if (newState == VLCState::PLAYING && m_volumeSlider)
    {
        if (m_volumeSlider->value() != libvlc_audio_get_volume(m_mediaPlayer))
        {
            setVolume(m_volumeSlider->value());
        }
    }
#endif

    if (newState == VLCState::STOPPED)
    {
        m_timer.stop();
    }
}

void VLCWrapper::setVolume(int volume)
{
    if (m_isStarted)
    {
#if USE_CUSTOM_AUDIO
        ctxAudio.m_manager->setVolume(volume);
#else
        libvlc_audio_set_volume(m_mediaPlayer, volume);
#endif
    }
}

void VLCWrapper::setHue(double value)
{
    if (m_isStarted)
    {
        libvlc_video_set_adjust_float(m_mediaPlayer, libvlc_video_adjust_option_t::libvlc_adjust_Hue, (float)value);
    }
}

void VLCWrapper::setSaturation(double value)
{
    if (m_isStarted)
    {
        libvlc_video_set_adjust_float(m_mediaPlayer, libvlc_video_adjust_option_t::libvlc_adjust_Saturation, (float)value);
    }
}

void VLCWrapper::setGamma(double value)
{
    if (m_isStarted)
    {
        libvlc_video_set_adjust_float(m_mediaPlayer, libvlc_video_adjust_option_t::libvlc_adjust_Gamma, (float)value);
    }
}
