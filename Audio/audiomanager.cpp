#include "audiomanager.h"

AudioManager::AudioManager(QWidget *parent) : QWidget(parent)
{
    // Set up global audio format
    m_audioFormat.setSampleRate(48000);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_audioFormat))
    {
        QMessageBox::warning(this, "Warning", "Raw audio format not supported by backend, cannot play audio.", QMessageBox::Ok);
        m_audioOutput = Q_NULLPTR;
        m_audioDevice = Q_NULLPTR;
    }
    else
    {
        m_audioOutput = new QAudioOutput(m_audioFormat, this);
        m_audioDevice = m_audioOutput->start();
    }
}

void AudioManager::pushAudioData(const void *samples, unsigned int count, int64_t pts)
{
    if (!isInitialized()) return;

    // Playback
    m_playbackMutex.lock();
    m_audioDevice->write((const char*)samples, count * sizeof(int16_t) * 2);
    m_playbackMutex.unlock();

    // TODO: Processing
}
