#ifndef VIDEOEFFECTSETTING_H
#define VIDEOEFFECTSETTING_H

#include <QWidget>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>

namespace Ui {
class VideoEffectSetting;
}

class VideoEffectSetting : public QWidget
{
    Q_OBJECT

public:
    explicit VideoEffectSetting(QWidget *parent = nullptr);
    ~VideoEffectSetting();

    void closeEvent(QCloseEvent *event);

    // value
    double getHue();
    double getSaturation();
    double getContrast();

private slots:
    void on_S_Hue_valueChanged(int value);
    void on_S_Saturation_valueChanged(int value);
    void on_S_Contrast_valueChanged(int value);
    void on_PB_Default_clicked();

signals:
    void hueChanged(double);
    void saturationChanged(double);
    void contrastChanged(double);

private:
    Ui::VideoEffectSetting *ui;
    QSettings *m_settings;
};

#endif // VIDEOEFFECTSETTING_H
