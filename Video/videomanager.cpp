#include "videomanager.h"

VideoManager::VideoManager(QWidget *parent) : QWidget(parent)
{
    this->setFixedSize(640,360);
    connect(&m_displayTimer, &QTimer::timeout, this, &VideoManager::displayTimeout);

    m_isUseFixedImage = false;
    m_defaultAreaEnable = false;
    m_defaultArea = CaptureArea(QRect(0,0,VIDEO_WIDTH,VIDEO_HEIGHT), QColor(255,0,0));
    stop();
}

void VideoManager::start()
{
    m_displayMutex.lock();
    {
        m_isStarted = true;

        m_displayImage = QImage(VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_ARGB32);
        m_displayImage.fill(Qt::black);
        m_fixedImage = m_displayImage;

        // ~60fps (actual: 62.5fps)
        m_displayTimer.start(16);
    }
    m_displayMutex.unlock();
    this->show();
}

void VideoManager::stop()
{
    m_displayMutex.lock();
    {
        m_isStarted = false;
        m_displayTimer.stop();
    }
    m_displayMutex.unlock();
    this->hide();
}

void VideoManager::setFixedImage(const QImage &fixedImage)
{
    m_displayMutex.lock();
    {
        m_fixedImage = fixedImage;
    }
    m_displayMutex.unlock();
}

void VideoManager::setFixedImageUsed(bool used)
{
    m_displayMutex.lock();
    {
        m_isUseFixedImage = used;
    }
    m_displayMutex.unlock();
}

void VideoManager::displayTimeout()
{
    emit timeout();
    QWidget::update();
}

//---------------------------------------------
// Input
//---------------------------------------------
void VideoManager::pushVideoData(const unsigned char *data)
{
    m_displayMutex.lock();
    {
        m_displayImage = m_isUseFixedImage ? m_fixedImage : QImage(data, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_ARGB32);
    }
    m_displayMutex.unlock();

    // We don't immediately update, let timeout do it
}

//---------------------------------------------
// Output
//---------------------------------------------
void VideoManager::getFrame(QImage &frame)
{
    m_displayMutex.lock();
    {
        frame = m_displayImage;
    }
    m_displayMutex.unlock();
}

//---------------------------------------------
// Drawing
//---------------------------------------------
void VideoManager::paintEvent(QPaintEvent *event)
{
    QImage imageScaled;
    m_displayMutex.lock();
    {
        imageScaled = m_displayImage.scaled(640, 360);
    }
    m_displayMutex.unlock();

    // Draw current frame
    QPainter painter(this);
    painter.drawImage(this->rect(), imageScaled);

    // Draw overlay boxes
    QPen pen;
    pen.setWidth(2);
    if (m_defaultAreaEnable)
    {
        pen.setColor(m_defaultArea.m_color);
        painter.setPen(pen);
        painter.drawRect(getRectScaled(m_defaultArea.m_rect));
    }

    for (CaptureArea const& area : m_areas)
    {
        pen.setColor(area.m_color);
        painter.setPen(pen);
        painter.drawRect(getRectScaled(area.m_rect));
    }

    for (CapturePoint const& points : m_points)
    {
        pen.setColor(points.m_color);
        painter.setPen(pen);
        painter.drawLine(points.m_point / 2 + QPoint(7,7), points.m_point / 2 + QPoint(-7,-7));
        painter.drawLine(points.m_point / 2 + QPoint(-7,7), points.m_point / 2 + QPoint(7,-7));
    }
}
