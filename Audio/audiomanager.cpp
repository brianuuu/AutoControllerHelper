#include "audiomanager.h"

AudioManager::AudioManager(QWidget *parent) : QWidget(parent)
{
    // Set up display widget
    m_displayWidget = new AudioDisplayWidget(this);

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
    }
    else
    {
        m_audioOutput = new QAudioOutput(m_audioFormat, this);
    }

    m_audioDevice = Q_NULLPTR;
}

void AudioManager::start()
{
    m_audioDevice = m_audioOutput->start();
    m_displayWidget->start();
}

void AudioManager::stop()
{
    m_audioOutput->stop();
    m_audioDevice = Q_NULLPTR;
    m_displayWidget->stop();
}

void AudioManager::pushAudioData(const void *samples, unsigned int count, int64_t pts)
{
    if (!isStarted()) return;

    size_t sampleSize = count * m_audioFormat.bytesPerFrame();

    // Playback
    m_playbackMutex.lock();
    if (m_volumeScaleDB == 1.0)
    {
        // Don't convert anything
        m_audioDevice->write((const char*)samples, sampleSize);
    }
    else if (m_volumeScaleDB > 0.0)
    {
        // Scale the audio, unfortunately we have to assume the data type here...
        int16_t* sampleScaled = new int16_t[sampleSize / sizeof(int16_t)];
        memcpy(sampleScaled, samples, sampleSize);
        for (size_t i = 0; i < sampleSize / sizeof(int16_t); i++)
        {
            sampleScaled[i] = static_cast<int16_t>((double)sampleScaled[i] * m_volumeScaleDB);
        }
        m_audioDevice->write((const char*)sampleScaled, sampleSize);
        delete[] sampleScaled;
    }
    m_playbackMutex.unlock();

    // TODO: Processing
    if (m_displayWidget->getDisplayMode() == ADM_RawWave)
    {
        m_displayWidget->sendData_rawWave(m_audioFormat, (const char*)samples, sampleSize);
    }
}

void AudioManager::setVolume(int volume)
{
    volume = qBound(0, volume, 100);

    if (volume == 100)
    {
        m_volumeScaleDB = 1.0;
        return;
    }

    if (volume == 0)
    {
        m_volumeScaleDB = 0.0;
        return;
    }

    constexpr double exp = 20.0;
    m_volumeScaleDB = (qPow(exp, (double)volume * 0.01) - 1.0) / (exp - 1.0);
}
