#include "pushbuttonmapping.h"

PushButtonMapping::PushButtonMapping(QWidget *parent) : QPushButton(parent)
{
    connect(&m_turboTimer, &QTimer::timeout, this, &PushButtonMapping::ToggleTurbo);
    m_turboPressed = false;
    ButtonMapFinish();
}

void PushButtonMapping::ButtonMapStart()
{
    this->setStyleSheet("background-color: rgb(255,220,0); font-size: 18px;");
}

void PushButtonMapping::ButtonMapFinish()
{
    this->setStyleSheet("background-color: rgb(255,255,255); font-size: 18px;");
}

void PushButtonMapping::ButtonTurbo()
{
    // 1 tick = 48.05ms
    m_turboTimer.start(48);
    m_turboPressed = false;
}

void PushButtonMapping::ButtonPressed()
{
    this->setStyleSheet("background-color: rgb(255,80,80); font-size: 18px;");
}

void PushButtonMapping::ButtonReleased()
{
    ButtonMapFinish();
    m_turboTimer.stop();
}

void PushButtonMapping::ToggleTurbo()
{
    if (m_turboPressed)
    {
        ButtonMapFinish();
    }
    else
    {
        ButtonPressed();
    }

    m_turboPressed = !m_turboPressed;
}


