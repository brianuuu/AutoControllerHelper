#include "audiodisplaywidget.h"

AudioDisplayWidget::AudioDisplayWidget(QWidget *parent) : QWidget(parent)
{
    m_started = false;
    m_mode = ADM_None;

    m_displaySamples = 512;
}

void AudioDisplayWidget::start()
{
    m_displayMutex.lock();
    {
        m_started = true;
    }
    m_displayMutex.unlock();
    QWidget::update();
}

void AudioDisplayWidget::stop()
{
    m_displayMutex.lock();
    {
        m_started = false;
        m_dataRawWave.clear();
    }
    m_displayMutex.unlock();
    QWidget::update();
}

//---------------------------------------------
// Recieve data
//---------------------------------------------
void AudioDisplayWidget::sendData_rawWave(const QAudioFormat &format, const char* samples, size_t sampleSize)
{
    // Only support this atm...
    if (format.sampleSize() != 16 || format.sampleType() != QAudioFormat::SignedInt) return;

    bool update = false;
    m_displayMutex.lock();
    {
        // Convert raw samples to float
        QVector<float> newRawData;
        AudioConversionUtils::convertSamplesToFloat(format, samples, sampleSize, newRawData);

        int frameCount = newRawData.size() / 2;
        if (m_displaySamples <= this->width())
        {
            // Always wipe data if we can't display all samples
            m_dataRawWave.clear();
            m_dataRawWave.reserve(frameCount);
        }
        else
        {
            // Data is full, wipe them and start from beginning
            if (m_dataRawWave.size() >= m_displaySamples)
            {
                m_dataRawWave.clear();
                m_dataRawWave.reserve(frameCount);
            }
        }

        // Average LR channels
        for (int i = 0; i < frameCount; i++)
        {
            m_dataRawWave.push_back((newRawData[2*i] + newRawData[2*i+1]) * 0.5f);
        }

        // Draw when we have enough samples
        update = (m_dataRawWave.size() >= m_displaySamples);
    }
    m_displayMutex.unlock();

    if (update)
    {
        QWidget::update();
    }
}

//---------------------------------------------
// Signals
//---------------------------------------------
void AudioDisplayWidget::displayModeChanged(int index)
{
    qDebug() << "Display Mode Updated to" << index;
    m_displayMutex.lock();
    {
        m_mode = (AudioDisplayMode)index;
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);

        switch (m_mode)
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

        // Wipe cached spectrogram image
        if (m_mode == ADM_Spectrogram)
        {
            m_image.fill(Qt::black);
        }
    }
    m_displayMutex.unlock();

    // call paintEvent
    QWidget::update();
}

void AudioDisplayWidget::displaySampleChanged(int count)
{
    qDebug() << "Display Sample size changed to" << count;

    bool update = false;
    m_displayMutex.lock();
    {
        m_displaySamples = count;
        update = (m_mode == ADM_RawWave);
    }
    m_displayMutex.unlock();

    if (update)
    {
        QWidget::update();
    }
}

//---------------------------------------------
// Drawing
//---------------------------------------------
void AudioDisplayWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    if (this->height() == 0)
    {
        return;
    }

    m_displayMutex.lock();
    {
        // Handle resize
        if (m_image.size() != this->size())
        {
            m_image = QImage(this->size(), QImage::Format_RGB32);
            m_image.fill(Qt::black);
        }

        // Draw on image
        paintImage();
    }
    m_displayMutex.unlock();
}

void AudioDisplayWidget::paintImage()
{
    float const width = static_cast<float>(this->width());
    float const height = static_cast<float>(this->height());
    float const heightHalf = height * 0.5f;

    // Paint image
    switch (m_mode)
    {
    case ADM_RawWave:
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        painter.setPen(QColor(Qt::cyan));

        if (m_dataRawWave.isEmpty()) return;

        QPoint lastPointPos(0, (int)heightHalf);
        if (m_displaySamples < width)
        {
            // fewer samples than width, we need to scale it up
            float const pointWidth = width / (float)m_displaySamples;
            for (int i = 0; i < m_dataRawWave.size(); i++)
            {
                float const p = m_dataRawWave[i] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(0, (int)p);
                }
                else
                {
                    float const xPos = pointWidth * (float)i;
                    QPoint newPointPos = QPoint((int)xPos, (int)p);
                    painter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }
        }
        else
        {
            // More samples then width, will need to ignore some
            float const sampleRatio = (float)m_dataRawWave.size() / (float)width;
            for (int i = 0; i < width; i++)
            {
                int sampleIndex = static_cast<int>(sampleRatio * (float)i);
                float const p = m_dataRawWave[sampleIndex] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(0, (int)p);
                }
                else
                {
                    QPoint newPointPos = QPoint(i, (int)p);
                    painter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }
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
