#include "videoeffectsetting.h"
#include "ui_videoeffectsetting.h"

VideoEffectSetting::VideoEffectSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoEffectSetting)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    m_settings = new QSettings("brianuuu", "AutoControllerHelper", this);
    ui->S_Hue->setValue(m_settings->value("VideoHue", 0).toInt());
    ui->S_Saturation->setValue(m_settings->value("VideoSaturation", 100).toInt());
    ui->S_Contrast->setValue(m_settings->value("VideoContrast", 100).toInt());
}

VideoEffectSetting::~VideoEffectSetting()
{
    delete ui;
}

void VideoEffectSetting::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("VideoHue", ui->S_Hue->value());
    m_settings->setValue("VideoSaturation", ui->S_Saturation->value());
    m_settings->setValue("VideoContrast", ui->S_Contrast->value());
}

double VideoEffectSetting::getHue()
{
    return (double)ui->S_Hue->value() / 100;
}

double VideoEffectSetting::getSaturation()
{
    return (double)ui->S_Saturation->value() / 100;
}

double VideoEffectSetting::getContrast()
{
    return (double)ui->S_Contrast->value() / 100;
}

void VideoEffectSetting::on_S_Hue_valueChanged(int value)
{
    double d = (double)value / 100;
    emit hueChanged(d);

    QString s;
    s.setNum(d, 'f', 2);
    ui->L_Hue->setText(s);
}

void VideoEffectSetting::on_S_Saturation_valueChanged(int value)
{
    double d = (double)value / 100;
    emit saturationChanged(d);

    QString s;
    s.setNum(d, 'f', 2);
    ui->L_Saturation->setText(s);
}

void VideoEffectSetting::on_S_Contrast_valueChanged(int value)
{
    double d = (double)value / 100;
    emit contrastChanged(d);

    QString s;
    s.setNum(d, 'f', 2);
    ui->L_Contrast->setText(s);
}

void VideoEffectSetting::on_PB_Default_clicked()
{
    ui->S_Hue->setValue(0);
    ui->S_Saturation->setValue(100);
    ui->S_Contrast->setValue(100);
}
