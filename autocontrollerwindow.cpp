#include "autocontrollerwindow.h"
#include "ui_autocontrollerwindow.h"

static const QString c_version = "5.0.1";

//---------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------
autocontrollerwindow::autocontrollerwindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::autocontrollerwindow)
{
    ui->setupUi(this);
    //this->layout()->setSizeConstraint(QLayout::SetMinimumSize);

    // Program
    m_validProgram = false;
    m_programEnumMap["DaySkipper"]              = P_DaySkipper;
    m_programEnumMap["DaySkipper_Unlimited"]    = P_DaySkipper_Unlimited;
    m_programEnumMap["WattFarmer"]              = P_WattFarmer;
    m_programEnumMap["FriendDeleteAdd"]         = P_FriendDeleteAdd;
    m_programEnumMap["BerryFarmer"]             = P_BerryFarmer;
    m_programEnumMap["AutoLoto"]                = P_AutoLoto;
    m_programEnumMap["BoxRelease"]              = P_BoxRelease;
    m_programEnumMap["AutoFossil"]              = P_AutoFossil;
    m_programEnumMap["AutoFossil_GR"]           = P_AutoFossil_GR;
    m_programEnumMap["Auto3DaySkipper"]         = P_Auto3DaySkipper;
    m_programEnumMap["BoxSurpriseTrade"]        = P_BoxSurpriseTrade;
    m_programEnumMap["AutoHost"]                = P_AutoHost;
    m_programEnumMap["EggCollector"]            = P_EggCollector;
    m_programEnumMap["EggCollector_IT"]         = P_EggCollector_IT;
    m_programEnumMap["EggHatcher"]              = P_EggHatcher;
    m_programEnumMap["GodEggDuplication"]       = P_GodEggDuplication;
    m_programEnumMap["ShinyFiveRegi"]           = P_ShinyFiveRegi;
    m_programEnumMap["ShinySwordTrio"]          = P_ShinySwordTrio;
    m_programEnumMap["DailyHighlightFarmer"]    = P_DailyHighlightFarmer;
    m_programEnumMap["ShinyRegigigas"]          = P_ShinyRegigigas;
    m_programEnumMap["Others_SmartProgram"]     = P_SmartProgram;
    m_programEnumMap["Others_TurboA"]           = P_TurboA;
    m_programEnumMap["AutoBattleTower"]         = P_AutoBattleTower;
    m_programEnumMap["AutoTournament"]          = P_AutoTournament;
    m_programEnumMap["PurpleBeamFinder"]        = P_PurpleBeamFinder;

    m_programEnumMap["BDSP_ResetDialgaPalkia"]  = P_BDSP_ResetDialgaPalkia;
    m_programEnumMap["BDSP_ResetStarter"]       = P_BDSP_ResetStarter;
    m_programEnumMap["BDSP_MenuGlitch113"]      = P_BDSP_MenuGlitch113;
    m_programEnumMap["BDSP_BoxDuplication"]     = P_BDSP_BoxDuplication;
    m_programEnumMap["BDSP_BoxOperation"]       = P_BDSP_BoxOperation;

    m_tabID[P_DaySkipper]           = 1;
    m_tabID[P_DaySkipper_Unlimited] = 2;
    m_tabID[P_WattFarmer]           = 17; // 3 now use for error
    m_tabID[P_FriendDeleteAdd]      = 4;
    m_tabID[P_BerryFarmer]          = 17; // 5 deprecated
    m_tabID[P_AutoLoto]             = 6;
    m_tabID[P_BoxRelease]           = 7;
    m_tabID[P_AutoFossil]           = 8;
    m_tabID[P_AutoFossil_GR]        = 8;
    m_tabID[P_Auto3DaySkipper]      = 9;
    m_tabID[P_BoxSurpriseTrade]     = 10;
    m_tabID[P_AutoHost]             = 11;
    m_tabID[P_EggCollector]         = 12;
    m_tabID[P_EggCollector_IT]      = 12;
    m_tabID[P_EggHatcher]           = 13;
    m_tabID[P_GodEggDuplication]    = 14;
    m_tabID[P_ShinyFiveRegi]        = 15;
    m_tabID[P_ShinySwordTrio]       = 16;
    m_tabID[P_DailyHighlightFarmer] = 17;
    m_tabID[P_ShinyRegigigas]       = 18;
    m_tabID[P_SmartProgram]         = 19;

    m_tabID[P_TurboA]               = 0;
    m_tabID[P_AutoBattleTower]      = 0;
    m_tabID[P_AutoTournament]       = 0;
    m_tabID[P_PurpleBeamFinder]     = 0;

    m_tabID[P_BDSP_ResetDialgaPalkia]   = 0;
    m_tabID[P_BDSP_ResetStarter]        = 20;
    m_tabID[P_BDSP_MenuGlitch113]       = 0;
    m_tabID[P_BDSP_BoxDuplication]      = 21;
    m_tabID[P_BDSP_BoxOperation]        = 22;

    if (!QDir(HEX_PATH).exists())
    {
        QDir().mkdir(HEX_PATH);
    }

    // Load all timings from .xml file
    QDomDocument document;
    QFile file(QString(BOT_PATH) + "Times.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Fail to load Times.xml file!", QMessageBox::Ok);
    }
    else
    {
        document.setContent(&file);
        file.close();

        QDomNodeList times = document.firstChildElement().childNodes();
        for (int i = 0; i < times.count(); i++)
        {
            QDomElement element = times.at(i).toElement();
            m_times[element.tagName()] = element.text().toInt();
        }
    }

    // Load instruction checklist
    QFile instructionFile(QString(BOT_PATH) + "Instructions.xml");
    if (!instructionFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Fail to load Instructions.xml file!", QMessageBox::Ok);
        m_instructionLoaded = false;
    }
    else
    {
        m_instructions.setContent(&instructionFile);
        instructionFile.close();
        m_instructionLoaded = true;
    }

    // Load all available program names
    QDirIterator it(BOT_PATH);
    while (it.hasNext())
    {
        QString dir = it.next();
        QString program = dir.mid(dir.lastIndexOf('/') + 1);
        if (!program.contains('.'))
        {
            QChar const start = program[0];
            bool startWithAlphabet = (start >= 'a' && start <= 'z') || (start >= 'A' && start <= 'Z');
            if (program.contains(" ") || !startWithAlphabet)
            {
                QString warning = "Bots \"" + program + "\" cannot contain spaces and must start with an alphabet, please change the folder and .c file name and restart this program.";
                QMessageBox::warning(this, "Warning", warning, QMessageBox::Ok);
            }
            else
            {
                ui->LW_Bots->addItem(program);
            }
        }
    }

    // Load all pokemon egg cycle group
    QFile eggCycleFile(QString(BOT_PATH) + "EggHatcher/EggCycles.txt");
    if (eggCycleFile.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QTextStream in(&eggCycleFile);
        while (!in.atEnd())
        {
            QStringList str = in.readLine().split(',');
            if (str.size() == 2)
            {
                m_eggCycles[str[0]] = str[1].toInt();
                ui->EggHatcher_Pokemon->addItem(str[0]);
            }
        }

        eggCycleFile.close();
    }

    // Load previous settings
    m_settings = new QSettings("brianuuu", "AutoControllerHelper", this);

    // Load bots for current game
    ui->TW_Bots->blockSignals(true);
    ui->TW_Bots->setCurrentIndex(m_settings->value("GameIndex", 0).toInt());
    ui->TW_Bots->blockSignals(false);
    on_TW_Bots_currentChanged(ui->TW_Bots->currentIndex());

    QString const savedProgram = m_settings->value("Bots", "DaySkipper").toString();
    for (int i = 0; i < ui->LW_Bots->count(); i++)
    {
        if (ui->LW_Bots->item(i)->text() == savedProgram)
        {
            ui->LW_Bots->setCurrentRow(i);
            break;
        }
    }
    ui->CB_MCU->setCurrentIndex(m_settings->value("MCU", 0).toInt());

    // Compile signals
    connect(&m_process, &QProcess::errorOccurred, this, &autocontrollerwindow::on_CompileErrorOccurred);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &autocontrollerwindow::on_CompileOutputReady);
    connect(&m_process, &QProcess::readyReadStandardError, this, &autocontrollerwindow::on_CompileErrorReady);
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_CompileFinished()));

    QTime time = QTime::currentTime();
    qsrand(static_cast<uint>(time.msec()));
    ui->AutoHost_LinkCode->setValidator( new QIntValidator(0, 99999999, this) );

    // Version
    this->setWindowTitle(this->windowTitle() + " v" + c_version);
    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &autocontrollerwindow::on_NetworkManager_Finished);
    CheckVersion();

    // Bind extra shortcuts
    QShortcut *applyChanges = new QShortcut(QKeySequence("Ctrl+H"), this);
    connect(applyChanges, SIGNAL(activated()), this, SLOT(on_Debug_dimension()));
}

//---------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------
autocontrollerwindow::~autocontrollerwindow()
{
    m_settings->setValue("GameIndex", ui->TW_Bots->currentIndex());
    if (ui->LW_Bots->currentRow() != -1)
    {
        m_settings->setValue("Bots", ui->LW_Bots->currentItem()->text());
    }
    m_settings->setValue("MCU", ui->CB_MCU->currentIndex());
    delete ui;
}

//---------------------------------------------------------------------------
// Closing application
//---------------------------------------------------------------------------
void autocontrollerwindow::closeEvent(QCloseEvent *event)
{
    if (m_remoteController)
    {
        m_remoteController->close();
    }
    event->accept();
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void autocontrollerwindow::test()
{

}

void autocontrollerwindow::on_PB_Source_clicked()
{
    QString sourceFolder = QString(BOT_PATH) + ui->LW_Bots->currentItem()->text();
    QDesktopServices::openUrl(QUrl::fromLocalFile(sourceFolder));
}

void autocontrollerwindow::on_PB_Manual_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(ROOT_PATH + QString("AutoController_Manual.pdf")));
}

void autocontrollerwindow::on_PB_Log_clicked()
{
    QString logFile = QString(SOURCE_PATH) + "/output.log";
    if (QFile::exists(logFile))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logFile));
    }
}

void autocontrollerwindow::on_PB_Unsync_clicked()
{
    QString str = "Pokemon games only allows one controller, to disconnect the current controller, ";
    str += "press the sync button or unplug any wired controller, then plug the Arduino/Teensy to the Switch.";
    QMessageBox::information(this, "What is this?", str, QMessageBox::Ok);
}

void autocontrollerwindow::on_PB_Generate_clicked()
{
    ui->PB_Generate->setText("Compiling...");
    ui->PB_Generate->setEnabled(false);
    ui->PB_Log->setEnabled(false);
    ui->LW_Bots->setEnabled(false);
    ui->CB_MCU->setEnabled(false);
    qApp->processEvents();

    SaveConfig();

    QString name = ui->LW_Bots->currentItem()->text();

    // Edit makefile
    QString contents;
    QString makefile = QString(SOURCE_PATH) + "makefile";

    QFile ifile(makefile);
    if (!ifile.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "makefile missing or failed to read!", QMessageBox::Ok);
        return;
    }
    QTextStream istream(&ifile);
    contents = istream.readAll();
    ifile.close();

    // Replace MCU
    {
        int MCUIndex = contents.indexOf("MCU          = ");
        int targetIndex = contents.indexOf('=', MCUIndex);
        int endLineIndex = contents.indexOf("\n", MCUIndex);
        QString currentTarget = contents.mid(targetIndex, endLineIndex - targetIndex);
        switch (ui->CB_MCU->currentIndex())
        {
        case 0:
            contents.replace(currentTarget, "= atmega16u2");
            break;
        case 1:
            contents.replace(currentTarget, "= atmega32u4");
            break;
        case 2:
            contents.replace(currentTarget, "= at90usb1286");
            break;
        }
    }

    // Replace TARGET
    {
        int targetIndex = contents.indexOf("= ./Bots/") + 2;
        int endLineIndex = contents.indexOf("\n", targetIndex);
        QString currentTarget = contents.mid(targetIndex, endLineIndex - targetIndex);
        contents.replace(currentTarget, QString("./Bots/") + name + "/" + name);
    }

    QFile ofile(makefile);
    if (ofile.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        QTextStream ostream(&ofile);
        ostream << contents;
    }
    ofile.close();

    // Delete the .hex file in the hex directory
    QString hexFileName = name + QString(".hex");
    QFile::remove(QString(HEX_PATH) + hexFileName);

    // Run cmd "make"
    m_output.clear();
    m_error.clear();
    m_process.setWorkingDirectory(SOURCE_PATH);
    m_process.start("make");
}

//---------------------------------------------------------------------------
// Switch program
//---------------------------------------------------------------------------
void autocontrollerwindow::on_CB_Bots_currentIndexChanged(const QString &arg1)
{
    LoadConfig();
}

void autocontrollerwindow::on_LW_Bots_currentTextChanged(const QString &currentText)
{
    LoadConfig();
}

//---------------------------------------------------------------------------
// Hidden debug window dimension
//---------------------------------------------------------------------------
void autocontrollerwindow::on_Debug_dimension()
{
    qDebug() << this->size();
}

//---------------------------------------------------------------------------
// HTTP Request read version
//---------------------------------------------------------------------------
void autocontrollerwindow::CheckVersion()
{
    ui->L_Update->setText("Checking for Update...");
    m_networkRequest.setUrl(QUrl("https://raw.githubusercontent.com/brianuuu/AutoController_swsh/main/UpdateVersion.ini"));
    m_networkManager->get(m_networkRequest);
}

//---------------------------------------------------------------------------
// Read version online
//---------------------------------------------------------------------------
void autocontrollerwindow::on_NetworkManager_Finished(QNetworkReply *reply)
{
    QString link = "https://github.com/brianuuu/AutoController_swsh/releases";

    if (reply->error())
    {
        QString errorDebug;
        //errorDebug += "\nBuild Ver: " + QSslSocket::sslLibraryBuildVersionString();
        //errorDebug += QString("\nOpenSSL Suported: ") + (QSslSocket::supportsSsl() ? "true" : "false");
        //errorDebug += "\nRuntime Ver: " + QSslSocket::sslLibraryVersionString();

        QString message = "Update check failed: " + reply->errorString() + errorDebug;
        message += "\nDo you want to check download page for newest version?";

        QMessageBox::StandardButton resBtn = QMessageBox::Yes;
        resBtn = QMessageBox::question(this, "Error", message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (resBtn == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl(link));
        }

        ui->L_Update->setText("<html><head/><body><p>Update check failed: <a href=\"" + link + "\"><span style=\" text-decoration: underline; color:#0000ff;\">Download Page</span></a>!</p></body></html>");
        return;
    }

    QString answer = reply->readAll();

    int verStart = answer.indexOf("Version=\"") + 9;
    int verEnd = answer.indexOf('\"', verStart);
    QString newVersion = answer.mid(verStart, verEnd - verStart);

    QStringList newVerNo = newVersion.split('.');
    QStringList curVerNo = c_version.split('.');

    bool outdated = false;
    for (int i = 0; i < newVerNo.size(); i++)
    {
        if (newVerNo[i] > curVerNo[i])
        {
            outdated = true;
            break;
        }
        else if (curVerNo[i] > newVerNo[i])
        {
            // Program is newer than github, forgot to update on github?
            qDebug() << "Github ver: " + newVersion;
            ui->L_Update->setText("Github not commited.");
            return;
        }
    }

    if (outdated)
    {
        int changeStart = answer.indexOf("ChangeLog=\"") + 11;
        int changeEnd = answer.indexOf('\"', changeStart);
        QString changeLog = answer.mid(changeStart, changeEnd - changeStart);

        QMessageBox::StandardButton resBtn = QMessageBox::Yes;
        QString message = "New version " + newVersion + " available, do you wish to download it?";
        message += "\n\nChange Log:\n" + changeLog;
        resBtn = QMessageBox::question(this, "Update", message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (resBtn == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl(link));
            this->close();
        }
        else
        {
            ui->L_Update->setText("<html><head/><body><p>Update available <a href=\"" + link + "\"><span style=\" text-decoration: underline; color:#0000ff;\">here</span></a>!</p></body></html>");
        }
    }
    else
    {
        ui->L_Update->setText("Program up to date!");
    }

}

//---------------------------------------------------------------------------
// Compile program
//---------------------------------------------------------------------------
void autocontrollerwindow::on_CompileErrorOccurred(QProcess::ProcessError error)
{
    QMessageBox::critical(this, "Error", "Compilation failed to start, please ensure you have installed WinAVR before using this program.", QMessageBox::Ok);

    ui->PB_Generate->setText("Save && Generate HEX file");
    ui->PB_Generate->setEnabled(true);
    ui->PB_Log->setEnabled(true);
    ui->LW_Bots->setEnabled(true);
    ui->CB_MCU->setEnabled(true);
}

void autocontrollerwindow::on_CompileOutputReady()
{
    QByteArray data = m_process.readAllStandardOutput();
    m_output.append(QString::fromStdString(data.toStdString()));
    float gccCount = m_output.count("[GCC]");
    ui->progressBar->setValue(static_cast<int>(gccCount * 100 /17.0f));
}

void autocontrollerwindow::on_CompileErrorReady()
{
    QByteArray data = m_process.readAllStandardError();
    m_error.append(QString::fromStdString(data.toStdString()));
}

void autocontrollerwindow::on_CompileFinished()
{
    ui->PB_Generate->setText("Save && Generate HEX file");

    // Output log
    QFile logFile(QString(SOURCE_PATH) + "/output.log");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream out(&logFile);
    out << m_output;

    QString name = ui->LW_Bots->currentItem()->text();

    QString hexFileName = name + QString(".hex");
    QString originalName = QString(BOT_PATH) + name + "/" + hexFileName;
    if (QFile::exists(originalName))
    {
        logFile.close();
        ui->progressBar->setValue(100);
        qApp->processEvents();

        // Move hex file to hex folder
        QFile::rename(originalName, QString(HEX_PATH) + hexFileName);

        // Delete everything that is not .c or .h or .txt or .xml file
        QDirIterator it(QString(BOT_PATH) + name);
        while (it.hasNext())
        {
            QString dir = it.next();
            QString file = dir.mid(dir.lastIndexOf('/') + 1);
            if (file.endsWith(".d") || file.endsWith(".elf") || file.endsWith(".map") || file.endsWith(".o"))
            {
                QFile::remove(dir);
            }
        }

        QMessageBox::information(this, "Success", hexFileName + " generated at Hex directory!", QMessageBox::Ok);
    }
    else
    {
        out << m_error;
        logFile.close();
        qApp->processEvents();

        // Delete everything that is not .c or .h file
        QDirIterator it(QString(BOT_PATH) + name);
        while (it.hasNext())
        {
            QString dir = it.next();
            QString file = dir.mid(dir.lastIndexOf('/') + 1);
            if (file != "." && file != ".." && !file.endsWith(".h") && !file.endsWith(".c") && !file.endsWith(".txt"))
            {
                QFile::remove(dir);
            }
        }

        QMessageBox::warning(this, "Failed", "Build failed, please check compile log.", QMessageBox::Ok);
    }

    ui->PB_Generate->setEnabled(true);
    ui->PB_Log->setEnabled(true);
    ui->LW_Bots->setEnabled(true);
    ui->CB_MCU->setEnabled(true);
}

//---------------------------------------------------------------------------
// Change game
//---------------------------------------------------------------------------
void autocontrollerwindow::on_TW_Bots_currentChanged(int index)
{
    for (int row = 0; row < ui->LW_Bots->count(); ++row)
    {
        QListWidgetItem* item = ui->LW_Bots->item(row);
        if (item->text().startsWith("Others"))
        {
            item->setHidden(ui->TW_Bots->tabText(index) != "Others");
        }
        else if (item->text().startsWith("BDSP"))
        {
            item->setHidden(ui->TW_Bots->tabText(index) != "BDSP");
        }
        else
        {
            item->setHidden(index != 0);
        }
    }

    // Force set to first bot in the list
    for (int row = 0; row < ui->LW_Bots->count(); ++row)
    {
        QListWidgetItem const* item = ui->LW_Bots->item(row);
        if (!item->isHidden())
        {
            ui->LW_Bots->setCurrentRow(row);
            break;
        }
    }
}

//---------------------------------------------------------------------------
// DaySkipper signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_DaySkipper_DaysToSkip_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_DaySkipper_CurrentDate_clicked()
{
    ui->DaySkipper_Date->setDate(QDate::currentDate());
}

void autocontrollerwindow::on_DaySkipper_Date_dateChanged(const QDate &date)
{
    UpdateInfo();
}

void autocontrollerwindow::on_DaySkipper_DateArrangement_currentIndexChanged(int index)
{
    if (index == 0)
    {
        // JP
        ui->DaySkipper_Date->setDisplayFormat("yyyy/MM/dd");
    }
    else if (index == 1)
    {
        // EU
        ui->DaySkipper_Date->setDisplayFormat("dd/MM/yyyy");
    }
    else
    {
        // US
        ui->DaySkipper_Date->setDisplayFormat("MM/dd/yyyy");
    }

    UpdateInfo();
}

//---------------------------------------------------------------------------
// DaySkipper_Unlimited signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_DaySkipper_Unlimited_DateArrangement_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_DaySkipper_Unlimited_DaysToSkip_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// FriendDeleteAdd signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_FriendDeleteAdd_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// AutoLoto signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_AutoLoto_DateArrangement_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoLoto_CheckBox_stateChanged(int arg1)
{
    ui->AutoLoto_DaysToSkip->setEnabled(ui->AutoLoto_CheckBox->isChecked());
    UpdateInfo();
}

void autocontrollerwindow::on_AutoLoto_DaysToSkip_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// BoxRelease signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_BoxRelease_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// AutoFossil signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_AutoFossil_First_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoFossil_Second_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoFossil_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoFossil_SR_stateChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// Auto3DaySkipper signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_Auto3DaySkipper_DateArrangement_currentIndexChanged(int index)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// BoxSurpriseTrade signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_BoxSurpriseTrade_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_BoxSurpriseTrade_CompleteDex_stateChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// AutoHost signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_AutoHost_DateArrangement_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_SkipDays_stateChanged(int arg1)
{
    bool enabled = ui->AutoHost_SkipDays->isChecked();
    ui->AutoHost_DateArrangement->setEnabled(enabled);
    ui->AutoHost_NoSR->setEnabled(!enabled);
    if (enabled)
    {
        ui->AutoHost_NoSR->setChecked(false);
    }
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_UseLinkCode_stateChanged(int arg1)
{
    bool enabled = ui->AutoHost_UseLinkCode->isChecked();
    ui->AutoHost_Fixed->setEnabled(enabled);
    ui->AutoHost_LinkCode->setEnabled(ui->AutoHost_UseLinkCode->isChecked() && ui->AutoHost_Fixed->isChecked());
    ui->AutoHost_Random->setEnabled(enabled);
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_Fixed_clicked()
{
    ui->AutoHost_LinkCode->setEnabled(ui->AutoHost_UseLinkCode->isChecked() && ui->AutoHost_Fixed->isChecked());
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_LinkCode_textChanged(const QString &arg1)
{
    QColor color;
    if (arg1.size() != 8)
    {
        color = QColor(255,60,60);
    }
    else
    {
        color = QColor(255,255,255);
    }
    QPalette pal = palette();
    pal.setColor(QPalette::Base, color);
    ui->AutoHost_LinkCode->setPalette(pal);
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_Random_clicked()
{
    ui->AutoHost_LinkCode->setEnabled(ui->AutoHost_UseLinkCode->isChecked() && ui->AutoHost_Fixed->isChecked());
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_WaitTime_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_AddFriends_stateChanged(int arg1)
{
    ui->AutoHost_Profile->setEnabled(ui->AutoHost_AddFriends->isChecked());
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_Profile_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_NoSR_stateChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_ProfileWhat_clicked()
{
    QMessageBox::information(this, "What is this?", "When you press HOME, the profiles on the top,\nchoose the one you wish to add friend.", QMessageBox::Ok);
}

void autocontrollerwindow::on_AutoHost_Local_clicked()
{
    ui->AutoHost_AddFriends->setEnabled(false);
    ui->AutoHost_AddFriends->setChecked(false);
    ui->AutoHost_Internet->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_AutoHost_Online_clicked()
{
    ui->AutoHost_AddFriends->setEnabled(true);
    ui->AutoHost_Internet->setEnabled(true);
    UpdateInfo();
}

//---------------------------------------------------------------------------
// EggCollector signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_EggCollector_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// EggHatcher signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_EggHatcher_Pokemon_currentIndexChanged(const QString &arg1)
{
    QString pokemon = ui->EggHatcher_Pokemon->currentText();
    QString str = m_eggCycles.contains(pokemon) ? QString::number(m_eggCycles[pokemon]) : "Unknown";
    ui->EggHatcher_Cycle->setText(str);
    UpdateInfo();
}

void autocontrollerwindow::on_EggHatcher_Column_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// GodEggDuplication signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_GodEgg_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// ShinyFiveRegi signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_ShinyRegi_Type_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegi_Slow_clicked()
{
    ui->ShinyRegi_Time->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegi_Fast_clicked()
{
    ui->ShinyRegi_Time->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegi_Aware_clicked()
{
    ui->ShinyRegi_Time->setEnabled(true);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegi_Time_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// ShinySwordTrio signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_ShinySword_Slow_clicked()
{
    ui->ShinySword_Time->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinySword_Fast_clicked()
{
    ui->ShinySword_Time->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinySword_Aware_clicked()
{
    ui->ShinySword_Time->setEnabled(true);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinySword_Time_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// Generic Farmer signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_Farmer_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_Farmer_DateArrangement_currentIndexChanged(int index)
{
    UpdateInfo();
}

void autocontrollerwindow::on_Farmer_Save_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// Shiny Regigigas signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_ShinyRegigigas_SR_clicked()
{
    ui->ShinyRegigigas_Berry->setEnabled(false);
    ui->ShinyRegigigas_Time->setEnabled(false);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegigigas_Aware_clicked()
{
    ui->ShinyRegigigas_Berry->setEnabled(true);
    ui->ShinyRegigigas_Time->setEnabled(true);
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegigigas_Time_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_ShinyRegigigas_Berry_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// Remote Control signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_RemoteControl_Tool_clicked()
{
    if (m_remoteController && !m_remoteController->isVisible())
    {
        delete m_remoteController;
        m_remoteController = Q_NULLPTR;
    }

    if (!m_remoteController)
    {
        m_remoteController = new RemoteControllerWindow();
        connect(m_remoteController, &RemoteControllerWindow::closeWindowSignal, this, [&]()
        {
            // Restore window
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
            this->raise();
        });
    }

    m_remoteController->show();
    m_remoteController->raise();

    // Minimize auto controller window
    this->setWindowState(this->windowState() | Qt::WindowMinimized);
}

//---------------------------------------------------------------------------
// BDSP Box Duplication signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_BDSPBoxDuplication_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// BDSP Box Operation signals
//---------------------------------------------------------------------------
void autocontrollerwindow::on_BDSPBoxOperation_Count_valueChanged(int arg1)
{
    UpdateInfo();
}

void autocontrollerwindow::on_BDSPBoxOperation_Type_currentIndexChanged(int index)
{
    UpdateInfo();
}

//---------------------------------------------------------------------------
// Get <variable> from "xxxxx = <variable>;"
//---------------------------------------------------------------------------
QString autocontrollerwindow::GetVariableString(const QString &_str)
{
    int index = _str.lastIndexOf(' ') + 1;
    return _str.mid(index, _str.lastIndexOf(';') - index);
}

//---------------------------------------------------------------------------
// Read config
//---------------------------------------------------------------------------
void autocontrollerwindow::LoadConfig()
{
    // Check if program exist in database
    QString name = ui->LW_Bots->currentItem()->text();
    m_validProgram = m_programEnumMap.contains(name);
    Program program = P_INVALID;
    if (m_validProgram)
    {
        program = m_programEnumMap[name];
    }

    // Read config file is program is valid
    bool configExist = true;
    QFile configFile(QString(BOT_PATH) + name + "/Config.h");
    if (!m_validProgram)
    {

    }
    else if (!configFile.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Config.h file missing or failed to read!", QMessageBox::Ok);
        configExist = false;
        program = P_INVALID;
    }

    // Set visibility
    ui->GB_Error->setHidden(program != P_INVALID && !configExist);
    ui->GB_DaySkipper->setHidden(program != P_DaySkipper);
    ui->GB_DaySkipper_Unlimited->setHidden(program != P_DaySkipper_Unlimited);
    ui->GB_FriendDeleteAdd->setHidden(program != P_FriendDeleteAdd);
    ui->GB_BerryFarmer->setHidden(false);
    ui->GB_AutoLoto->setHidden(program != P_AutoLoto);
    ui->GB_BoxRelease->setHidden(program != P_BoxRelease);
    ui->GB_AutoFossil->setHidden(program != P_AutoFossil && program != P_AutoFossil_GR);
    ui->GB_Auto3DaySkipper->setHidden(program != P_Auto3DaySkipper);
    ui->GB_BoxSurpriseTrade->setHidden(program != P_BoxSurpriseTrade);
    ui->GB_AutoHost->setHidden(program != P_AutoHost);
    ui->GB_EggCollector->setHidden(program != P_EggCollector && program != P_EggCollector_IT);
    ui->GB_EggHatcher->setHidden(program != P_EggHatcher);
    ui->GB_GodEgg->setHidden(program != P_GodEggDuplication);
    ui->GB_ShinyRegi->setHidden(program != P_ShinyFiveRegi);
    ui->GB_ShinySword->setHidden(program != P_ShinySwordTrio);
    ui->GB_Farmer->setHidden(program != P_DailyHighlightFarmer && program != P_WattFarmer && program != P_BerryFarmer);
    ui->GB_ShinyRegigigas->setHidden(program != P_ShinyRegigigas);

    ui->PB_Unsync->setHidden(program == P_INVALID || program == P_DaySkipper || program == P_DaySkipper_Unlimited || program == P_FriendDeleteAdd);
    ui->L_Unsync->setHidden(program == P_INVALID || program == P_DaySkipper || program == P_DaySkipper_Unlimited || program == P_FriendDeleteAdd);

    switch (program)
    {
    //--------------------------------------------------------
    case P_DaySkipper:
    {
        int day = 1;
        int month = 1;
        int year = 2000;

        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->DaySkipper_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
            else if (line.indexOf("m_dayToSkip = ") != -1)
            {
                ui->DaySkipper_DaysToSkip->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_day = ") != -1)
            {
                day = GetVariableString(line).toInt();
            }
            else if (line.indexOf("m_month = ") != -1)
            {
                month = GetVariableString(line).toInt();
            }
            else if (line.indexOf("m_year = ") != -1)
            {
                year = GetVariableString(line).toInt();
            }
        }

        ui->DaySkipper_Date->setDate(QDate(year, month, day));
        break;
    }

    //--------------------------------------------------------
    case P_DaySkipper_Unlimited:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->DaySkipper_Unlimited_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
            else if (line.indexOf("m_dayToSkip = ") != -1)
            {
                ui->DaySkipper_Unlimited_DaysToSkip->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_TurboA:
    {
        break;
    }

    //--------------------------------------------------------
    case P_FriendDeleteAdd:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_deleteCount = ") != -1)
            {
                ui->FriendDeleteAdd_Count->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_addFriend = ") != -1)
            {
                ui->FriendDeleteAdd_Add->setChecked(GetVariableString(line) == "true" ? true : false);
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_AutoLoto:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->AutoLoto_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
            else if (line.indexOf("m_dayToSkip = ") != -1)
            {
                int value = GetVariableString(line).toInt();
                ui->AutoLoto_CheckBox->setChecked(value > 0);
                if (value > 0)
                {
                    ui->AutoLoto_DaysToSkip->setValue(value);
                }
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_BoxRelease:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_boxCount = ") != -1)
            {
                ui->BoxRelease_Count->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_AutoFossil:
    case P_AutoFossil_GR:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_firstFossilTopSlot = ") != -1)
            {
                ui->AutoFossil_First->setCurrentIndex(GetVariableString(line) == "true" ? 0 : 1);
            }
            else if (line.indexOf("m_secondFossilTopSlot = ") != -1)
            {
                ui->AutoFossil_Second->setCurrentIndex(GetVariableString(line) == "true" ? 0 : 1);
            }
            else if (line.indexOf("m_timesBeforeSR = ") != -1)
            {
                ui->AutoFossil_Count->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_autoSoftReset = ") != -1)
            {
                ui->AutoFossil_SR->setChecked(GetVariableString(line) == "true" ? true : false);
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_Auto3DaySkipper:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->Auto3DaySkipper_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_BoxSurpriseTrade:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_boxesToTrade = ") != -1)
            {
                ui->BoxSurpriseTrade_Count->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_completeDex = ") != -1)
            {
                ui->BoxSurpriseTrade_CompleteDex->setChecked(GetVariableString(line) == "true" ? true : false);
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_AutoHost:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_localMode = ") != -1)
            {
                bool localMode = GetVariableString(line) == "true" ? true : false;
                if (localMode)
                {
                    ui->AutoHost_Local->setChecked(true);
                    on_AutoHost_Local_clicked();
                }
                else
                {
                    ui->AutoHost_Online->setChecked(true);
                    on_AutoHost_Online_clicked();
                }
            }
            else if (line.indexOf("m_internetTime = ") != -1)
            {
                ui->AutoHost_Internet->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->AutoHost_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
            else if (line.indexOf("m_skip3Days = ") != -1)
            {
                ui->AutoHost_SkipDays->setChecked(GetVariableString(line) == "true" ? true : false);
            }
            else if (line.indexOf("m_unsafeDC = ") != -1)
            {
                ui->AutoHost_NoSR->setChecked(GetVariableString(line) == "true" ? true : false);
            }
            else if (line.indexOf("m_useLinkCode = ") != -1)
            {
                ui->AutoHost_UseLinkCode->setChecked(GetVariableString(line) == "true" ? true : false);
            }
            else if (line.indexOf("m_useRandomCode = ") != -1)
            {
                bool useRandomCode = GetVariableString(line) == "true" ? true : false;
                if (useRandomCode)
                {
                    ui->AutoHost_Random->setChecked(true);
                }
                else
                {
                    ui->AutoHost_Fixed->setChecked(true);
                }
                ui->AutoHost_LinkCode->setEnabled(ui->AutoHost_UseLinkCode->isChecked() && ui->AutoHost_Fixed->isChecked());
            }
            else if (line.indexOf("m_linkCode[] = ") != -1)
            {
                QStringList digits = GetVariableString(line).mid(1,15).split(',');
                QString linkCode;
                for (QString const& digit : digits)
                {
                    linkCode.append(digit);
                }
                ui->AutoHost_LinkCode->setText(linkCode.size() == 8 ? linkCode : "00000000");
            }
            else if (line.indexOf("m_waitTime = ") != -1)
            {
                int waitTime = GetVariableString(line).toInt();
                if (waitTime >= 0 && waitTime <= 1)
                {
                    ui->AutoHost_WaitTime->setCurrentIndex(waitTime);
                }
            }
            else if (line.indexOf("m_addFriends = ") != -1)
            {
                ui->AutoHost_AddFriends->setChecked(GetVariableString(line) == "true" ? true : false);
            }
            else if (line.indexOf("m_profile = ") != -1)
            {
                ui->AutoHost_Profile->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_EggCollector:
    case P_EggCollector_IT:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_maxCycle = ") != -1)
            {
                ui->EggCollector_Count->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_EggHatcher:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_columnsOfEggs = ") != -1)
            {
                ui->EggHatcher_Column->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("Pokemon = ") != -1)
            {
                ui->EggHatcher_Pokemon->setCurrentIndex(ui->EggHatcher_Pokemon->findText(GetVariableString(line)));
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_AutoBattleTower:
    {
        break;
    }

    //--------------------------------------------------------
    case P_AutoTournament:
    {
        break;
    }

    //--------------------------------------------------------
    case P_GodEggDuplication:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_maxCycle = ") != -1)
            {
                ui->GodEgg_Count->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_PurpleBeamFinder:
    {
        break;
    }

    //--------------------------------------------------------
    case P_ShinyFiveRegi:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_type = ") != -1)
            {
                ui->ShinyRegi_Type->setCurrentIndex(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_mode = ") != -1)
            {
                int mode = GetVariableString(line).toInt();
                if (mode == 0)
                {
                    ui->ShinyRegi_Slow->setChecked(true);
                }
                else if (mode == 1)
                {
                    ui->ShinyRegi_Fast->setChecked(true);
                }
                else
                {
                    ui->ShinyRegi_Aware->setChecked(true);
                }

                ui->ShinyRegi_Time->setEnabled(mode == 2);
            }
            else if (line.indexOf("m_battleWaitTicks = ") != -1)
            {
                ui->ShinyRegi_Time->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_ShinySwordTrio:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_mode = ") != -1)
            {
                int mode = GetVariableString(line).toInt();
                if (mode == 0)
                {
                    ui->ShinySword_Slow->setChecked(true);
                }
                else if (mode == 1)
                {
                    ui->ShinySword_Fast->setChecked(true);
                }
                else
                {
                    ui->ShinySword_Aware->setChecked(true);
                }

                ui->ShinySword_Time->setEnabled(mode == 2);
            }
            else if (line.indexOf("m_battleWaitTicks = ") != -1)
            {
                ui->ShinySword_Time->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_hourlyRollback = ") != -1)
            {
                ui->ShinySword_Rollback->setChecked(GetVariableString(line) == "true" ? true : false);
            }
            else if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->ShinySword_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_DailyHighlightFarmer:
    case P_WattFarmer:
    case P_BerryFarmer:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_farmTotal = ") != -1)
            {
                ui->Farmer_Count->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_JP_EU_US = ") != -1)
            {
                int m_JP_EU_US = GetVariableString(line).toInt();
                if (m_JP_EU_US >= 0 && m_JP_EU_US <= 2)
                {
                    ui->Farmer_DateArrangement->setCurrentIndex(m_JP_EU_US);
                }
            }
            else if (line.indexOf("m_saveAt = ") != -1)
            {
                ui->Farmer_Save->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_ShinyRegigigas:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_shinyAware = ") != -1)
            {
                bool aware = GetVariableString(line) == "true" ? true : false;
                if (aware)
                {
                    ui->ShinyRegigigas_Aware->setChecked(true);
                }
                else
                {
                    ui->ShinyRegigigas_SR->setChecked(true);
                }

                ui->ShinyRegigigas_Berry->setEnabled(aware);
                ui->ShinyRegigigas_Time->setEnabled(aware);
            }
            else if (line.indexOf("m_leppaBerry = ") != -1)
            {
                ui->ShinyRegigigas_Berry->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_battleWaitTicks = ") != -1)
            {
                ui->ShinyRegigigas_Time->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_SmartProgram:
    {
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_ResetDialgaPalkia:
    {
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_ResetStarter:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_starter = ") != -1)
            {
                int m_starter = GetVariableString(line).toInt();
                if (m_starter >= 0 && m_starter <= 2)
                {
                    ui->BDSPStarter_Type->setCurrentIndex(m_starter);
                }
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_MenuGlitch113:
    {
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxDuplication:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_boxCount = ") != -1)
            {
                ui->BDSPBoxDuplication_Count->setValue(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxOperation:
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.indexOf("m_boxCount = ") != -1)
            {
                ui->BDSPBoxOperation_Count->setValue(GetVariableString(line).toInt());
            }
            else if (line.indexOf("m_operationType = ") != -1)
            {
                ui->BDSPBoxOperation_Type->setCurrentIndex(GetVariableString(line).toInt());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_INVALID:
    {
        if (configExist)
        {
            // custom program
        }
        else
        {
            // someone removed Config.h
        }
        break;
    }
    }

    UpdateInfo();
    UpdateInstruction();

    // Switch to the correct tab
    int tabIndex = 0;
    if (!configExist)
    {
        // error tab
        tabIndex = 3;
    }
    else if (m_tabID.contains(program))
    {
        tabIndex = m_tabID[program];
    }
    ui->SW_Settings->setCurrentIndex(tabIndex);
    ui->SW_Settings->setHidden(tabIndex == 0);

    ui->GB_Info->setHidden(m_validProgram && !configExist);
    ui->GB_Checklist->setHidden(!m_validProgram || !configExist);
    ui->PB_Generate->setEnabled(configExist);
}

//---------------------------------------------------------------------------
// Save config
//---------------------------------------------------------------------------
void autocontrollerwindow::SaveConfig()
{
    if (!m_validProgram)
    {
        return;
    }

    QString name = ui->LW_Bots->currentItem()->text();
    if (!m_programEnumMap.contains(name)) return;
    Program program = m_programEnumMap[name];

    QFile configFile(QString(BOT_PATH) + name + "/Config.h");
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Fail to write Config.h file!", QMessageBox::Ok);
        return;
    }

    QTextStream out(&configFile);
    out << "// WARNING: You are not adviced to change this manually\n// Please run AutoControllerHelper tool!!!\n\n";

    switch (program)
    {
    //--------------------------------------------------------
    case P_DaySkipper:
    {
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->DaySkipper_DateArrangement->currentIndex()) << ";\n";
        QDate const date = ui->DaySkipper_Date->date();
        out << "uint8_t m_day = " << QString::number(date.day()) << ";\n";
        out << "uint8_t m_month = " << QString::number(date.month()) << ";\n";
        out << "int m_year = " << QString::number(date.year()) << ";\n";
        out << "int m_dayToSkip = " << QString::number(ui->DaySkipper_DaysToSkip->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_DaySkipper_Unlimited:
    {
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->DaySkipper_Unlimited_DateArrangement->currentIndex()) << ";\n";
        out << "unsigned long m_dayToSkip = " << QString::number(ui->DaySkipper_Unlimited_DaysToSkip->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_FriendDeleteAdd:
    {
        out << "unsigned int m_deleteCount = " << QString::number(ui->FriendDeleteAdd_Count->value()) << ";\n";
        out << "bool m_addFriend = " << (ui->FriendDeleteAdd_Add->isChecked() ? "true" : "false") << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_AutoLoto:
    {
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->AutoLoto_DateArrangement->currentIndex()) << ";\n";
        int value = ui->AutoLoto_CheckBox->isChecked() ? ui->AutoLoto_DaysToSkip->value() : 0;
        out << "uint16_t m_dayToSkip = " << QString::number(value) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_BoxRelease:
    {
        out << "int m_boxCount = " << QString::number(ui->BoxRelease_Count->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_AutoFossil:
    case P_AutoFossil_GR:
    {
        out << "bool m_firstFossilTopSlot = " << (ui->AutoFossil_First->currentIndex() == 0 ? "true" : "false") << ";\n";
        out << "bool m_secondFossilTopSlot = " << (ui->AutoFossil_Second->currentIndex() == 0 ? "true" : "false") << ";\n";
        out << "int m_timesBeforeSR = " << QString::number(ui->AutoFossil_Count->value()) << ";\n";
        out << "bool m_autoSoftReset = " << (ui->AutoFossil_SR->isChecked() ? "true" : "false") << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_Auto3DaySkipper:
    {
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->Auto3DaySkipper_DateArrangement->currentIndex()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_BoxSurpriseTrade:
    {
        out << "uint8_t m_boxesToTrade = " << QString::number(ui->BoxSurpriseTrade_Count->value()) << ";\n";
        out << "bool m_completeDex = " << (ui->BoxSurpriseTrade_CompleteDex->isChecked() ? "true" : "false") << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_AutoHost:
    {
        out << "bool m_localMode = " << (ui->AutoHost_Local->isChecked() ? "true" : "false") << ";\n";
        out << "uint16_t m_internetTime = " << QString::number(ui->AutoHost_Internet->value()) << ";\n";
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->AutoHost_DateArrangement->currentIndex()) << ";\n";
        out << "bool m_skip3Days = " << (ui->AutoHost_SkipDays->isChecked() ? "true" : "false") << ";\n";
        out << "bool m_unsafeDC = " << (ui->AutoHost_NoSR->isChecked() ? "true" : "false") << ";\n";
        bool useLinkCode = ui->AutoHost_UseLinkCode->isChecked();
        out << "bool m_useLinkCode = " << (useLinkCode ? "true" : "false") << ";\n";
        out << "bool m_useRandomCode = " << ((useLinkCode && ui->AutoHost_Random->isChecked()) ? "true" : "false") << ";\n";
        out << "uint8_t m_seed = " << QString::number(qrand() % 256) << ";\n";

        QString digits = ui->AutoHost_LinkCode->text();
        if (digits.size() != 8)
        {
            digits = "00000000";
        }
        digits.insert(1, ',');
        digits.insert(3, ',');
        digits.insert(5, ',');
        digits.insert(7, ',');
        digits.insert(9, ',');
        digits.insert(11, ',');
        digits.insert(13, ',');
        out << "uint8_t m_linkCode[] = {" << digits << "};\n";

        out << "uint8_t m_waitTime = " << QString::number(ui->AutoHost_WaitTime->currentIndex()) << ";\n";
        out << "bool m_addFriends = " << (ui->AutoHost_AddFriends->isChecked() ? "true" : "false") << ";\n";
        out << "uint8_t m_profile = " << QString::number(ui->AutoHost_Profile->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_EggCollector:
    case P_EggCollector_IT:
    {
        out << "int m_maxCycle = " << QString::number(ui->EggCollector_Count->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_EggHatcher:
    {
        QString pokemon = ui->EggHatcher_Pokemon->currentText();
        out << "// Pokemon = " << pokemon << ";\n";

        int cycleText = m_eggCycles[pokemon];
        QString group;
        if (cycleText == 1280) group = "0";
        else if (cycleText == 2560) group = "1";
        else if (cycleText == 3840) group = "2";
        else if (cycleText == 5120) group = "3";
        else if (cycleText == 6400) group = "4";
        else if (cycleText == 7680) group = "5";
        else if (cycleText == 8960) group = "6";
        else if (cycleText == 10240) group = "7";

        out << "uint8_t m_eggStepGroup = " << group << ";\n";
        out << "uint8_t m_columnsOfEggs = " << QString::number(ui->EggHatcher_Column->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_GodEggDuplication:
    {
        out << "int m_maxCycle = " << QString::number(ui->GodEgg_Count->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_ShinyFiveRegi:
    {
        out << "uint8_t m_type = " << QString::number(ui->ShinyRegi_Type->currentIndex()) << ";\n";
        out << "uint8_t m_mode = " << (ui->ShinyRegi_Aware->isChecked() ? "2" : (ui->ShinyRegi_Fast->isChecked() ? "1" : "0")) << ";\n";
        out << "uint16_t m_battleWaitTicks = " << QString::number(ui->ShinyRegi_Time->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_ShinySwordTrio:
    {
        out << "uint8_t m_mode = " << (ui->ShinySword_Aware->isChecked() ? "2" : (ui->ShinySword_Fast->isChecked() ? "1" : "0")) << ";\n";
        out << "uint16_t m_battleWaitTicks = " << QString::number(ui->ShinySword_Time->value()) << ";\n";
        out << "bool m_hourlyRollback = " << (ui->ShinySword_Rollback->isChecked() ? "true" : "false") << ";\n";
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->ShinySword_DateArrangement->currentIndex()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_DailyHighlightFarmer:
    case P_WattFarmer:
    case P_BerryFarmer:
    {
        out << "uint16_t m_farmTotal = " << QString::number(ui->Farmer_Count->value()) << ";\n";
        out << "uint8_t m_JP_EU_US = " << QString::number(ui->Farmer_DateArrangement->currentIndex()) << ";\n";
        out << "uint16_t m_saveAt = " << QString::number(ui->Farmer_Save->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_ShinyRegigigas:
    {
        out << "bool m_shinyAware = " << (ui->ShinyRegigigas_Aware->isChecked() ? "true" : "false") << ";\n";
        out << "int m_leppaBerry = " << QString::number(ui->ShinyRegigigas_Berry->value()) << ";\n";
        out << "uint16_t m_battleWaitTicks = " << QString::number(ui->ShinyRegigigas_Time->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_ResetStarter:
    {
        out << "uint8_t m_starter = " << QString::number(ui->BDSPStarter_Type->currentIndex()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxDuplication:
    {
        out << "int m_boxCount = " << QString::number(ui->BDSPBoxDuplication_Count->value()) << ";\n";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxOperation:
    {
        out << "int m_boxCount = " << QString::number(ui->BDSPBoxOperation_Count->value()) << ";\n";
        out << "int m_operationType = " << QString::number(ui->BDSPBoxOperation_Type->currentIndex()) << ";\n";
        break;
    }

    default: break;
    }

    configFile.close();
}

//---------------------------------------------------------------------------
// Create a user check list
//---------------------------------------------------------------------------
void autocontrollerwindow::UpdateInstruction()
{
    if (!m_instructionLoaded) return;
    QString name = ui->LW_Bots->currentItem()->text();

    // Save and delete all current checklists
    QLayout* vBox = ui->VL_Checklist->layout();
    while (vBox->count() > 1)
    {
        delete vBox->takeAt(0)->widget();
    }

    // Dynamically create checkboxes
    QDomNodeList bots = m_instructions.firstChildElement().childNodes();
    for (int i = 0; i < bots.count(); i++)
    {
        QDomElement bot = bots.at(i).toElement();
        if (bot.tagName() == name)
        {
            QDomNodeList instructions = bot.childNodes();
            for (int j = instructions.count() - 1; j >= 0; j--)
            {
                QDomElement instruction = instructions.at(j).toElement();
                QString text = instruction.text();

                QLabel *label = new QLabel();
                label->setText(QString::number(j + 1) + ":\t" + text);
                ui->VL_Checklist->insertWidget(0, label);
            }
            break;
        }
    }
}

//---------------------------------------------------------------------------
// Update info
//---------------------------------------------------------------------------
void autocontrollerwindow::UpdateInfo()
{
    ui->progressBar->setValue(0);

    if (!m_validProgram)
    {
        QString info = "No info available for custom-made program, please edit the source code manually.";
        info += "\nMake sure folder name matches the .c program file!";
        info += "\nWhen finished, you can use this to generate the .hex file.";
        ui->L_Info->setText(info);
        return;
    }

    QString name = ui->LW_Bots->currentItem()->text();
    if (!m_programEnumMap.contains(name)) return;
    Program program = m_programEnumMap[name];
    bool valid = true;

    QString info;

    switch (program)
    {
    //--------------------------------------------------------
    case P_DaySkipper:
    {
        QDate startDate = ui->DaySkipper_Date->date();
        QDate endDate = startDate.addDays(ui->DaySkipper_DaysToSkip->value());

        QString dateText;
        int index = ui->DaySkipper_DateArrangement->currentIndex();
        if (index == 0)
        {
            // JP
            dateText = endDate.toString("yyyy/MM/dd");
        }
        else if (index == 1)
        {
            // EU
            dateText = endDate.toString("dd/MM/yyyy");
        }
        else
        {
            // US
            dateText = endDate.toString("MM/dd/yyyy");
        }

        if (endDate.year() > 2060)
        {
            valid = false;
            info = "End date ";
            info += dateText;
            info += " exceed Switch's maximum date (end of 2060)!\nPlease set your system time and configuration to ";
            info += index == 0 ? "2000/1/1." : "1/1/2000.";
        }
        else
        {
            info = "Program Duration: " + GetTimeString(name + QString::number(ui->DaySkipper_DateArrangement->currentIndex()), ui->DaySkipper_DaysToSkip->value());
            info += "\nEnd Date: ";
            info += dateText;
        }
        break;
    }

    //--------------------------------------------------------
    case P_DaySkipper_Unlimited:
    {
        int value = ui->DaySkipper_Unlimited_DaysToSkip->value();
        int iter = value + (value - 1) / 30; // Every 30 skips, one skip extra to go from 31->1
        info = "Program Duration: " + GetTimeString(name + QString::number(ui->DaySkipper_Unlimited_DateArrangement->currentIndex()), iter);
        info += "\nEnd Day: ";
        int endDay = 2 + ((value - 1) % 30);
        info += QString::number(endDay);
        info += (endDay == 1 || endDay == 21 || endDay == 31) ? "st" :
                (endDay == 2 || endDay == 22 || endDay == 32) ? "nd" :
                (endDay == 3 || endDay == 23 || endDay == 33) ? "rd" : "th";
        break;
    }

    //--------------------------------------------------------
    case P_TurboA:
    {
        info = "Common usage:\n> Cram-o-matic\n> Digging Duo/Digging Pa\n> Fossil Farming (first slots only)\n> Auto Accepting Friend Requests";
        break;
    }

    //--------------------------------------------------------
    case P_FriendDeleteAdd:
    {
        int value = ui->FriendDeleteAdd_Count->value();
        info = "Time to delete ";
        info += QString::number(value) + " friends: " + GetTimeString(name, value);
        break;
    }

    //--------------------------------------------------------
    case P_AutoLoto:
    {
        if (ui->AutoLoto_CheckBox->isChecked())
        {
            info = "Program Duration: " + GetTimeString(name, ui->AutoLoto_DaysToSkip->value());
        }
        else
        {
            info = "Time per loto: " + GetTimeString(name, 0);
        }
        break;
    }

    //--------------------------------------------------------
    case P_BoxRelease:
    {
        info = "Program Duration: " + GetTimeString(name, ui->BoxRelease_Count->value());
        break;
    }

    //--------------------------------------------------------
    case P_AutoFossil:
    case P_AutoFossil_GR:
    {
        info = (ui->AutoFossil_SR->isChecked() ? "Time before Soft-Reset: " : "Program Duration: ");
        int type = ui->AutoFossil_First->currentIndex() + ui->AutoFossil_Second->currentIndex();
        info += GetTimeString(name + QString::number(type), ui->AutoFossil_Count->value());
        break;
    }

    //--------------------------------------------------------
    case P_Auto3DaySkipper:
    {
        info = "Time per 3 days skips + soft-reset: " + GetTimeString(name, 0);
        break;
    }

    //--------------------------------------------------------
    case P_BoxSurpriseTrade:
    {
        info = "Program Duration: " + GetTimeString(name + (ui->BoxSurpriseTrade_CompleteDex->isChecked() ? "0" : "1"), ui->BoxSurpriseTrade_Count->value());
        info += "\n\nThe next trade can fail in the following conditions:";
        info += "\n> Unable to find current trade in under 30 seconds";
        info += "\n> Current trade contains a trade evolution";
        break;
    }

    //--------------------------------------------------------
    case P_AutoHost:
    {
        if (ui->AutoHost_UseLinkCode->isChecked() && ui->AutoHost_Fixed->isChecked() && ui->AutoHost_LinkCode->text().size()!= 8)
        {
            valid = false;
            info = "Invalid Link Code!";
        }
        else
        {
            QString timeName = "AutoHost";
            timeName += QString::number(ui->AutoHost_WaitTime->currentIndex());
            timeName += ui->AutoHost_NoSR->isChecked() ? "1" : "0";
            timeName += ui->AutoHost_SkipDays->isChecked() ? QString::number(ui->AutoHost_DateArrangement->currentIndex()) : "";
            info = "Time to host each raid: " + GetTimeString(timeName, 0);

            info += "\n\nAll clients should be ready before the ";
            info += ui->AutoHost_WaitTime->currentIndex() == 0 ? "2:00 mark" : "1:00 mark";
            if (ui->AutoHost_NoSR->isChecked())
            {
                info += "\nHost should monitor the program, if no one joined the raid, the program will break";
            }
            else
            {
                info += "\nClient's side may freeze for 10-15 seconds when host restarts the game";
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_EggCollector:
    case P_EggCollector_IT:
    {
        int count = ui->EggCollector_Count->value();
        if (count > 0)
        {
            info = "Program Duration: " + GetTimeString(name, count);
        }
        else
        {
            info = "Time per egg collection: " + GetTimeString(name, 0);
        }
        info += "\nYou will get about 80% of the target number if you follow all the instructions.";
        break;
    }

    //--------------------------------------------------------
    case P_EggHatcher:
    {
        if (ui->EggHatcher_Pokemon->currentIndex() == -1)
        {
            valid = false;
            info = "No Pokemon selected!";
        }
        else
        {
            int eggCycle = m_eggCycles[ui->EggHatcher_Pokemon->currentText()];
            if (eggCycle > 10240)
            {
                valid = false;
                info = "You cannot hatch this Pokemon!";
            }
            else
            {
                info = "Program Duration: " + GetTimeString(name + QString::number(eggCycle), ui->EggHatcher_Column->value());
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_AutoBattleTower:
    {
        info = "It takes about 30-40 minutes to finish 10 battles (20BP).";
        info += "\nPlease read the manual for more information on teams and training etc.";
        break;
    }

    //--------------------------------------------------------
    case P_AutoTournament:
    {
        info = "WARNING: This program does not work if you have triggered Galarian Star Tournament!";
        info += "\nIt takes about 9-11 minutes to finish one tournament.";
        info += "\nPlease read the manual for more information on teams and training etc.";
        break;
    }

    //--------------------------------------------------------
    case P_GodEggDuplication:
    {
        int count = ui->GodEgg_Count->value();
        if (count > 0)
        {
            info = "Program Duration: " + GetTimeString(name, count);
        }
        else
        {
            info = "Time per duplication: " + GetTimeString(name, 0);
        }
        info += "\nYou will get about 80% of the target number if you follow all the instructions.";
        break;
    }

    //--------------------------------------------------------
    case P_PurpleBeamFinder:
    {
        info = "Time per soft-reset: " + GetTimeString(name, 0);
        info += "\nKeep an eye when putting a wishing piece, if you DON'T see any red streaks,";
        info += "\nthat means you got a purple beam, now simply unplug the board.";
        info += "\n\nProgram by: Pleebz";
        break;
    }

    //--------------------------------------------------------
    case P_ShinyFiveRegi:
    {
        int index = ui->ShinyRegi_Type->currentIndex();
        int mode = ui->ShinyRegi_Aware->isChecked() ? 2 : (ui->ShinyRegi_Fast->isChecked() ? 1 : 0);
        info = (mode == 2 ? "Default time per encounter: " : "Time per encounter: ") + GetTimeString(name + QString::number(index) + QString::number(mode), 0);
        if (ui->ShinyRegi_Aware->isChecked())
        {
            info += "\nYou are REQUIRED to calibrate the battle start ticks, read the MANUAL for details!";
            info += "\nThe game should stuck at Pokemon summary screen when shiny is found!";
            info += "\n\nContributed by: zsebedits";
        }
        else
        {
            info += "\nKeep an eye or listen to shiny effects when the battle starts.";
            info += "\nIf shiny is found unplug the board or take the Switch out from dock IMMEDIATELY!!";
            if (ui->ShinyRegi_Slow->isChecked())
            {
                info += "\nSLOW MODE you are allowed to use any Pokemon.";
            }
            else
            {
                info += "\nFAST MODE you are REQUIRED to follow requirements below (or check Manual)!";
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_ShinySwordTrio:
    {
        int mode = ui->ShinySword_Aware->isChecked() ? 2 : (ui->ShinySword_Fast->isChecked() ? 1 : 0);
        info = (mode == 2 ? "Default time per encounter: " : "Time per encounter: ") + GetTimeString(name + QString::number(mode), 0);
        info += "\nThis program also works with Spiritomb encounter in Ballimere Lake!";

        if (ui->ShinySword_Aware->isChecked())
        {
            info += "\nYou are REQUIRED to calibrate the battle start ticks, read the MANUAL for details!";
            info += "\nThe game should stuck at Pokemon summary screen when shiny is found!";
            info += "\n\nContributed by: Yeray Arroyo";
        }
        else
        {
            info += "\nKeep an eye or listen to shiny effects when the battle starts.";
            info += "\nIf shiny is found unplug the board or take the Switch out from dock IMMEDIATELY!!";
            if (ui->ShinySword_Slow->isChecked())
            {
                info += "\n\nSLOW MODE you are allowed to use any Pokemon.";
            }
            else
            {
                info += "\n\nFAST MODE you are REQUIRED to follow requirements below (or check Manual)!";
            }
        }
        break;
    }

    //--------------------------------------------------------
    case P_DailyHighlightFarmer:
    case P_WattFarmer:
    case P_BerryFarmer:
    {
        if (ui->Farmer_Count->value() > 0)
        {
            info = "Program Duration: " + GetTimeString(name, ui->Farmer_Count->value());
        }
        else
        {
            info = "Time per farm: " + GetTimeString(name, 0);
        }
        break;
    }

    //--------------------------------------------------------
    case P_ShinyRegigigas:
    {
        bool aware = ui->ShinyRegigigas_Aware->isChecked();
        if (aware)
        {
            int berry = ui->ShinyRegigigas_Berry->value();
            if (berry == 0)
            {
                info = "Program Duration (10 encounters): " + GetTimeString(name + "1", 10);
            }
            else
            {
                info = "Program Duration (" + QString::number(berry * 10 + 10) + QString(" encounters): ") + GetTimeString(name + "1", berry * 10, name + "2");
            }
            info += "\nYou are REQUIRED to calibrate the battle start ticks, read the MANUAL for details!";
            info += "\nThe game should stuck at Pokemon summary screen or battle when shiny is found!";
        }
        else
        {
            info = "Time per encounter: " + GetTimeString(name + "0", 0);
            info += "\nKeep an eye on Regigigas's color (blue) when the battle starts.";
            info += "\nIf shiny is found unplug the board or take the Switch out from dock IMMEDIATELY!!";
        }
        info += "\n\nContributed by: Yeray Arroyo";
        break;
    }

    //--------------------------------------------------------
    case P_SmartProgram:
    {
        info = "This is an advanced program that allows you to control the game on PC.";
        info += "\nYou are REQUIRED to also have CP2014 chip and a capture card for this.";
        info += "\nYou can also use this to run Smart Programs without recompiling.";
        info += "\nFor more details, please read Smart Program section of the manual.";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_ResetDialgaPalkia:
    {
        info = "Time per soft-reset: " + GetTimeString(name, 0);
        info += "\nThis program DOES NOT stop by itself, you MUST keep an eye on encounters.";
        info += "\nIf shiny is found unplug the board or take the Switch out from dock IMMEDIATELY!!";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_ResetStarter:
    {
        info = "Time per soft-reset: " + GetTimeString(name, 0);
        info += "\nThis program DOES NOT stop by itself, you MUST keep an eye on encounters.";
        info += "\nIf shiny is found unplug the board or take the Switch out from dock IMMEDIATELY!!";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_MenuGlitch113:
    {
        info = "WARNING: This glitch maybe patched by Nintendo and may not work in later versions!";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxDuplication:
    {
        info = "Program Duration: " + GetTimeString(name, ui->BDSPBoxDuplication_Count->value());
        info += "\nFor item duplication, use BDSP_BoxOperation instead.";
        break;
    }

    //--------------------------------------------------------
    case P_BDSP_BoxOperation:
    {
        int index = ui->BDSPBoxOperation_Type->currentIndex();
        info = "Program Duration: " + GetTimeString(name + QString::number(index), ui->BDSPBoxOperation_Count->value());
        break;
    }

    case P_INVALID: break;
    }

    QPalette pal = ui->L_Info->palette();
    pal.setColor(QPalette::WindowText, valid ? QColor(0,0,0) : QColor(255,0,0));
    ui->L_Info->setPalette(pal);
    ui->L_Info->setText(info);
    ui->PB_Generate->setEnabled(valid);
}

//---------------------------------------------------------------------------
// Get string for how long the program will run
//---------------------------------------------------------------------------
QString autocontrollerwindow::GetTimeString(const QString &_bot, int _iter, const QString &_botExtra)
{
    QString str;
    if (!m_times.contains(_bot))
    {
        str = "Unknown";
    }
    else
    {
        int64_t day = 0;
        int64_t hour = 0;
        int64_t minute = 0;
        int64_t second = 0;
        int64_t milsecond = ((_iter != 0) ? _iter : 1) * static_cast<int64_t>(m_times[_bot]);

        // Extra time added to total time
        if (!_botExtra.isEmpty() && m_times.contains(_botExtra))
        {
            milsecond += static_cast<int64_t>(m_times[_botExtra]);
        }

        second = milsecond / 1000;
        milsecond = milsecond % 1000;

        minute = second / 60;
        second = second % 60;

        hour = minute / 60;
        minute = minute % 60;

        day = hour / 24;
        hour = hour % 24;

        str = day > 0 ? QString::number(day) + (day == 1 ? " day " : " days ") : "";
        str += hour > 0 ? QString::number(hour) + (hour == 1 ? " hour " : " hours ") : "";
        str += minute > 0 ? QString::number(minute) + (minute == 1 ? " minute " : " minutes ") : "";

        double sms = static_cast<double>(second) + (static_cast<double>(milsecond) / 1000.0);
        str += QString::number(sms) + " seconds";
    }

    return str;
}
