#ifndef SELECTORWIDGET_H
#define SELECTORWIDGET_H

#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

class SelectorWidget : public QDialog
{
    Q_OBJECT
public:
    explicit SelectorWidget(QWidget* parent = nullptr);
    QPixmap selectedPixmap;
    QRect selectedRect;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap desktopPixmap;
};

#endif // SELECTORWIDGET_H
