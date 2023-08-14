#ifndef AUTOCONTROLLERWINDOW_H
#define AUTOCONTROLLERWINDOW_H

#include "autocontrollerdefines.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDirIterator>
#include <QDomDocument>
#include <QFileDialog>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QUrl>

#include "remotecontrollerwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class autocontrollerwindow; }
QT_END_NAMESPACE

class autocontrollerwindow : public QMainWindow
{
    Q_OBJECT

public:
    autocontrollerwindow(QWidget *parent = nullptr);
    ~autocontrollerwindow();

    void closeEvent (QCloseEvent *event);

private slots:
    void test();
    void on_PB_Source_clicked();
    void on_PB_Manual_clicked();
    void on_PB_Generate_clicked();
    void on_PB_Log_clicked();
    void on_PB_Unsync_clicked();
    void on_LW_Bots_currentTextChanged(const QString &currentText);
    void on_Debug_dimension();

    void on_NetworkManager_Finished(QNetworkReply *reply);

    void on_CompileErrorOccurred(QProcess::ProcessError error);
    void on_CompileOutputReady();
    void on_CompileErrorReady();
    void on_CompileFinished();

    void on_CB_Bots_currentIndexChanged(int index);

    void on_DaySkipper_DateArrangement_currentIndexChanged(int index);
    void on_DaySkipper_DaysToSkip_valueChanged(int arg1);
    void on_DaySkipper_Date_dateChanged(const QDate &date);
    void on_DaySkipper_CurrentDate_clicked();

    void on_DaySkipper_Unlimited_DateArrangement_currentIndexChanged(int index);
    void on_DaySkipper_Unlimited_DaysToSkip_valueChanged(int arg1);

    void on_FriendDeleteAdd_Count_valueChanged(int arg1);

    void on_AutoLoto_DateArrangement_currentIndexChanged(int index);
    void on_AutoLoto_CheckBox_stateChanged(int arg1);
    void on_AutoLoto_DaysToSkip_valueChanged(int arg1);

    void on_Generic1_Count_valueChanged(int arg1);

    void on_AutoFossil_First_currentIndexChanged(int index);
    void on_AutoFossil_Second_currentIndexChanged(int index);
    void on_AutoFossil_Count_valueChanged(int arg1);
    void on_AutoFossil_SR_stateChanged(int arg1);

    void on_Auto3DaySkipper_DateArrangement_currentIndexChanged(int index);

    void on_BoxSurpriseTrade_Count_valueChanged(int arg1);
    void on_BoxSurpriseTrade_CompleteDex_stateChanged(int arg1);

    void on_AutoHost_DateArrangement_currentIndexChanged(int index);
    void on_AutoHost_SkipDays_stateChanged(int arg1);
    void on_AutoHost_UseLinkCode_stateChanged(int arg1);
    void on_AutoHost_Fixed_clicked();
    void on_AutoHost_LinkCode_textChanged(const QString &arg1);
    void on_AutoHost_Random_clicked();
    void on_AutoHost_WaitTime_currentIndexChanged(int index);
    void on_AutoHost_AddFriends_stateChanged(int arg1);
    void on_AutoHost_Profile_valueChanged(int arg1);
    void on_AutoHost_NoSR_stateChanged(int arg1);
    void on_AutoHost_ProfileWhat_clicked();
    void on_AutoHost_Local_clicked();
    void on_AutoHost_Online_clicked();

    void on_EggHatcher_Pokemon_currentIndexChanged(const QString &arg1);
    void on_EggHatcher_Column_valueChanged(int arg1);

    void on_ShinyRegi_Type_currentIndexChanged(int index);
    void on_ShinyRegi_Slow_clicked();
    void on_ShinyRegi_Fast_clicked();
    void on_ShinyRegi_Aware_clicked();
    void on_ShinyRegi_Time_valueChanged(int arg1);

    void on_ShinySword_Slow_clicked();
    void on_ShinySword_Fast_clicked();
    void on_ShinySword_Aware_clicked();
    void on_ShinySword_Time_valueChanged(int arg1);

    void on_Farmer_Count_valueChanged(int arg1);
    void on_Farmer_DateArrangement_currentIndexChanged(int index);
    void on_Farmer_Save_valueChanged(int arg1);

    void on_ShinyRegigigas_SR_clicked();
    void on_ShinyRegigigas_Aware_clicked();
    void on_ShinyRegigigas_Time_valueChanged(int arg1);
    void on_ShinyRegigigas_Berry_valueChanged(int arg1);

    void on_RemoteControl_Tool_clicked();

    void on_BDSPBoxOperation_Count_valueChanged(int arg1);
    void on_BDSPBoxOperation_Type_currentIndexChanged(int index);

    void on_SVEggCollector_Count_valueChanged(int arg1);

    void on_SVEggHatcher_Cycle_currentIndexChanged(int index);
    void on_SVEggHatcher_Column_valueChanged(int arg1);

private:
    QString GetVariableString(QString const& _str);
    void LoadConfig();
    void SaveConfig();

    void CheckVersion();

    // Information
    void UpdateInstruction();
    void UpdateInfo();
    QString GetTimeString(QString const& _bot, int _iter, QString const& _botExtra = "");

private:
    Ui::autocontrollerwindow *ui;
    QSettings *m_settings;
    RemoteControllerWindow* m_remoteController = Q_NULLPTR;

    enum Program : int
    {
        // SwSh Programs
        P_DaySkipper = 0,
        P_DaySkipper_Unlimited,
        P_WattFarmer,
        P_FriendDeleteAdd,
        P_BerryFarmer,
        P_AutoLoto,
        P_BoxRelease,
        P_AutoFossil,
        P_AutoFossil_GR,
        P_Auto3DaySkipper,
        P_BoxSurpriseTrade,
        P_AutoHost,
        P_EggCollector,
        P_EggCollector_IT,
        P_EggHatcher,
        P_GodEggDuplication,
        P_GodEggDuplication_JP,
        P_ShinyFiveRegi,
        P_ShinySwordTrio,
        P_DailyHighlightFarmer,
        P_ShinyRegigigas,
        P_SmartProgram,

        P_TurboA,
        P_AutoBattleTower,
        P_AutoTournament,
        P_PurpleBeamFinder,

        // BDSP Programs
        P_BDSP_ResetDialgaPalkia,
        P_BDSP_ResetStarter,
        P_BDSP_MenuGlitch113,
        P_BDSP_BoxDuplication,
        P_BDSP_BoxOperation,
        P_BDSP_ResetLegendary,
        P_BDSP_ResetShaymin,
        P_BDSP_ResetArceus,

        // PLA Programs
        P_PLA_ResetAlphaCrobat,
        P_PLA_ResetAlphaGallade,

        // SV Programs
        P_SV_ItemDuplication,
        P_SV_EggCollector,
        P_SV_EggHatcher,
        P_SV_BoxRelease,
        P_SV_GimmighoulFarmer,

        // TOTK Programs
        P_TOTK_BowFuseDuplication,
        P_TOTK_MineruDuplication,
        P_TOTK_ShieldSurfDuplication,
        P_TOTK_ZonaiDeviceDuplication,

        P_INVALID
    };
    QMap<QString, Program> m_programEnumMap;

    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;

    bool m_validProgram;
    QMap<Program, int> m_tabID;

    QMap<QString, int> m_eggCycles;

    // Compile process
    QProcess m_process;
    QString m_output;
    QString m_error;

    // Program times in milliseconds
    QMap<QString, int> m_times;

    // Quick instruction
    bool m_instructionLoaded;
    QDomDocument m_instructions;
    QVector<bool> m_checked;
};
#endif // AUTOCONTROLLERWINDOW_H
