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
    }
    else
    {
        m_audioOutput = new QAudioOutput(m_audioFormat, this);
    }

    m_audioDevice = Q_NULLPTR;
    connect(this, &AudioManager::drawSignal, this, &AudioManager::drawSlot);

    // Display
    m_displayMode = ADM_None;
    m_displaySamples = 1024;
}

void AudioManager::start()
{
    m_audioDevice = m_audioOutput->start();
    QWidget::update();
}

void AudioManager::stop()
{
    m_audioOutput->stop();
    m_audioDevice = Q_NULLPTR;

    m_displayMutex.lock();
    {
        m_dataRawWave.clear();
    }
    m_displayMutex.unlock();
    QWidget::update();
}

//---------------------------------------------
// Slots
//---------------------------------------------
void AudioManager::setVolume(int volume)
{
    qDebug() << "Volume set to" << volume;
    volume = qBound(0, volume, 100);

    m_playbackMutex.lock();
    {
        if (volume == 100)
        {
            m_volumeScaleDB = 1.0;
        }
        else if (volume == 0)
        {
            m_volumeScaleDB = 0.0;
        }
        else
        {
            constexpr double exp = 20.0;
            m_volumeScaleDB = (qPow(exp, (double)volume * 0.01) - 1.0) / (exp - 1.0);
        }
    }
    m_playbackMutex.unlock();
}

void AudioManager::displayModeChanged(int index)
{
    qDebug() << "Display Mode Updated to" << index;
    m_displayMutex.lock();
    {
        m_displayMode = (AudioDisplayMode)index;

        // Wipe cached spectrogram image
        if (m_displayMode == ADM_Spectrogram)
        {
            m_displayImage.fill(Qt::black);
        }

        switch (m_displayMode)
        {
            case ADM_RawWave:
            case ADM_FreqBars:
            case ADM_Spectrogram:
            {
                this->setFixedHeight(100);
                break;
            }
            default:
            {
                this->setFixedHeight(0);
                break;
            }
        }
    }
    m_displayMutex.unlock();

    // call paintEvent
    QWidget::update();
}

void AudioManager::displaySampleChanged(int count)
{
    qDebug() << "Display Sample size changed to" << count;
    m_displayMutex.lock();
    {
        m_displaySamples = count;
        m_dataRawWave.clear();
    }
    m_displayMutex.unlock();
    QWidget::update();
}

void AudioManager::drawSlot()
{
    QWidget::update();
}

//---------------------------------------------
// Recieve data
//---------------------------------------------
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
    switch (m_displayMode)
    {
        case ADM_RawWave:
        {
            sendData_rawWave(m_audioFormat, (const char*)samples, sampleSize);
            break;
        }
        default: break;
    }
}

void AudioManager::sendData_rawWave(const QAudioFormat &format, const char* samples, size_t sampleSize)
{
    // Only support this atm...
    if (format.sampleSize() != 16 || format.sampleType() != QAudioFormat::SignedInt) return;

    // Convert raw samples to float
    QVector<float> newRawData;
    AudioConversionUtils::convertSamplesToFloat(format, samples, sampleSize, newRawData);

    m_displayMutex.lock();
    {
        int frameCount = newRawData.size() / 2;
        m_dataRawWave.clear();
        m_dataRawWave.reserve(frameCount);

        // Average LR channels
        for (int i = 0; i < frameCount; i++)
        {
            m_dataRawWave.push_back((newRawData[2*i] + newRawData[2*i+1]) * 0.5f);
        }
    }
    m_displayMutex.unlock();
    emit drawSignal();
}

//---------------------------------------------
// Drawing
//---------------------------------------------
void AudioManager::paintEvent(QPaintEvent* event)
{
    if (this->height() == 0) return;

    m_displayMutex.lock();
    {
        // Handle resize
        if (m_displayImage.size() != this->size())
        {
            m_displayImage = QImage(this->size(), QImage::Format_RGB32);
            m_displayImage.fill(Qt::black);
        }

        // Draw on image
        paintImage();
    }
    m_displayMutex.unlock();
}

void AudioManager::paintImage()
{
    int const width = this->width();
    int const height = this->height();
    float const heightHalf = height * 0.5f;

    // Paint image
    switch (m_displayMode)
    {
    case ADM_RawWave:
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        painter.setPen(QColor(Qt::cyan));

        if (m_dataRawWave.isEmpty()) return;

        QPoint lastPointPos(0, (int)heightHalf);
        if (m_displaySamples <= width)
        {
            // fewer samples than width, we need to scale it up
            float const pointWidth = (float)width / (float)m_displaySamples;
            for (int i = 0; i < m_dataRawWave.size(); i++)
            {
                float const p = m_dataRawWave[i] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(0, (int)p);
                }
                else
                {
                    int const xPos = (int)pointWidth * i;
                    QPoint newPointPos = QPoint(xPos, (int)p);
                    painter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }
        }
        else
        {

            // More samples then width, will need to ignore some
            float const sampleRatio = (float)m_displaySamples / (float)width;
            int const drawWidth = (int)((float)m_dataRawWave.size() / sampleRatio);

            m_displayImage = m_displayImage.copy(drawWidth, 0, width, height);
            QPainter imagePainter(&m_displayImage);
            imagePainter.setPen(QColor(Qt::cyan));

            int const xPosStart = width - drawWidth;
            for (int i = 0; i < drawWidth; i++)
            {
                int sampleIndex = static_cast<int>(sampleRatio * (float)i);
                if (sampleIndex >= m_dataRawWave.size()) break;

                float const p = m_dataRawWave[sampleIndex] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(xPosStart, (int)p);
                }
                else
                {
                    QPoint newPointPos = QPoint(xPosStart + i, (int)p);
                    imagePainter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }
            painter.drawImage(this->rect(), m_displayImage);

            //m_displayImage = this->grab().toImage();
        }
        break;
    }
    case ADM_FreqBars:
    {
        // TODO:
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        break;
    }
    case ADM_Spectrogram:
    {
        // TODO: Shift?
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        break;
    }
    default: break;
    }
}
