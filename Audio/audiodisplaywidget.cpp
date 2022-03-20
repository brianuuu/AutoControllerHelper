#include "audiodisplaywidget.h"

AudioDisplayWidget::AudioDisplayWidget(QWidget *parent) : QWidget(parent)
{
    m_started = false;
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
    }
    m_displayMutex.unlock();
    QWidget::update();
}

void AudioDisplayWidget::displayModeChanged(int index)
{
    qDebug() << "Display Mode Updated to" << index;
    m_displayMutex.lock();
    {
        m_mode = (AudioDisplayMode)index;
        m_image.fill(Qt::black);
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
    }
    m_displayMutex.unlock();

    // call paintEvent
    QWidget::update();
}

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

            // TODO: for spectrogram, just shift image?
            m_image.fill(Qt::black);
        }

        // Draw on image
        if (m_started)
        {
            paintImage();
        }
        else
        {
            m_image.fill(Qt::black);
        }

        // Finally draw the image on screen
        QPainter painter(this);
        painter.drawImage(this->rect(), m_image);
    }
    m_displayMutex.unlock();
}

void AudioDisplayWidget::paintImage()
{
    // Paint m_image
    switch (m_mode)
    {
    case ADM_RawWave:
    {
        m_image.fill(Qt::black);
        // TODO:
        break;
    }
    case ADM_FreqBars:
    {
        m_image.fill(Qt::black);
        // TODO:
        break;
    }
    case ADM_Spectrogram:
    {
        // TODO: Shift?
        break;
    }
    default: break;
    }

    QPainter painter(&m_image);
    painter.fillRect(this->rect(), Qt::black);
}
