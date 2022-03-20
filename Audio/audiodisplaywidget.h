#ifndef AUDIODISPLAYWIDGET_H
#define AUDIODISPLAYWIDGET_H

#include <QDebug>
#include <QPainter>
#include <QMutex>
#include <QWidget>

enum AudioDisplayMode : uint8_t
{
    ADM_None,
    ADM_RawWave,
    ADM_FreqBars,
    ADM_Spectrogram,

    ADM_COUNT
};

class AudioDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioDisplayWidget(QWidget *parent = nullptr);

    void start();
    void stop();

protected:
    virtual void paintEvent(QPaintEvent* event);

signals:

public slots:
    void displayModeChanged(int index);

private:
    void paintImage();

private:
    QMutex              m_displayMutex;
    bool                m_started;
    AudioDisplayMode    m_mode;
    QImage              m_image;
};

#endif // AUDIODISPLAYWIDGET_H
