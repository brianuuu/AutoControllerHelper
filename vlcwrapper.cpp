#include "vlcwrapper.h"

static void* cbVideoLock(void *opaque, void **planes)
{
    struct contextVideo *ctx = (contextVideo *)opaque;
    ctx->m_mutex.lock();

    // tell VLC to put the decoded data in the buffer
    *planes = ctx->m_pixels;
    return nullptr;
}

 // get the argb image and save it to a file
static void cbVideoUnlock(void *opaque, void *picture, void *const *planes)
{
    struct contextVideo *ctx = (contextVideo *)opaque;
    unsigned char const* data = (unsigned char const*)*planes;

    ctx->m_manager->pushVideoData(data);
    ctx->m_mutex.unlock();
}

static void cbAudioPlay(void* p_audio_data, const void *samples, unsigned int count, int64_t pts)
{
    struct contextAudio *ctx = (contextAudio *)p_audio_data;
    if (!ctx->m_manager) return;

    // Pass new raw data to manager
    ctx->m_manager->pushAudioData(samples, count, pts);
}

VLCWrapper::VLCWrapper(QWidget *parent) : QWidget(parent)
{
    const char* const vlc_args[] = {
                "--intf", "dummy",
                "--vout", "dummy",
                };
    m_instance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

    //m_instance = libvlc_new(0, nullptr);
    m_mediaPlayer = libvlc_media_player_new(m_instance);

    m_isStarted = false;
    m_state = VLCState::NONE;

    // Video
    ctxVideo.m_manager = new VideoManager(this);
    ctxVideo.m_pixels = new uchar[VIDEO_WIDTH * VIDEO_HEIGHT * 4];
    memset(ctxVideo.m_pixels, 0, VIDEO_WIDTH * VIDEO_HEIGHT * 4);
    connect(ctxVideo.m_manager, &VideoManager::timeout, this, &VLCWrapper::timeout);

    // Audio
    ctxAudio.m_manager = new AudioManager(this);
}

VLCWrapper::~VLCWrapper()
{
    delete[] ctxVideo.m_pixels;

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

    // Set the callback to extract the frame or display it on the screen
    libvlc_video_set_callbacks(m_mediaPlayer, cbVideoLock, cbVideoUnlock, nullptr, &ctxVideo);
    libvlc_video_set_format(m_mediaPlayer, "BGRA", VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH * 4);

    // Set callback to extract raw PCM data
    QAudioFormat const format = ctxAudio.m_manager->getAudioFormat();
    libvlc_audio_set_callbacks(m_mediaPlayer, cbAudioPlay, nullptr, nullptr, nullptr, nullptr, &ctxAudio);
    libvlc_audio_set_format(m_mediaPlayer, "s16l", format.sampleRate(), format.channelCount());

    // Play media
    int result = libvlc_media_player_play(m_mediaPlayer);
    if (result == -1)
    {
        return false;
    }
    else
    {
        m_isStarted = true;

        // Start video & audio playback
        ctxVideo.m_manager->start();
        ctxAudio.m_manager->start();

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

        // Stop display
        ctxVideo.m_manager->stop();
        ctxAudio.m_manager->stop();
    }
}

bool VLCWrapper::takeSnapshot(const QString &path)
{
    if (!isPlaying()) return false;

    // 0 on success, -1 if the video was not found
    int result = libvlc_video_take_snapshot(m_mediaPlayer, 0, path.toStdString().c_str(), 1280, 720);
    return result == 0;
}

void VLCWrapper::timeout()
{
    // Check state
    VLCState newState = static_cast<VLCState>(libvlc_media_player_get_state(m_mediaPlayer));
    if (m_state != newState)
    {
        m_state = newState;
        emit stateChanged(m_state);
    }
}

void VLCWrapper::setVolume(int volume)
{
    if (m_isStarted)
    {
        ctxAudio.m_manager->setVolume(volume);
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
