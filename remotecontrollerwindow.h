#ifndef REMOTECONTROLLERWINDOW_H
#define REMOTECONTROLLERWINDOW_H

#include "autocontrollerdefines.h"

#include <QAudioDeviceInfo>
#include <QCamera>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QColorDialog>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>
#include <QTimer>
#include <QWidget>

#include "commandsender.h"
#include "pushbuttonmapping.h"

#include "vlcwrapper.h"
#include "videoeffectsetting.h"
#include "smartprogramsetting.h"

#include "SmartPrograms/smartprogrambase.h"
#include "SmartPrograms/smartsimpleprogram.h"

#include "SmartPrograms/Dev/smartdelaycalibrator.h"
#include "SmartPrograms/Dev/smartcolorcalibrator.h"
#include "SmartPrograms/Dev/smartbrightnessmeanfinder.h"
#include "SmartPrograms/Dev/smarttestprogram.h"

#include "SmartPrograms/PokemonSwSh/smartpurplebeamfilder.h"
#include "SmartPrograms/PokemonSwSh/smartycommglitch.h"
#include "SmartPrograms/PokemonSwSh/smartsurprisetrade.h"
#include "SmartPrograms/PokemonSwSh/smartmaxraidbattler.h"
#include "SmartPrograms/PokemonSwSh/smartdayskipper.h"
#include "SmartPrograms/PokemonSwSh/smartloto.h"
#include "SmartPrograms/PokemonSwSh/smartdailyhighlight.h"
#include "SmartPrograms/PokemonSwSh/smartberryfarmer.h"
#include "SmartPrograms/PokemonSwSh/smartwattfarmer.h"

#include "SmartPrograms/PokemonBDSP/smartbdspstarter.h"
#include "SmartPrograms/PokemonBDSP/smartbdspboxoperation.h"
#include "SmartPrograms/PokemonBDSP/smartbdspshinylegendary.h"
#include "SmartPrograms/PokemonBDSP/smartbdspduplicateitem1to30.h"

#include "SmartPrograms/PokemonPLA/smartplanuggetfarmer.h"
#include "SmartPrograms/PokemonPLA/smartplaresetalphapokemon.h"
#include "SmartPrograms/PokemonPLA/smartpladistortionwaiter.h"
#include "SmartPrograms/PokemonPLA/smartplaoutbreakfinder.h"
#include "SmartPrograms/PokemonPLA/smartplapasturesorter.h"

#include "SmartPrograms/Widgets/pokemonautofilllineedit.h"

namespace Ui {
class RemoteControllerWindow;
}

class RemoteControllerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteControllerWindow(QWidget *parent = nullptr);
    ~RemoteControllerWindow();

    bool eventFilter(QObject *object, QEvent *event);
    void closeEvent (QCloseEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private slots:
    // Serial
    void on_SerialPort_readyRead();
    void on_SerialPort_errorOccurred(QSerialPort::SerialPortError error);
    void on_ReadTimer_timeout();
    void on_PB_Refresh_clicked();
    void on_PB_Connect_clicked();
    void DisplayNextCommand();
    void SerialConnectComplete();

    // Remap buttons
    void on_PB_A_clicked();
    void on_PB_B_clicked();
    void on_PB_X_clicked();
    void on_PB_Y_clicked();
    void on_PB_L_clicked();
    void on_PB_R_clicked();
    void on_PB_ZL_clicked();
    void on_PB_ZR_clicked();
    void on_PB_Plus_clicked();
    void on_PB_Minus_clicked();
    void on_PB_Home_clicked();
    void on_PB_Capture_clicked();
    void on_PB_LClick_clicked();
    void on_PB_LUp_clicked();
    void on_PB_LDown_clicked();
    void on_PB_LLeft_clicked();
    void on_PB_LRight_clicked();
    void on_PB_RClick_clicked();
    void on_PB_RUp_clicked();
    void on_PB_RDown_clicked();
    void on_PB_RLeft_clicked();
    void on_PB_RRight_clicked();
    void on_PB_DUp_clicked();
    void on_PB_DDown_clicked();
    void on_PB_DLeft_clicked();
    void on_PB_DRight_clicked();
    void on_PB_MapDefault_clicked();
    void on_PB_HideController_clicked();

    // Command sender
    void on_LE_CommandSender_textChanged(const QString &arg1);
    void on_LE_CommandSender_returnPressed();
    void on_PB_SendCommand_clicked();
    void on_PB_SaveLog_clicked();
    void on_PB_ClearLog_clicked();

    // Camera
    void on_RB_DetectedDevice_clicked();
    void on_RB_CustomDevice_clicked();
    void on_PB_RefreshCamera_clicked();
    void on_PB_StartCamera_clicked();
    void on_PB_Screenshot_clicked();
    void on_PB_AdjustVideo_clicked();
    void on_VLCState_changed(VLCWrapper::VLCState state);
    void on_CB_AudioDisplayMode_currentIndexChanged(int index);
    void on_SB_AudioFreqLow_valueChanged(int arg1);
    void on_SB_AudioFreqHigh_valueChanged(int arg1);

    // Smart program
    void on_SP1_SB_X_valueChanged(int arg1);
    void on_SP1_SB_Y_valueChanged(int arg1);
    void on_SP1_SB_Width_valueChanged(int arg1);
    void on_SP1_SB_Height_valueChanged(int arg1);

    void on_SP2_SB_X_valueChanged(int arg1);
    void on_SP2_SB_Y_valueChanged(int arg1);

    void on_SP6_CB_Skips_clicked();

    void on_SoundDetection_required(int min, int max);
    void on_SmartProgram_printLog(QString const log, QColor color);
    void on_SmartProgram_completed();
    void on_SmartProgram_runSequence(QString const sequence);
    void on_PB_StartSmartProgram_clicked();
    void on_PB_SmartSettings_clicked();
    void on_PB_ModifySmartCommands_clicked();
    void on_PB_EditStats_clicked();
    void on_PB_ResetStats_clicked();
    void EnableResetStats(bool enabled);
    void on_LW_SmartProgram_currentTextChanged(const QString &currentText);
    void on_CB_SmartProgram_currentIndexChanged(int index);

private:
    // Others
    void PrintLog(QString const& log, QColor color = QColor(0,0,0));
    void SaveLog(QString const name = "Log");
    void UpdateLogStat();
    void UpdateStatus(QString status, QColor color = QColor(0,0,0));
    void UpdateStats(SmartProgram const sp, bool reset = false);

    // Serial
    void SerialConnect(QString const& port);
    void SerialDisconnect();
    void SerialWrite(QString const& msg);

    // Button mapping
    void SaveButtonMapSettings();
    void LoadButtonMapSettings();
    void ButtonMapToggle(PushButtonMapping* button, QString name = "");
    void UpdateButtonMap(PushButtonMapping* button, int key);
    void SetButtonMapText(PushButtonMapping* button);
    void UpdateButtonFlags(int key, bool pressed);
    void ClearButtonFlags();
    void SendButtonFlag();

    // Command sender
    bool SendCommand(QString const& commands);

    // Camera
    void CameraToggle(bool on);
    void CameraCaptureToFile(QString name = "screenshot");

signals:
    void closeWindowSignal();

    // Smart program
    void commandFinished();

private:
    Ui::RemoteControllerWindow *ui;
    QSettings *m_settings;
    bool m_pauseKeyEventFilter;

    // Logging
    int m_logCount;
    int m_successCount;
    int m_warningCount;
    int m_errorCount;

    // Buttons
    typedef uint32_t ButtonFlag;
    ButtonFlag m_buttonFlag;
    PushButtonMapping* m_buttonMapChange;
    QVector<PushButtonMapping*> m_buttonMapList;
    QMap<ButtonFlag, char> m_flagToCharMap;
    QMap<PushButtonMapping*, ButtonFlag> m_PBToFlagMap;
    QMap<PushButtonMapping*, int> m_PBToKeyMap;
    QMap<int, PushButtonMapping*> m_keyToPBMap;

    static ButtonFlag const m_turboFlag = 1u << 31;

    // Commands
    QStringList m_validCommandList;
    QMap<QString, ButtonFlag> m_commandToFlagMap;
    typedef QPair<QString, uint16_t> CommandPair;
    QVector<CommandPair> m_executeCommands;
    QMutex* m_commandMutex;
    QVector<ButtonFlag> m_displayFlags;
    bool m_infiniteLoop;
    bool m_executedCommandDirty;

    // Serials
    QSerialPort m_serialPort;
    QTimer m_readTimer;
    qint64 m_readTickCount;
    uint8_t m_hexVersion;
    enum SerialState
    {
        SS_Disconnect,
        SS_TestFeedback,
        SS_FeedbackOK,
        SS_WrongVersion,
        SS_Connect
    } m_serialState;

    // VLC player
    VLCWrapper* m_vlcWrapper;

private:
    // Smart Program
    SmartProgramBase* m_smartProgram = Q_NULLPTR;
    SmartProgramSetting* m_smartSetting = Q_NULLPTR;
    VideoEffectSetting* m_videoEffectSetting = Q_NULLPTR;

    // Smart program
    void ValidateSmartProgramCommands();
    void EnableSmartProgram();
    QString GetSmartProgramName(SmartProgram sp);
    bool CanRunSmartProgram();
    void RunSmartProgram(SmartProgram sp);
    void StopSmartProgram();
    void SetEnableNonExceptionButtons(bool enabled);

    void SetCaptureAreaPos(QMouseEvent *event);

    // BrightnessMeanFinder
    QGraphicsScene* m_SP1_graphicScene;
    QGraphicsScene* m_SP1_graphicSceneMasked;

    // PurpleBeamFinder
    QGraphicsScene* m_SP2_graphicScene;
    QGraphicsScene* m_SP2_graphicSceneMasked;
};

#endif // REMOTECONTROLLERWINDOW_H
