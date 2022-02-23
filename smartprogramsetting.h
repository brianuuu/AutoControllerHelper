#ifndef SMARTPROGRAMSETTING_H
#define SMARTPROGRAMSETTING_H

#include "autocontrollerdefines.h"
#include "pokemondatabase.h"

#include <QDate>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QSettings>
#include <QSound>
#include <QTextStream>
#include <QWidget>

enum DateArrangement
{
    DA_JP,
    DA_EU,
    DA_US
};

namespace Ui {
class SmartProgramSetting;
}

class SmartProgramSetting : public QWidget
{
    Q_OBJECT

public:
    explicit SmartProgramSetting(QWidget *parent = nullptr);
    ~SmartProgramSetting();

    void closeEvent(QCloseEvent *event);

    // System
    bool isPreventUpdate();
    DateArrangement getDateArrangement();
    GameLanguage getGameLanguage();
    bool ensureTrainedDataExist();

    // Sound
    bool isSoundEnabled();
    void playSound();

    // Stream Counter
    bool isStreamCounterEnabled();
    bool isStreamCounterExcludePrefix();

    // Others
    bool isLogAutosave();
    bool isLogDebugCommand();
    bool isLogDebugColor();

private slots:
    // System
    void on_CB_DateArrangement_currentIndexChanged(int index);
    void on_PB_CurrentDate_clicked();
    void on_CB_GameLanguage_currentIndexChanged(int index);

    // Sound
    void on_RB_DefaultSound_clicked();
    void on_RB_CustomSound_clicked();
    void on_PB_Sound_clicked();
    void on_PB_PlaySound_clicked();

private:
    // Sound
    void SetCustomSoundEnabled();

private:
    Ui::SmartProgramSetting *ui;
    QSettings *m_settings;
    QString m_path;

    QMediaPlayer m_defaultPlayer;
    QSound* m_customSound = Q_NULLPTR;
};

#endif // SMARTPROGRAMSETTING_H
