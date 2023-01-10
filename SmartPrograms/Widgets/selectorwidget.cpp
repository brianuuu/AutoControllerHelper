#include "selectorwidget.h"

SelectorWidget::SelectorWidget(QWidget* parent) : QDialog(parent, Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    desktopPixmap = QGuiApplication::primaryScreen()->grabWindow(0);
    setGeometry(desktopPixmap.rect());

    /*QRect fullRect(0,0,0,0);
    for (QScreen* screen : QGuiApplication::screens())
    {
        QPoint pos = screen->geometry().topLeft();
        QSize size = screen->grabWindow(0).size();
        QRect rect(pos, size);

        fullRect.setX(qMin(fullRect.x(), pos.x()));
        fullRect.setY(qMin(fullRect.y(), pos.y()));
        fullRect.setRight(qMax(fullRect.right(), rect.right()));
        fullRect.setBottom(qMax(fullRect.bottom(), rect.bottom()));
    }
    setGeometry(fullRect);

    desktopPixmap = QPixmap(geometry().size());
    QPainter p(&desktopPixmap);
    p.setWindow(geometry());

    for (QScreen* screen : QGuiApplication::screens())
    {
        QPixmap window = screen->grabWindow(0);
        QPoint pos = screen->geometry().topLeft();
        QRect rect(pos, window.size());
        p.drawPixmap(rect, window, window.rect());
    }*/
}

void SelectorWidget::mousePressEvent(QMouseEvent* event)
{
    selectedRect.setTopLeft(event->globalPos());
}

void SelectorWidget::mouseMoveEvent(QMouseEvent* event)
{
    selectedRect.setBottomRight(event->globalPos());
    update();
}

void SelectorWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // fix the selected rect that may caused by HDPI fuckery
    qreal ratio = this->devicePixelRatio();
    selectedRect = QRect
    (
        static_cast<int>(selectedRect.x() * ratio),
        static_cast<int>(selectedRect.y() * ratio),
        static_cast<int>(selectedRect.width() * ratio),
        static_cast<int>(selectedRect.height() * ratio)
    );
    selectedPixmap = desktopPixmap.copy(selectedRect);
    accept();
}

void SelectorWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawPixmap(geometry().topLeft(), desktopPixmap);

    QPainterPath path;
    path.addRect(rect());
    path.addRect(selectedRect);
    p.fillPath(path, QColor::fromRgb(255,255,255,200));

    p.setPen(Qt::red);
    p.drawRect(selectedRect);
}
