#ifndef PUSHBUTTONMAPPING_H
#define PUSHBUTTONMAPPING_H

#include <QPushButton>
#include <QTimer>

class PushButtonMapping : public QPushButton
{
    Q_OBJECT
public:
    explicit PushButtonMapping(QWidget *parent = nullptr);

    void ButtonMapStart();
    void ButtonMapFinish();

    void ButtonTurbo();
    void ButtonPressed();
    void ButtonReleased();

signals:

private slots:
    void ToggleTurbo();

private:

    QTimer m_turboTimer;
    bool m_turboPressed;
};

#endif // PUSHBUTTONMAPPING_H
