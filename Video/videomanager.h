#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QDebug>
#include <QTimer>
#include <QMutex>
#include <QPainter>
#include <QWidget>

#include "autocontrollerdefines.h"

#define VIDEO_WIDTH   1280
#define VIDEO_HEIGHT  720

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

class VideoManager : public QWidget
{
    Q_OBJECT
public:
    explicit VideoManager(QWidget *parent = nullptr);

    // Const functions
    bool isStarted() const { return m_isStarted; }

    // Controls
    void start();
    void stop();

    // Input
    void pushVideoData(unsigned char const* data);

    // Output
    void getFrame(QImage& frame);

    // Capture Areas
    void setDefaultAreaEnabled(bool enabled) { m_defaultAreaEnable = enabled; }
    void setDefaultArea(QRect const rect) { m_defaultArea.m_rect = rect; }

    void clearCaptures() { m_points.clear(); m_areas.clear(); }
    void clearPoints() { m_points.clear(); }
    void clearAreas() { m_areas.clear(); }
    void setPoints(QVector<CapturePoint> const& points) { m_points.clear(); m_points = points; }
    void setAreas(QVector<CaptureArea> const& areas) { m_areas.clear(); m_areas = areas; }
    QRect getRectScaled(QRect const& rect) { return QRect(rect.topLeft() / 2, rect.size() / 2); }

protected:
    virtual void paintEvent(QPaintEvent* event) override;

signals:
    void printLog(QString const log, QColor color = QColor(0,0,0));
    void timeout();

public slots:

private slots:
    void displayTimeout();

private:
    // Display
    bool    m_isStarted;
    QMutex  m_displayMutex;
    QTimer  m_displayTimer;
    QImage  m_displayImage;

    // Capture areas/points
    bool        m_defaultAreaEnable;
    CaptureArea m_defaultArea;
    QVector<CaptureArea>    m_areas;
    QVector<CapturePoint>   m_points;
};

#endif // VIDEOMANAGER_H
