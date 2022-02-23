#ifndef SMARTPROGRAMSETTING_H
#define SMARTPROGRAMSETTING_H

#include "autocontrollerdefines.h"

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

enum GameLanguage : uint8_t
{
    GL_English = 0,
    GL_ChineseSimplified,
    GL_ChineseTraditional,
    GL_French,
    GL_German,
    GL_Italian,
    GL_Japanese,
    GL_Korean,
    GL_Spanish
};

namespace Ui {
class SmartProgramSetting;
}

class SmartProgramSetting : public QWidget
{
    Q_OBJECT

public:
    static QString getGameLanguagePrefix(GameLanguage sp)
    {
        switch (sp)
        {
        case GL_English:            return "eng";
        case GL_ChineseSimplified:  return "chi_sim";
        case GL_ChineseTraditional: return "chi_tra";
        case GL_French:             return "fra";
        case GL_German:             return "deu";
        case GL_Italian:            return "ita";
        case GL_Japanese:           return "jpn";
        case GL_Korean:             return "kor";
        case GL_Spanish:            return "spa";
        }
        return "invalid";
    }

    static QString getGameLanguageName(GameLanguage sp)
    {
        switch (sp)
        {
        case GL_English:            return "English";
        case GL_ChineseSimplified:  return "Chinese (Simplified)";
        case GL_ChineseTraditional: return "Chinese (Traditional)";
        case GL_French:             return "French";
        case GL_German:             return "German";
        case GL_Italian:            return "Italian";
        case GL_Japanese:           return "Japanese";
        case GL_Korean:             return "Korean";
        case GL_Spanish:            return "Spanish";
        }
        return "Unknown Language";
    }

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
