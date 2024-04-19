#include "remotecontrollerwindow.h"
#include "ui_remotecontrollerwindow.h"

RemoteControllerWindow::RemoteControllerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RemoteControllerWindow)
{
    ui->setupUi(this);
    connect(&m_serialPort, &QSerialPort::readyRead, this, &RemoteControllerWindow::on_SerialPort_readyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &RemoteControllerWindow::on_SerialPort_errorOccurred);
    connect(&m_readTimer, &QTimer::timeout, this, &RemoteControllerWindow::on_ReadTimer_timeout);
    m_readTickCount = 0;
    m_executedCommandDirty = false;

    m_serialState = SS_Disconnect;

    // Get list initially
    on_PB_Refresh_clicked();
    on_PB_RefreshCamera_clicked();

    // Save the list of ui button pointers
    m_buttonMapList.push_back(ui->PB_A);
    m_buttonMapList.push_back(ui->PB_B);
    m_buttonMapList.push_back(ui->PB_X);
    m_buttonMapList.push_back(ui->PB_Y);
    m_buttonMapList.push_back(ui->PB_L);
    m_buttonMapList.push_back(ui->PB_R);
    m_buttonMapList.push_back(ui->PB_ZL);
    m_buttonMapList.push_back(ui->PB_ZR);
    m_buttonMapList.push_back(ui->PB_Plus);
    m_buttonMapList.push_back(ui->PB_Minus);
    m_buttonMapList.push_back(ui->PB_Home);
    m_buttonMapList.push_back(ui->PB_Capture);
    m_buttonMapList.push_back(ui->PB_LClick);
    m_buttonMapList.push_back(ui->PB_LUp);
    m_buttonMapList.push_back(ui->PB_LDown);
    m_buttonMapList.push_back(ui->PB_LLeft);
    m_buttonMapList.push_back(ui->PB_LRight);
    m_buttonMapList.push_back(ui->PB_RClick);
    m_buttonMapList.push_back(ui->PB_RUp);
    m_buttonMapList.push_back(ui->PB_RDown);
    m_buttonMapList.push_back(ui->PB_RLeft);
    m_buttonMapList.push_back(ui->PB_RRight);
    m_buttonMapList.push_back(ui->PB_DUp);
    m_buttonMapList.push_back(ui->PB_DDown);
    m_buttonMapList.push_back(ui->PB_DLeft);
    m_buttonMapList.push_back(ui->PB_DRight);

    // Flags
    for (int i = 0; i < m_buttonMapList.size(); i++)
    {
        PushButtonMapping* pb = m_buttonMapList[i];
        m_PBToFlagMap[pb] = 1 << i;
    }

    m_flagToCharMap.insert(0, '0');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_A], '1');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_B], '2');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_X], '3');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_Y], '4');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_L], '5');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_R], '6');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZL], '7');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZR], '8');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_Plus], '9');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_Minus], 'q');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_Home], 'w');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_Capture], 'e');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LClick], 'r');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LUp], 't');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LDown], 'y');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LLeft], 'u');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LRight], 'i');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_RClick], 'o');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_RUp], 'p');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_RDown], 'a');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_RLeft], 's');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_RRight], 'd');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_DUp], 'f');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_DDown], 'g');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_DLeft], 'h');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_DRight], 'j');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_ZR], 'k');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LLeft], 'l');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LRight], 'z');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_LLeft], 'x');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_LRight], 'c');
    // ASpam = 'v'
    // BSpam = 'b'
    // Loop = 'n'
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_A], 'm');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_A], ',');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LRight] | m_PBToFlagMap[ui->PB_A], '.');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_DRight] | m_PBToFlagMap[ui->PB_R], '/');
    // INVALID = '!'
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LClick], '@');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LLeft] | m_PBToFlagMap[ui->PB_B], '#');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_LRight] | m_PBToFlagMap[ui->PB_B], '$');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_X] | m_PBToFlagMap[ui->PB_DUp], '%');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZR] | m_PBToFlagMap[ui->PB_DUp], '^');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_Y], '&');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_X], '*');
    m_flagToCharMap.insert(m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_A], '(');

    m_commandToFlagMap.insert("Nothing", 0);
    m_commandToFlagMap.insert("A", m_PBToFlagMap[ui->PB_A]);
    m_commandToFlagMap.insert("B", m_PBToFlagMap[ui->PB_B]);
    m_commandToFlagMap.insert("X", m_PBToFlagMap[ui->PB_X]);
    m_commandToFlagMap.insert("Y", m_PBToFlagMap[ui->PB_Y]);
    m_commandToFlagMap.insert("L", m_PBToFlagMap[ui->PB_L]);
    m_commandToFlagMap.insert("R", m_PBToFlagMap[ui->PB_R]);
    m_commandToFlagMap.insert("ZL", m_PBToFlagMap[ui->PB_ZL]);
    m_commandToFlagMap.insert("ZR", m_PBToFlagMap[ui->PB_ZR]);
    m_commandToFlagMap.insert("Plus", m_PBToFlagMap[ui->PB_Plus]);
    m_commandToFlagMap.insert("Minus", m_PBToFlagMap[ui->PB_Minus]);
    m_commandToFlagMap.insert("Home", m_PBToFlagMap[ui->PB_Home]);
    m_commandToFlagMap.insert("Capture", m_PBToFlagMap[ui->PB_Capture]);
    m_commandToFlagMap.insert("LClick", m_PBToFlagMap[ui->PB_LClick]);
    m_commandToFlagMap.insert("LUp", m_PBToFlagMap[ui->PB_LUp]);
    m_commandToFlagMap.insert("LDown", m_PBToFlagMap[ui->PB_LDown]);
    m_commandToFlagMap.insert("LLeft", m_PBToFlagMap[ui->PB_LLeft]);
    m_commandToFlagMap.insert("LRight", m_PBToFlagMap[ui->PB_LRight]);
    m_commandToFlagMap.insert("RClick", m_PBToFlagMap[ui->PB_RClick]);
    m_commandToFlagMap.insert("RUp", m_PBToFlagMap[ui->PB_RUp]);
    m_commandToFlagMap.insert("RDown", m_PBToFlagMap[ui->PB_RDown]);
    m_commandToFlagMap.insert("RLeft", m_PBToFlagMap[ui->PB_RLeft]);
    m_commandToFlagMap.insert("RRight", m_PBToFlagMap[ui->PB_RRight]);
    m_commandToFlagMap.insert("DUp", m_PBToFlagMap[ui->PB_DUp]);
    m_commandToFlagMap.insert("DDown", m_PBToFlagMap[ui->PB_DDown]);
    m_commandToFlagMap.insert("DLeft", m_PBToFlagMap[ui->PB_DLeft]);
    m_commandToFlagMap.insert("DRight", m_PBToFlagMap[ui->PB_DRight]);
    m_commandToFlagMap.insert("Triggers", m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_ZR]);
    m_commandToFlagMap.insert("LUpLeft", m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LLeft]);
    m_commandToFlagMap.insert("LUpRight", m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LRight]);
    m_commandToFlagMap.insert("LDownLeft", m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_LLeft]);
    m_commandToFlagMap.insert("LDownRight", m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_LRight]);
    m_commandToFlagMap.insert("LUpA", m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_A]);
    m_commandToFlagMap.insert("LDownA", m_PBToFlagMap[ui->PB_LDown] | m_PBToFlagMap[ui->PB_A]);
    m_commandToFlagMap.insert("LRightA", m_PBToFlagMap[ui->PB_LRight] | m_PBToFlagMap[ui->PB_A]);
    m_commandToFlagMap.insert("DRightR", m_PBToFlagMap[ui->PB_DRight] | m_PBToFlagMap[ui->PB_R]);
    m_commandToFlagMap.insert("LUpClick", m_PBToFlagMap[ui->PB_LUp] | m_PBToFlagMap[ui->PB_LClick]);
    m_commandToFlagMap.insert("LLeftB", m_PBToFlagMap[ui->PB_LLeft] | m_PBToFlagMap[ui->PB_B]);
    m_commandToFlagMap.insert("LRightB", m_PBToFlagMap[ui->PB_LRight] | m_PBToFlagMap[ui->PB_B]);
    m_commandToFlagMap.insert("BXDUp", m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_X] | m_PBToFlagMap[ui->PB_DUp]);
    m_commandToFlagMap.insert("ZRDUp", m_PBToFlagMap[ui->PB_ZR] | m_PBToFlagMap[ui->PB_DUp]);
    m_commandToFlagMap.insert("BY", m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_Y]);
    m_commandToFlagMap.insert("ZLBX", m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_B] | m_PBToFlagMap[ui->PB_X]);
    m_commandToFlagMap.insert("ZLA", m_PBToFlagMap[ui->PB_ZL] | m_PBToFlagMap[ui->PB_A]);

    // Special case
    m_commandToFlagMap.insert("ASpam", m_PBToFlagMap[ui->PB_A] | m_turboFlag);
    m_commandToFlagMap.insert("BSpam", m_PBToFlagMap[ui->PB_B] | m_turboFlag);
    m_commandToFlagMap.insert("Loop", 0);
    // NOTE: Must match QSet in SmartProgramBase::validateCommand!

    QStringList validCommands;
    for (QMap<QString, ButtonFlag>::iterator iter = m_commandToFlagMap.begin(); iter != m_commandToFlagMap.end(); iter++)
    {
        validCommands << iter.key();
    }
    validCommands.sort();
    ui->LE_CommandSender->InitCompleter(validCommands);
    ui->SP5_LE_PokemonList->InitCompleter(PokemonDatabase::getList_SwShSprites());
    ui->SP13_LE_PokemonList->InitCompleter(PokemonDatabase::getList_PLAMassOutbreak());

    // Logging
    m_logCount = 0;
    m_successCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;

    // Buttons
    on_PB_MapDefault_clicked();
    m_buttonFlag = 0;
    m_buttonMapChange = Q_NULLPTR;
    ui->L_MapWait->setVisible(false);
    qApp->installEventFilter(this);
    m_pauseKeyEventFilter = false;

    // Video
    m_vlcWrapper = new VLCWrapper(this);
    connect(m_vlcWrapper, &VLCWrapper::stateChanged, this, &RemoteControllerWindow::on_VLCState_changed);
    VideoManager* videoManager = m_vlcWrapper->getVideoManager();
    ui->VL_MediaView->insertWidget(0, videoManager);
    connect(videoManager, &VideoManager::printLog, this, &RemoteControllerWindow::on_SmartProgram_printLog);

    // Audio
    AudioManager* audioManager = m_vlcWrapper->getAudioManager();
    ui->VL_MediaView->insertWidget(0, audioManager);
    connect(ui->CB_AudioDisplayMode, SIGNAL(currentIndexChanged(int)), audioManager, SLOT(displayModeChanged(int)));
    connect(ui->SB_AudioSamples, SIGNAL(valueChanged(int)), audioManager, SLOT(displaySampleChanged(int)));
    connect(ui->S_Volume, &QSlider::valueChanged, audioManager, &AudioManager::setVolume);
    connect(ui->SB_AudioFreqLow, SIGNAL(valueChanged(int)), audioManager, SLOT(freqLowChanged(int)));
    connect(ui->SB_AudioFreqHigh, SIGNAL(valueChanged(int)), audioManager, SLOT(freqHighChanged(int)));
    connect(audioManager, &AudioManager::printLog, this, &RemoteControllerWindow::on_SmartProgram_printLog);
    connect(audioManager, &AudioManager::soundDetectionRequired, this, &RemoteControllerWindow::on_SoundDetection_required);

    if (!QDir(SCREENSHOT_PATH).exists())
    {
        QDir().mkdir(SCREENSHOT_PATH);
    }
    if (!QDir(LOG_PATH).exists())
    {
        QDir().mkdir(LOG_PATH);
    }
    if (!QDir(STREAM_COUNTER_PATH).exists())
    {
        QDir().mkdir(STREAM_COUNTER_PATH);
    }

    // Smart program
    m_commandMutex = new QMutex(QMutex::RecursionMode::Recursive);
    for (int i = 0 ; i < SP_COUNT; i++)
    {
        SmartProgram sp = (SmartProgram)i;
        if (SmartProgramBase::getProgramTabID(sp) >= 0)
        {
            QString const name = SmartProgramBase::getProgramNameFromEnum(sp);
            if (name != "Invalid")
            {
                ui->LW_SmartProgram->addItem(name);
            }
        }
    }

    // Load all timings from .xml file
    ValidateSmartProgramCommands();

    m_SP1_graphicScene = new QGraphicsScene(this);
    m_SP1_graphicSceneMasked = new QGraphicsScene(this);
    ui->SP1_GV_Preview->setScene(m_SP1_graphicScene);
    ui->SP1_GV_PreviewMasked->setScene(m_SP1_graphicSceneMasked);

    m_SP2_graphicScene = new QGraphicsScene(this);
    m_SP2_graphicSceneMasked = new QGraphicsScene(this);
    ui->SP2_GV_Preview->setScene(m_SP2_graphicScene);
    ui->SP2_GV_PreviewMasked->setScene(m_SP2_graphicSceneMasked);
    ui->SP2_Frame_Hide->setVisible(false);

    SmartPLAStaticSpawn::populateStaticPokemon(ui->SP16_CB_StaticPokemon);
    SmartMaxLair::populateMaxLairBoss(ui->SP21_CB_Legend);
    PokemonDatabase::populatePokeballs(ui->SP21_CB_LegendBall);
    PokemonDatabase::populatePokeballs(ui->SP21_CB_BossBall);

    // Call this after all smart program values are set up above
    //ui->LW_SmartProgram->setCurrentRow(0);
    on_CB_SmartProgram_currentIndexChanged(0);

    // Load previous settings
    m_settings = new QSettings("brianuuu", "AutoControllerHelper", this);
    LoadButtonMapSettings();

    // Set last used serial and camera
    ui->CB_SerialPorts->setCurrentText(m_settings->value("SerialName", "").toString());
    ui->CB_Cameras->setCurrentText(m_settings->value("CameraName", "").toString());

    ui->LE_CustomVideo->setText(m_settings->value("CustomVideo", "").toString());
    ui->LE_CustomAudio->setText(m_settings->value("CustomAudio", "").toString());
    if (m_settings->value("UseDetectedDevice", true).toBool())
    {
        ui->RB_DetectedDevice->setChecked(true);
        on_RB_DetectedDevice_clicked();
    }
    else
    {
        ui->RB_CustomDevice->setChecked(true);
        on_RB_CustomDevice_clicked();
    }

    // Set last used audio settings
    ui->L_AudioFreq->setHidden(true);
    ui->SB_AudioFreqLow->setHidden(true);
    ui->SB_AudioFreqHigh->setHidden(true);
    ui->S_Volume->setValue(m_settings->value("Volume", 100).toInt());
    audioManager->setVolume(ui->S_Volume->value());
    ui->CB_AudioDisplayMode->setCurrentIndex(m_settings->value("AudioDisplayMode", 0).toInt());
    ui->SB_AudioSamples->setValue(m_settings->value("AudioDisplaySamples", 1024).toInt());
    ui->SB_AudioFreqLow->setValue(m_settings->value("AudioFreqLow", 100).toInt());
    ui->SB_AudioFreqHigh->setValue(m_settings->value("AudioFreqHigh", 10500).toInt());
    ui->SB_AudioFreqHigh->setMinimum(ui->SB_AudioFreqLow->value() + 100);
    ui->SB_AudioFreqLow->setMaximum(ui->SB_AudioFreqHigh->value() - 100);

    m_smartSetting = new SmartProgramSetting();
    m_videoEffectSetting = new VideoEffectSetting();
    connect(m_videoEffectSetting, &VideoEffectSetting::hueChanged, m_vlcWrapper, &VLCWrapper::setHue);
    connect(m_videoEffectSetting, &VideoEffectSetting::saturationChanged, m_vlcWrapper, &VLCWrapper::setSaturation);
    connect(m_videoEffectSetting, &VideoEffectSetting::gammaChanged, m_vlcWrapper, &VLCWrapper::setGamma);

    // Goto last used smart program
    ui->CB_SmartProgram->blockSignals(true);
    ui->CB_SmartProgram->setCurrentIndex(m_settings->value("SmartProgramGame", 0).toInt());
    ui->CB_SmartProgram->blockSignals(false);
    on_CB_SmartProgram_currentIndexChanged(ui->CB_SmartProgram->currentIndex());

    QString const savedProgram = m_settings->value("SmartProgramName", SmartProgramBase::getProgramNameFromEnum(SP_BattleTower)).toString();
    for (int i = 0; i < ui->LW_SmartProgram->count(); i++)
    {
        if (ui->LW_SmartProgram->item(i)->text() == savedProgram)
        {
            ui->LW_SmartProgram->setCurrentRow(i);
            break;
        }
    }

    // values we want to load
    ui->SP9_SB_X->setValue(m_settings->value("SandwichX", 0).toInt());
    ui->SP9_SB_Y->setValue(m_settings->value("SandwichY", 0).toInt());

    QSize screenSize = QGuiApplication::primaryScreen()->size();
    ui->SP19_SB_X->setRange(0, screenSize.width() - 1);
    ui->SP19_SB_Y->setRange(0, screenSize.height() - 1);
    ui->SP19_SB_Width->setRange(1, screenSize.width());
    ui->SP19_SB_Height->setRange(1, screenSize.height());
    ui->SP19_LE_Code->setMaxLength(ui->SP19_SP_Count->value());

    // manually create menu bar since QWidget can't do it
    QMenuBar* menuBar = new QMenuBar();
    QMenu *fileMenu = new QMenu("Window");
    menuBar->addMenu(fileMenu);
    this->layout()->setMenuBar(menuBar);
    m_actionBreakLayout = fileMenu->addAction("Use Separate Layout");
    m_actionBreakLayout->setCheckable(true);
    connect(m_actionBreakLayout, &QAction::triggered, this, &RemoteControllerWindow::ActionBreakLayout_triggered);
    m_actionSmartProgram = fileMenu->addAction("Smart Program Settings");
    m_actionSmartProgram->setEnabled(false);
    connect(m_actionSmartProgram, &QAction::triggered, this, &RemoteControllerWindow::ActionSmartProgram_triggered);
    m_actionCommandLog = fileMenu->addAction("Command Sender && Log");
    m_actionCommandLog->setEnabled(false);
    connect(m_actionCommandLog, &QAction::triggered, this, &RemoteControllerWindow::ActionCommandLog_triggered);
    m_actionController = fileMenu->addAction("Virtual Controller");
    m_actionController->setCheckable(true);
    if (m_settings->value("ShowPopOutController", true).toBool())
    {
        m_actionController->setChecked(true);
    }
    else
    {
        m_actionController->setChecked(false);
        ActionController_triggered();
    }
    connect(m_actionController, &QAction::triggered, this, &RemoteControllerWindow::ActionController_triggered);
    if (m_settings->value("PopOut", false).toBool())
    {
        m_actionBreakLayout->setChecked(true);
        PopOut();
    }

    // Discord settings
    m_discordSettings = new DiscordSettings();
    QMenu *discordMenu = new QMenu("Discord");
    menuBar->addMenu(discordMenu);
    QAction* discordAction = discordMenu->addAction("Connection Settings");
    connect(discordAction, &QAction::triggered, this, &RemoteControllerWindow::ActionDiscord_triggered);
    connect(m_discordSettings, &DiscordSettings::signalScreenshot, this, &RemoteControllerWindow::DiscordScreenshot);
    connect(m_discordSettings, &DiscordSettings::signalStatus, this, &RemoteControllerWindow::DiscordStatus);
    connect(m_discordSettings, &DiscordSettings::signalCommand, this, &RemoteControllerWindow::DiscordCommand);
}

RemoteControllerWindow::~RemoteControllerWindow()
{
    delete m_smartSetting;
    delete m_videoEffectSetting;
    delete m_commandMutex;
    delete ui;
}

bool RemoteControllerWindow::eventFilter(QObject *object, QEvent *event)
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget)
    {
        QString className = QString(focusWidget->metaObject()->className());
        static const QSet<QString> classNameToIgnore
        {
            "QLineEdit",
            "CommandSender",
            "QSpinBox",
            "QDoubleSpinBox",
            "PokemonAutofillLineEdit"
        };
        if (classNameToIgnore.contains(className))
        {
            return false;
        }
    }

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        if (m_pauseKeyEventFilter)
        {
            return false;
        }

        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        if (e->isAutoRepeat())
        {
            return false;
        }

        int key = e->key();
        if (key == 16777250) // Windows key
        {
            return false;
        }

        if (event->type() == QEvent::KeyPress && m_buttonMapChange != Q_NULLPTR)
        {
            // Remapping key
            UpdateButtonMap(m_buttonMapChange, key);
            ButtonMapToggle(m_buttonMapChange);
        }
        else if (m_executeCommands.isEmpty())
        {
            if (!m_smartProgram || (m_smartProgram && SmartProgramBase::getProgramEnableControl(m_smartProgram->getProgramEnum())))
            // Button press changed, only allow when no smart program or commands is running in progress
            UpdateButtonFlags(key, event->type() == QEvent::KeyPress);
        }

        return true;
    }

    return false;
}

void RemoteControllerWindow::closeEvent(QCloseEvent *event)
{
    SerialDisconnect();
    CameraToggle(false);
    // EnableSmartProgram(); // already called on above two

    SaveButtonMapSettings();

    // Save last used serial and camera
    m_settings->setValue("SerialName", ui->CB_Cameras->currentText());
    m_settings->setValue("CameraName", ui->CB_Cameras->currentText());

    m_settings->setValue("CustomVideo", ui->LE_CustomVideo->text());
    m_settings->setValue("CustomAudio", ui->LE_CustomAudio->text());
    m_settings->setValue("UseDetectedDevice", ui->RB_DetectedDevice->isChecked());

    // Save audio settings
    m_settings->setValue("Volume", ui->S_Volume->value());
    m_settings->setValue("AudioDisplayMode", ui->CB_AudioDisplayMode->currentIndex());
    m_settings->setValue("AudioDisplaySamples", ui->SB_AudioSamples->value());
    m_settings->setValue("AudioFreqLow", ui->SB_AudioFreqLow->value());
    m_settings->setValue("AudioFreqHigh", ui->SB_AudioFreqHigh->value());

    // Remember last used Smart Program
    m_settings->setValue("SmartProgramGame", ui->CB_SmartProgram->currentIndex());
    if (ui->LW_SmartProgram->currentRow() != -1)
    {
        m_settings->setValue("SmartProgramName", ui->LW_SmartProgram->currentItem()->text());
    }

    // values we want to save
    m_settings->setValue("SandwichX", ui->SP9_SB_X->value());
    m_settings->setValue("SandwichY", ui->SP9_SB_Y->value());

    // remove all pop out windows
    DeletePopOut();

    emit closeWindowSignal();
}

void RemoteControllerWindow::mousePressEvent(QMouseEvent *event)
{
    SetCaptureAreaPos(event);
}

void RemoteControllerWindow::mouseMoveEvent(QMouseEvent *event)
{
    SetCaptureAreaPos(event);
}

//---------------------------------------------------------------------------
// Other functions
//---------------------------------------------------------------------------
void RemoteControllerWindow::DisplayNextCommand()
{
    // Unhighlight
    for (PushButtonMapping* pb : m_buttonMapList)
    {
        if (pb != m_buttonMapChange)
        {
            pb->ButtonReleased();
        }
        else
        {
            pb->ButtonMapStart();
        }
    }

    m_commandMutex->lock();
    if (!m_displayFlags.isEmpty())
    {
        // Highlight buttons
        ButtonFlag const flag = m_displayFlags.front();
        m_displayFlags.pop_front();
        for (int i = 0; i < m_buttonMapList.size(); i++)
        {
            if (flag & (1 << i))
            {
                if (flag & m_turboFlag)
                {
                    m_buttonMapList[i]->ButtonTurbo();
                }
                else
                {
                    m_buttonMapList[i]->ButtonPressed();
                }
            }
        }

        // Print commands if smart program is not running
        if (!m_smartProgram && !m_executeCommands.isEmpty())
        {
            CommandPair const& command = m_executeCommands.front();
            if (command.first.toLower() == "loop")
            {
                if (command.second == 0)
                {
                    PrintLog("Looping indefinitely, send new commands or any virtual controller input to stop it", LOG_WARNING);
                }
                else
                {
                    PrintLog("Looping: " + QString::number(command.second - 1) + " left");
                }
            }
            else
            {
                PrintLog("Executing command: [" + command.first + "," + QString::number(command.second) + "]");
            }
        }

        // For detecting disconnection (tick = 48.05, but we bump it up to 60)
        m_readTickCount = m_executeCommands.front().second * 60;
    }
    m_commandMutex->unlock();

    qApp->processEvents();
}

void RemoteControllerWindow::PrintLog(const QString &log, QColor color)
{
    QString r,g,b;
    r.setNum(color.red(), 16); if (color.red() < 0x10) r = "0" + r;
    g.setNum(color.green(), 16); if (color.green() < 0x10) g = "0" + g;
    b.setNum(color.blue(), 16); if (color.blue() < 0x10) b = "0" + b;

    QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "&nbsp;&nbsp;&nbsp;&nbsp;" + log;
    str = "<font color=\"#FF" + r + g + b + "\">" + str + "</font>";
    ui->TB_Log->append(str);

    m_logCount++;
    if (color == LOG_SUCCESS) m_successCount++;
    if (color == LOG_WARNING) m_warningCount++;
    if (color == LOG_ERROR) m_errorCount++;
    UpdateLogStat();

    // Clear if there are too many logs
    if (m_logCount >= 5000)
    {
        m_logCount = 0;
        ui->TB_Log->clear();
    }
}

void RemoteControllerWindow::SaveLog(const QString name)
{
    QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + name + ".log";
    QFile file(LOG_PATH + nameWithTime);
    if(file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);
        stream << ui->TB_Log->toPlainText();
        file.close();

        PrintLog("Log file saved: " + nameWithTime);
    }
}

void RemoteControllerWindow::UpdateLogStat()
{
    QString stat = "<font color=\"#FF00AA00\">Success: " + QString::number(m_successCount) + "</font>";
    stat += "&nbsp;&nbsp;<font color=\"#FFFF7800\">Warning: " + QString::number(m_warningCount) + "</font>";
    stat += "&nbsp;&nbsp;<font color=\"#FFFF0000\">Error: " + QString::number(m_errorCount) + "</font>";
    ui->L_LogStat->setText(stat);
}

void RemoteControllerWindow::UpdateStats(const SmartProgram sp, bool reset)
{
    // Delete all stream counter text files
    QDirIterator it(QString(STREAM_COUNTER_PATH));
    while (it.hasNext())
    {
        QString dir = it.next();
        QFile::remove(dir);
    }

    // Grab stats for current program
    QSettings stats(SMART_STATS_INI, QSettings::IniFormat, this);
    stats.beginGroup(SmartProgramBase::getProgramInternalNameFromEnum(sp));
    QStringList list = stats.allKeys();
    QString statsStr;
    if (list.isEmpty())
    {
        statsStr = "N/A";
        ui->PB_ResetStats->setEnabled(false);
    }
    else
    {
        ui->PB_ResetStats->setEnabled(true);
        for (int i = 0; i < list.size(); i++)
        {
            QString const& key = list[i];
            if (i != 0)
            {
                statsStr += ", ";
            }

            if (reset)
            {
                stats.setValue(key, 0);
            }

            int count = stats.value(key, 0).toInt();
            statsStr += key + ": " + QString::number(count);

            // Write to individual files for each stat
            if (m_smartSetting->isStreamCounterEnabled())
            {
                QFile file(STREAM_COUNTER_PATH + key + ".txt");
                if(file.open(QIODevice::WriteOnly))
                {
                    QTextStream stream(&file);
                    if (!m_smartSetting->isStreamCounterExcludePrefix())
                    {
                        stream << key + ": ";
                    }
                    stream << count;
                    file.close();
                }
            }
        }
    }
    ui->L_CurrentStats->setText(statsStr);
}

//---------------------------------------------------------------------------
// Read serial signal
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_SerialPort_readyRead()
{
    QByteArray ba = m_serialPort.readAll();

    if (m_serialState != SS_Connect)
    {
        if (!ba.isEmpty())
        {
            m_hexVersion = (uint8_t)ba.front();
            if (m_hexVersion == 120)
            {
                m_hexVersion = 1;
            }

            // Version checking
            if (m_hexVersion == SMART_HEX_VERSION)
            {
                m_serialState = SS_FeedbackOK;
            }
            else
            {
                m_serialState = SS_WrongVersion;
            }
        }
        return;
    }

    // Command can be interrupted right at this moment from SendCommand(), if we have feedback ready,
    // this causes the first command for the new set to be removed earlier than it should!

    m_commandMutex->lock();
    if (!ba.isEmpty() && !m_executeCommands.isEmpty())
    {
        for (int i = 0; i < ba.size(); i++)
        {
            uint8_t const byte = (uint8_t)ba.at(i);
            if (m_executedCommandDirty)
            {
                if (m_hexVersion != byte)
                {
                    // We expect the first byte returned to be the version number
                    qDebug() << "Command has been interrupted, discarding feedback from previous command!";
                    continue;
                }
                else
                {
                    // Discard until we make sure the first feedback is correct
                    m_executedCommandDirty = false;
                }
            }
            else if (byte != 0xFF)
            {
                // All other feedback should be 0xFF
                PrintLog("Unexpected byte (" + QString::number(byte) + ") returned from serial! ", LOG_ERROR);
            }

            if (m_executeCommands.empty())
            {
                PrintLog("Unexpected extra " + QString::number(ba.size() - i) + " byte(s) returned from serial, ignoring...", LOG_ERROR);
                break;
            }
            else
            {
                // Command is finished, remove
                m_executeCommands.pop_front();

                // Check if any command is loop
                // Only set this once, reset when sending new command
                if (!m_executeCommands.isEmpty() && !m_infiniteLoop)
                {
                    CommandPair const& command = m_executeCommands.front();
                    m_infiniteLoop = (command.first.toLower() == "loop" && command.second == 0);
                }
            }
        }

        if (m_executeCommands.isEmpty())
        {
            // Finished
            if (!m_infiniteLoop)
            {
                // Signal for smart program
                emit commandFinished();
            }

            if (!m_smartProgram)
            {
                PrintLog("-------------Finished--------------");
                ui->LE_CommandSender->clear();
                ui->LE_CommandSender->setEnabled(true);
                ui->LE_CommandSender->setFocus();
                ui->PB_SendCommand->setText("Send");
                ui->PB_SendCommand->setEnabled(false);
            }
        }
        else
        {
            // Remember the next flag to display
            CommandPair const& command = m_executeCommands.front();
            m_displayFlags.push_back(m_commandToFlagMap[command.first]);
        }

        // Print the next command
        DisplayNextCommand();
    }
    m_commandMutex->unlock();
}

void RemoteControllerWindow::on_SerialPort_errorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        // Refresh the list
        on_PB_Refresh_clicked();

        SerialDisconnect();
        QMessageBox::critical(this, "Error", "Serial port disconnected unexpectedly!", QMessageBox::Ok);
    }
}

void RemoteControllerWindow::on_ReadTimer_timeout()
{
    m_serialPort.waitForReadyRead(1);

    /* This is not working for number of people, removing for now
    if (m_readTickCount > 0)
    {
        m_readTickCount--;
        if (m_readTickCount == 0 && !m_executeCommands.isEmpty())
        {
            PrintLog("Unable to receive command completion feedback, Arduino/Teensy might have fully/briefly disconnected from the Switch", LOG_ERROR);
            SerialDisconnect();
        }
    }
    */
}

//---------------------------------------------------------------------------
// Get available serial ports
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_PB_Refresh_clicked()
{
    ui->CB_SerialPorts->clear();
    for (QSerialPortInfo const& info : QSerialPortInfo::availablePorts())
    {
        ui->CB_SerialPorts->addItem(info.portName() + ": " + info.description(), info.portName());
    }

    ui->PB_Connect->setEnabled(ui->CB_SerialPorts->count() > 0);
}

void RemoteControllerWindow::on_PB_Connect_clicked()
{
    if (m_serialPort.isOpen())
    {
        SerialDisconnect();
    }
    else if (ui->CB_SerialPorts->currentIndex() != -1)
    {
        SerialConnect(ui->CB_SerialPorts->currentData().toString());
    }
}

//---------------------------------------------------------------------------
// Serial functions
//---------------------------------------------------------------------------
void RemoteControllerWindow::SerialConnect(const QString &port)
{
    m_serialPort.setPortName(port);
    m_serialPort.setBaudRate(QSerialPort::Baud9600);
    m_serialPort.setDataBits(QSerialPort::Data8);
    m_serialPort.setParity(QSerialPort::NoParity);
    m_serialPort.setStopBits(QSerialPort::OneStop);
    m_serialPort.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort.open(QIODevice::ReadWrite))
    {
        PrintLog("Serial failed to connect", LOG_ERROR);
        QMessageBox::critical(this, "Error", "Failed to connect serial port!", QMessageBox::Ok);
        m_serialState = SS_Disconnect;

        EnableSmartProgram();
    }
    else
    {
        // on_SerialPort_readyRead() will varify the feedback
        m_serialState = SS_TestFeedback;
        ui->PB_Connect->setEnabled(false);
        ui->PB_Refresh->setEnabled(false);
        ui->CB_SerialPorts->setEnabled(false);
        QTimer::singleShot(500, this, &RemoteControllerWindow::SerialConnectComplete);

        // Send a nothing command and check if it returns a feedback
        QByteArray ba;
        ba.append((char)0xFE); // mode = FE
        ba.append('0');
        ba.append((char)5); // ~240ms
        ba.append((char)0);
        for (int i = 1; i < COMMAND_MAX; i++)
        {
            ba.append('!'); // this should be invalid
            ba.append((char)0);
            ba.append((char)0);
        }
        m_serialPort.write(ba);
        m_readTimer.start(1);

        // no need to run this, will do at SerialConnectComplete()
        //EnableSmartProgram();
    }
}

void RemoteControllerWindow::SerialConnectComplete()
{
    if (m_serialState == SS_FeedbackOK)
    {
        m_serialState = SS_Connect;
        PrintLog("Serial connected (Version: " + QString::number(m_hexVersion) + ")", LOG_SUCCESS);
        ui->PB_Connect->setEnabled(true);
        ui->PB_Connect->setText("Disconnect");
        ui->PB_Refresh->setEnabled(false);
        ui->CB_SerialPorts->setEnabled(false);
        ui->LE_CommandSender->setEnabled(true);
        ui->PB_SendCommand->setText("Send");
        ui->PB_SendCommand->setEnabled(!ui->LE_CommandSender->text().isEmpty());

        EnableSmartProgram();
    }
    else if (m_serialState == SS_WrongVersion)
    {
        QString msg = "SmartProgram.hex version is not matching, please compile it and install with the newest version.";
        msg += "\nVersion Detected: " + QString::number(m_hexVersion);
        msg += "\nCurrent Version: " + QString::number(SMART_HEX_VERSION);
        QMessageBox::critical(this, "Error", msg, QMessageBox::Ok);
        SerialDisconnect();
    }
    else
    {
        QString msg = "Failed to receive feedback from Arduino/Teensy, make sure you have done the following:";
        msg += "\n1. Installed SmartProgram.hex to the board";
        msg += "\n2. Have correct wire connections between board and CP210x chip";
        msg += "\n3. Board connect to Switch after disconnecting other controllers";
        msg += "\n4. Restart Switch by holding power button for 5s and press restart";
        msg += "\n5. For Arduino Uno R3, you may have to remove a chip, follow manual section 1.5.6";
        msg += "\nFor more information please read the manual.";
        QMessageBox::critical(this, "Error", msg, QMessageBox::Ok);
        SerialDisconnect();
    }
}

void RemoteControllerWindow::SerialDisconnect()
{
    if (m_serialPort.isOpen())
    {
        m_commandMutex->lock();
        m_executeCommands.clear();
        m_commandMutex->unlock();

        m_readTimer.stop();
        m_serialPort.close();
        PrintLog("Serial disconnected", LOG_ERROR);
    }

    ui->PB_Connect->setEnabled(true);
    ui->PB_Connect->setText("Connect");
    ui->PB_Refresh->setEnabled(true);
    ui->CB_SerialPorts->setEnabled(true);
    ui->LE_CommandSender->setEnabled(false);
    ui->PB_SendCommand->setText("Send");
    ui->PB_SendCommand->setEnabled(false);
    m_serialState = SS_Disconnect;

    m_buttonFlag = 0;
    for (PushButtonMapping* pb : m_buttonMapList)
    {
        if (pb != m_buttonMapChange)
        {
            pb->ButtonReleased();
        }
        else
        {
            pb->ButtonMapStart();
        }
    }

    EnableSmartProgram();
}

void RemoteControllerWindow::SerialWrite(const QString &msg)
{
    if (m_serialPort.isOpen())
    {
        m_serialPort.write(msg.toLocal8Bit());
    }
    else
    {
        PrintLog("Command not sent: Serial not connected", LOG_ERROR);
    }
}

//---------------------------------------------------------------------------
// Button map signals
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_PB_A_clicked()
{
    ButtonMapToggle(ui->PB_A, "A Button");
}

void RemoteControllerWindow::on_PB_B_clicked()
{
    ButtonMapToggle(ui->PB_B, "B Button");
}

void RemoteControllerWindow::on_PB_X_clicked()
{
    ButtonMapToggle(ui->PB_X, "X Button");
}

void RemoteControllerWindow::on_PB_Y_clicked()
{
    ButtonMapToggle(ui->PB_Y, "Y Button");
}

void RemoteControllerWindow::on_PB_L_clicked()
{
    ButtonMapToggle(ui->PB_L, "L Button");
}

void RemoteControllerWindow::on_PB_R_clicked()
{
    ButtonMapToggle(ui->PB_R, "R Button");
}

void RemoteControllerWindow::on_PB_ZL_clicked()
{
    ButtonMapToggle(ui->PB_ZL, "ZL Button");
}

void RemoteControllerWindow::on_PB_ZR_clicked()
{
    ButtonMapToggle(ui->PB_ZR, "ZR Button");
}

void RemoteControllerWindow::on_PB_Plus_clicked()
{
    ButtonMapToggle(ui->PB_Plus, "Plus Button");
}

void RemoteControllerWindow::on_PB_Minus_clicked()
{
    ButtonMapToggle(ui->PB_Minus, "Minus Button");
}

void RemoteControllerWindow::on_PB_Home_clicked()
{
    ButtonMapToggle(ui->PB_Home, "Home Button");
}

void RemoteControllerWindow::on_PB_Capture_clicked()
{
    ButtonMapToggle(ui->PB_Capture, "Capture Button");
}

void RemoteControllerWindow::on_PB_LClick_clicked()
{
    ButtonMapToggle(ui->PB_LClick, "L-Stick Click");
}

void RemoteControllerWindow::on_PB_LUp_clicked()
{
    ButtonMapToggle(ui->PB_LUp, "L-Stick Up");
}

void RemoteControllerWindow::on_PB_LDown_clicked()
{
    ButtonMapToggle(ui->PB_LDown, "L-Stick Down");
}

void RemoteControllerWindow::on_PB_LLeft_clicked()
{
    ButtonMapToggle(ui->PB_LLeft, "L-Stick Left");
}

void RemoteControllerWindow::on_PB_LRight_clicked()
{
    ButtonMapToggle(ui->PB_LRight, "L-Stick Right");
}

void RemoteControllerWindow::on_PB_RClick_clicked()
{
    ButtonMapToggle(ui->PB_RClick, "R-Stick Click");
}

void RemoteControllerWindow::on_PB_RUp_clicked()
{
    ButtonMapToggle(ui->PB_RUp, "R-Stick Up");
}

void RemoteControllerWindow::on_PB_RDown_clicked()
{
    ButtonMapToggle(ui->PB_RDown, "R-Stick Down");
}

void RemoteControllerWindow::on_PB_RLeft_clicked()
{
    ButtonMapToggle(ui->PB_RLeft, "R-Stick Left");
}

void RemoteControllerWindow::on_PB_RRight_clicked()
{
    ButtonMapToggle(ui->PB_RRight, "R-Stick Right");
}

void RemoteControllerWindow::on_PB_DUp_clicked()
{
    ButtonMapToggle(ui->PB_DUp, "DPad Up");
}

void RemoteControllerWindow::on_PB_DDown_clicked()
{
    ButtonMapToggle(ui->PB_DDown, "DPad Down");
}

void RemoteControllerWindow::on_PB_DLeft_clicked()
{
    ButtonMapToggle(ui->PB_DLeft, "DPad Left");
}

void RemoteControllerWindow::on_PB_DRight_clicked()
{
    ButtonMapToggle(ui->PB_DRight, "DPad Right");
}

void RemoteControllerWindow::on_PB_MapDefault_clicked()
{
    m_PBToKeyMap.clear();
    m_keyToPBMap.clear();

    m_PBToKeyMap[ui->PB_A] = Qt::Key_Z;
    m_PBToKeyMap[ui->PB_B] = Qt::Key_X;
    m_PBToKeyMap[ui->PB_X] = Qt::Key_C;
    m_PBToKeyMap[ui->PB_Y] = Qt::Key_V;

    m_PBToKeyMap[ui->PB_L] = Qt::Key_U;
    m_PBToKeyMap[ui->PB_R] = Qt::Key_O;
    m_PBToKeyMap[ui->PB_ZL] = Qt::Key_Y;
    m_PBToKeyMap[ui->PB_ZR] = Qt::Key_P;

    m_PBToKeyMap[ui->PB_Plus] = Qt::Key_Equal;
    m_PBToKeyMap[ui->PB_Minus] = Qt::Key_Minus;
    m_PBToKeyMap[ui->PB_Home] = Qt::Key_Return;
    m_PBToKeyMap[ui->PB_Capture] = Qt::Key_Backspace;

    m_PBToKeyMap[ui->PB_LClick] = Qt::Key_Q;
    m_PBToKeyMap[ui->PB_LUp] = Qt::Key_I;
    m_PBToKeyMap[ui->PB_LDown] = Qt::Key_K;
    m_PBToKeyMap[ui->PB_LLeft] = Qt::Key_J;
    m_PBToKeyMap[ui->PB_LRight] = Qt::Key_L;

    m_PBToKeyMap[ui->PB_RClick] = Qt::Key_E;
    m_PBToKeyMap[ui->PB_RUp] = Qt::Key_W;
    m_PBToKeyMap[ui->PB_RDown] = Qt::Key_S;
    m_PBToKeyMap[ui->PB_RLeft] = Qt::Key_A;
    m_PBToKeyMap[ui->PB_RRight] = Qt::Key_D;

    m_PBToKeyMap[ui->PB_DUp] = Qt::Key_Up;
    m_PBToKeyMap[ui->PB_DDown] = Qt::Key_Down;
    m_PBToKeyMap[ui->PB_DLeft] = Qt::Key_Left;
    m_PBToKeyMap[ui->PB_DRight] = Qt::Key_Right;

    // Reverse mapping
    for (QMap<PushButtonMapping*, int>::iterator i = m_PBToKeyMap.begin(); i != m_PBToKeyMap.end(); ++i)
    {
        m_keyToPBMap[i.value()] = i.key();
    }

    // Set text
    for (PushButtonMapping* pb : m_buttonMapList)
    {
        SetButtonMapText(pb);
    }
}

//---------------------------------------------------------------------------
// Button map functions
//---------------------------------------------------------------------------
void RemoteControllerWindow::SaveButtonMapSettings()
{
    for (QMap<PushButtonMapping*, int>::iterator i = m_PBToKeyMap.begin(); i != m_PBToKeyMap.end(); ++i)
    {
        m_settings->setValue(i.key()->objectName(), i.value());
    }
}

void RemoteControllerWindow::LoadButtonMapSettings()
{
    // Note: Should've called get default button map before calling this!
    for (PushButtonMapping* pb : m_buttonMapList)
    {
        int key = m_settings->value(pb->objectName(), -1).toInt();
        if (key != -1)
        {
            UpdateButtonMap(pb, key);
        }
    }
}

void RemoteControllerWindow::ButtonMapToggle(PushButtonMapping *button, QString name)
{
    if (m_buttonMapChange == button)
    {
        // Cancel/Finish
        m_buttonMapChange->ButtonMapFinish();
        m_buttonMapChange = Q_NULLPTR;
    }
    else
    {
        // Previous button
        if (m_buttonMapChange != Q_NULLPTR)
        {
            m_buttonMapChange->ButtonMapFinish();
        }

        // Start mapping
        m_buttonMapChange = button;
        m_buttonMapChange->ButtonMapStart();
    }

    ui->L_MapWait->setVisible(m_buttonMapChange != Q_NULLPTR);
    ui->L_MapWait->setText("Press any key to map:\n" + name);
}

void RemoteControllerWindow::UpdateButtonMap(PushButtonMapping *button, int key)
{
    // Remove previous key
    if (m_PBToKeyMap.contains(button))
    {
        int keyPrev = m_PBToKeyMap[button];
        m_keyToPBMap.remove(keyPrev);
    }

    // Duplicated key
    if (m_keyToPBMap.contains(key))
    {
        PushButtonMapping* pb = m_keyToPBMap[key];
        pb->setText("");
        m_PBToKeyMap.remove(pb);
        m_keyToPBMap.remove(key);
    }

    // Set current key mapping if key is valid
    if (key != -1)
    {
        m_PBToKeyMap[button] = key;
        m_keyToPBMap[key] = button;
    }

    SetButtonMapText(button);
}

void RemoteControllerWindow::SetButtonMapText(PushButtonMapping *button)
{
    if (!m_PBToKeyMap.contains(button))
    {
        button->setText("");
        return;
    }

    int key = m_PBToKeyMap[button];
    QString keyString = QKeySequence(key).toString();
    if (key == Qt::Key_Alt)
    {
        keyString = "Alt";
    }
    else if (key == Qt::Key_Shift)
    {
        keyString = "Shift";
    }
    else if (key == Qt::Key_Backspace)
    {
        keyString = "BSpace";
    }
    else if (key == 16777249) // Ctrl
    {
        keyString = "Ctrl";
    }

    button->setText(keyString);
}

void RemoteControllerWindow::UpdateButtonFlags(int key, bool pressed)
{
    if (!m_keyToPBMap.contains(key))
    {
        return;
    }

    // Highlight button
    PushButtonMapping* pb = m_keyToPBMap[key];
    if (pressed)
    {
        pb->ButtonPressed();
    }
    else
    {
        pb->ButtonReleased();
    }

    // Set flag
    ButtonFlag buttonFlagPrev = m_buttonFlag;
    ButtonFlag flag = m_PBToFlagMap[pb];
    if (pressed)
    {
        m_buttonFlag |= flag;
    }
    else
    {
        m_buttonFlag &= ~flag;
    }

    // No change
    if (m_buttonFlag == buttonFlagPrev)
    {
        return;
    }

    SendButtonFlag();
}

void RemoteControllerWindow::ClearButtonFlags()
{
    m_buttonFlag = 0;
    for (PushButtonMapping* pb : m_buttonMapList)
    {
        pb->ButtonReleased();
    }

    SendButtonFlag();
}

void RemoteControllerWindow::SendButtonFlag()
{
    // Send button signal, same button shouldn't be sent again
    if (m_serialPort.isOpen())
    {
        QByteArray ba;
        ba.append((char)0xFF); // mode = FF
        ba.append((char)(m_buttonFlag & 0x000000FF));
        ba.append((char)((m_buttonFlag & 0x0000FF00) >> 8));
        ba.append((char)((m_buttonFlag & 0x00FF0000) >> 16));
        ba.append((char)((m_buttonFlag & 0xFF000000) >> 24));
        m_serialPort.write(ba);
    }
}

//---------------------------------------------------------------------------
// Command sender signals
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_LE_CommandSender_textChanged(const QString &arg1)
{
    // Enable if text is not empty, or some command is running
    ui->PB_SendCommand->setEnabled(!m_smartProgram && (!arg1.isEmpty() || !m_executeCommands.isEmpty()));
}

void RemoteControllerWindow::on_LE_CommandSender_returnPressed()
{
    m_pauseKeyEventFilter = true;
    on_PB_SendCommand_clicked();
    m_pauseKeyEventFilter = false;
}

void RemoteControllerWindow::on_PB_SendCommand_clicked()
{
    if (!ui->PB_SendCommand->isEnabled()) return;

    if (m_smartProgram)
    {
        PrintLog("Cannot send custom command while Smart Program is running", LOG_ERROR);
        return;
    }

    if (m_executeCommands.isEmpty())
    {

        if (SendCommand(ui->LE_CommandSender->text()))
        {
            if (!m_executeCommands.isEmpty())
            {
                ui->LE_CommandSender->setEnabled(false);
                ui->PB_SendCommand->setText("Stop");
            }
        }
    }
    else
    {
        ClearButtonFlags();

        m_commandMutex->lock();
        m_infiniteLoop = false;
        m_executeCommands.clear();
        m_commandMutex->unlock();

        ui->LE_CommandSender->setEnabled(true);
        ui->PB_SendCommand->setText("Send");
        PrintLog("Command stopped by user", LOG_WARNING);
    }
}

void RemoteControllerWindow::on_PB_SaveLog_clicked()
{
    SaveLog();
}

void RemoteControllerWindow::on_PB_ClearLog_clicked()
{
    ui->TB_Log->clear();

    m_logCount = 0;
    m_successCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;
    UpdateLogStat();
}

bool RemoteControllerWindow::SendCommand(const QString &commands)
{
    if (!m_serialPort.isOpen())
    {
        PrintLog("Command not sent: Serial not connected", LOG_ERROR);
        return false;
    }

    // Validate command
    QString errorMsg;
    if (!SmartProgramBase::validateCommand(commands, errorMsg))
    {
        PrintLog(errorMsg, LOG_ERROR);
        return false;
    }

    // We assume command is valid after here
    m_commandMutex->lock();
    m_infiniteLoop = false;
    m_displayFlags.clear();
    m_executedCommandDirty = true;
    m_executeCommands.clear();
    QByteArray ba;
    ba.append((char)0xFE); // mode = FE

    QStringList list = commands.split(',');
    CommandPair command;
    for (int i = 0; i < list.size(); i++)
    {
        QString const& str = list[i];
        if (i % 2 == 0)
        {
            // Command name
            QString commandLower = str.toLower();
            if (commandLower == "aspam")
            {
                command.first = "ASpam";
                ba.append('v');
            }
            else if (commandLower == "bspam")
            {
                command.first = "BSpam";
                ba.append('b');
            }
            else if (commandLower == "loop")
            {
                command.first = "Loop";
                ba.append('n');
            }
            else
            {
                for (QMap<QString, ButtonFlag>::iterator iter = m_commandToFlagMap.begin(); iter != m_commandToFlagMap.end(); iter++)
                {
                    if (commandLower == iter.key().toLower())
                    {
                        command.first = iter.key();
                        ba.append(m_flagToCharMap[iter.value()]);
                        break;
                    }
                }
            }
        }
        else
        {
            // Duration
            uint duration = str.toUInt();
            command.second = static_cast<uint16_t>(duration);
            m_executeCommands.push_back(command);
            ba.append((char)(duration & 0x00FF));
            ba.append((char)((duration & 0xFF00) >> 8));

            // Infinite loop, ignore everything afterwards
            if (command.first.toLower() == "loop" && command.second == 0 && i < list.size() - 1)
            {
                PrintLog("Commands after [Loop,0] will be discarded!", LOG_ERROR);
                break;
            }
        }
    }

    // Fill the remaining spaces with null command
    if (m_executeCommands.size() < COMMAND_MAX)
    {
        for (int i = 0; i < (COMMAND_MAX - m_executeCommands.size()); i++)
        {
            ba.append('!'); // this should be invalid
            ba.append((char)0);
            ba.append((char)0);
        }
    }

    // Send to serial
    m_serialPort.write(ba);
    ui->LE_CommandSender->SaveCommand();
    ui->LE_CommandSender->clear();

    // Only print manually sent commands
    if (!m_smartProgram)
    {
        PrintLog("Command sent: [" + commands + "]");
    }

    // Duplicate loop commands if duration is non-zero
    QVector<CommandPair> tempLoopCommands;
    for (int i = 0; i < m_executeCommands.size(); i++)
    {
        CommandPair const& c = m_executeCommands[i];
        tempLoopCommands.push_back(c);

        if (c.first.toLower() == "loop")
        {
            i++;
            if (c.second > 1)
            {
                // Copy loop duration - 1 times
                for (uint16_t j = c.second - 1; j > 0; j--)
                {
                    for (CommandPair const& copy : tempLoopCommands)
                    {
                        if (copy.first.toLower() == "loop")
                        {
                            // Loop duration goes down one by one
                            // [...,Loop,3,...,Loop,2,...,Loop,1]
                            m_executeCommands.insert(i, CommandPair(copy.first, j));
                        }
                        else
                        {
                            m_executeCommands.insert(i, copy);
                        }
                        i++;
                    }
                }
            }
            i--;

            tempLoopCommands.clear();
        }
    }

    // Display commands on virtual controller
    CommandPair const& firstCommand = m_executeCommands.front();
    m_displayFlags.push_back(m_commandToFlagMap[firstCommand.first]);
    DisplayNextCommand();
    m_commandMutex->unlock();

    return true;
}

//---------------------------------------------------------------------------
// Camera slots
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_RB_DetectedDevice_clicked()
{
    ui->GB_DetectedDevice->setHidden(false);
    ui->GB_CustomDevice->setHidden(true);
}

void RemoteControllerWindow::on_RB_CustomDevice_clicked()
{
    ui->GB_DetectedDevice->setHidden(true);
    ui->GB_CustomDevice->setHidden(false);
}

void RemoteControllerWindow::on_PB_RefreshCamera_clicked()
{
    ui->CB_Cameras->clear();
    for (QCameraInfo const& info : QCameraInfo::availableCameras())
    {
        ui->CB_Cameras->addItem(info.description());
    }

    ui->PB_StartCamera->setEnabled(ui->CB_Cameras->count() > 0);
}

void RemoteControllerWindow::on_PB_StartCamera_clicked()
{
    if (m_vlcWrapper->isStarted())
    {
        CameraToggle(false);
    }
    else if ((ui->RB_DetectedDevice->isChecked() && ui->CB_Cameras->currentIndex() != -1)
          || (ui->RB_CustomDevice->isChecked() && !ui->LE_CustomVideo->text().isEmpty()))
    {
        CameraToggle(true);
    }
}

void RemoteControllerWindow::on_PB_Screenshot_clicked()
{
    CameraCaptureToFile();
}

void RemoteControllerWindow::on_PB_AdjustVideo_clicked()
{
    m_videoEffectSetting->show();
    m_videoEffectSetting->raise();
}

void RemoteControllerWindow::on_VLCState_changed(VLCWrapper::VLCState state)
{
    switch (state)
    {
    case VLCWrapper::VLCState::OPENING:
    {
        PrintLog("Starting Camera...");
        break;
    }
    case VLCWrapper::VLCState::PLAYING:
    {
        PrintLog("Camera on", LOG_SUCCESS);
        ui->PB_StartCamera->setText("Stop Camera");
        ui->PB_StartCamera->setEnabled(true);
        EnableSmartProgram();
        break;
    }
    case VLCWrapper::VLCState::ENDED:
    case VLCWrapper::VLCState::ERROR:
    {
        CameraToggle(false);
        PrintLog("Failed to start camera", LOG_ERROR);
        QMessageBox::critical(this, "Error", "An error occured when starting camera!", QMessageBox::Ok);
        break;
    }
    default: break;
    }
}

void RemoteControllerWindow::on_CB_AudioDisplayMode_currentIndexChanged(int index)
{
    switch (index)
    {
    case ADM_RawWave:
    {
        ui->L_AudioSamples->setHidden(false);
        ui->SB_AudioSamples->setHidden(false);

        ui->L_AudioFreq->setHidden(true);
        ui->SB_AudioFreqLow->setHidden(true);
        ui->SB_AudioFreqHigh->setHidden(true);
        break;
    }
    case ADM_FreqBars:
    case ADM_Spectrogram:
    {
        ui->L_AudioSamples->setHidden(true);
        ui->SB_AudioSamples->setHidden(true);

        ui->L_AudioFreq->setHidden(false);
        ui->SB_AudioFreqLow->setHidden(false);
        ui->SB_AudioFreqHigh->setHidden(false);
        break;
    }
    }

    if (m_actionBreakLayout && m_actionBreakLayout->isChecked())
    {
        if (ui->CB_AudioDisplayMode->currentIndex() == ADM_None)
        {
            setFixedSize(700,560);
        }
        else
        {
            setFixedSize(700,670);
        }
    }
}

void RemoteControllerWindow::on_SB_AudioFreqLow_valueChanged(int arg1)
{
    ui->SB_AudioFreqHigh->setMinimum(arg1 + 100);
}

void RemoteControllerWindow::on_SB_AudioFreqHigh_valueChanged(int arg1)
{
    ui->SB_AudioFreqLow->setMaximum(arg1 - 100);
}

//---------------------------------------------------------------------------
// Camera functions
//---------------------------------------------------------------------------
void RemoteControllerWindow::CameraToggle(bool on)
{
    if (on)
    {
        bool enabled = false;
        if (ui->RB_DetectedDevice->isChecked())
        {
            QString const device = ui->CB_Cameras->currentText();
            enabled = m_vlcWrapper->start(device, device);
        }
        else if (ui->RB_CustomDevice->isChecked())
        {
            enabled = m_vlcWrapper->start(ui->LE_CustomVideo->text(), ui->LE_CustomAudio->text());
        }

        if (enabled)
        {
            ui->PB_StartCamera->setText("Starting...");
            ui->PB_StartCamera->setEnabled(false);

            ui->RB_DetectedDevice->setEnabled(false);
            ui->RB_CustomDevice->setEnabled(false);
            ui->GB_DetectedDevice->setEnabled(false);
            ui->GB_CustomDevice->setEnabled(false);
            ui->PB_Screenshot->setEnabled(true);
            ui->PB_AdjustVideo->setEnabled(true);
            ui->L_NoVideo->setHidden(true);

            // set initial HSC
            m_vlcWrapper->setHue(m_videoEffectSetting->getHue());
            m_vlcWrapper->setSaturation(m_videoEffectSetting->getSaturation());
            m_vlcWrapper->setGamma(m_videoEffectSetting->getGamma());
        }
        else
        {
            QMessageBox::critical(this, "Error", "An error occured when starting camera!", QMessageBox::Ok);
        }
    }
    else
    {
        if (m_vlcWrapper->isStarted())
        {
            m_vlcWrapper->stop();
            PrintLog("Camera off");
        }

        ui->PB_StartCamera->setText("Start Camera");
        ui->PB_StartCamera->setEnabled(true);

        ui->RB_DetectedDevice->setEnabled(true);
        ui->RB_CustomDevice->setEnabled(true);
        ui->GB_DetectedDevice->setEnabled(true);
        ui->GB_CustomDevice->setEnabled(true);
        ui->PB_Screenshot->setEnabled(false);
        ui->PB_AdjustVideo->setEnabled(false);
        ui->L_NoVideo->setHidden(false);

        if (m_smartSetting->isVisible())
        {
            m_smartSetting->close();
        }

        if (m_videoEffectSetting->isVisible())
        {
            m_videoEffectSetting->close();
        }
    }

    EnableSmartProgram();
}

void RemoteControllerWindow::CameraCaptureToFile(QString name)
{
    QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + name + ".png";

    // Grab frame and save from QT draw
    QImage frame;
    m_vlcWrapper->getVideoManager()->getFrame(frame);
    frame.save(SCREENSHOT_PATH + nameWithTime);
    PrintLog("Screenshot saved: " + nameWithTime);
}

//---------------------------------------------------------------------------
// Pop out
//---------------------------------------------------------------------------
void RemoteControllerWindow::PopOut()
{
    {
        m_actionSmartProgram->setEnabled(true);
        m_popOutSmartProgram = new QDialog();
        m_popOutSmartProgram->setFont(font());
        m_popOutSmartProgram->setWindowTitle(ui->GB_SmartProgram->title());
        m_popOutSmartProgram->setWindowFlags(m_popOutSmartProgram->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        ui->GB_SmartProgram->setTitle("");
        QVBoxLayout* vBoxLayout = new QVBoxLayout();
        vBoxLayout->addItem(ui->scrollAreaWidgetContents_16->layout()->takeAt(1));
        vBoxLayout->addItem(ui->VL_Camera->takeAt(4));
        m_popOutSmartProgram->setLayout(vBoxLayout);
        ActionSmartProgram_triggered();
        m_popOutSmartProgram->move(m_settings->value("PopOutSmartProgramPos", m_popOutSmartProgram->pos()).toPoint());
        m_popOutSmartProgram->resize(m_settings->value("PopOutSmartProgramSize", m_popOutSmartProgram->size()).toSize());
    }

    {
        m_actionCommandLog->setEnabled(true);
        m_popOutCommandLog = new QDialog();
        m_popOutCommandLog->setFont(font());
        m_popOutCommandLog->setWindowTitle("Command Sender & Log");
        m_popOutCommandLog->setWindowFlags(m_popOutCommandLog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        ui->GB_Log->setTitle("");
        ui->GB_Log->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QVBoxLayout* vBoxLayout = new QVBoxLayout();
        vBoxLayout->addWidget(ui->GB_Log);
        m_popOutCommandLog->setLayout(vBoxLayout);
        ActionCommandLog_triggered();
        m_popOutCommandLog->move(m_settings->value("PopOutCommandLogPos", m_popOutCommandLog->pos()).toPoint());
        m_popOutCommandLog->resize(m_settings->value("PopOutCommandLogSize", m_popOutCommandLog->size()).toSize());
    }

    {
        m_actionController->setCheckable(false);
        m_popOutController = new QDialog();
        m_popOutController->setFont(font());
        m_popOutController->setWindowTitle(ui->GB_Controller->title());
        m_popOutController->setWindowFlags(m_popOutController->windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
        ui->GB_Controller->setTitle("");
        QVBoxLayout* vBoxLayout = new QVBoxLayout();
        vBoxLayout->addWidget(ui->GB_Controller);
        m_popOutController->setLayout(vBoxLayout);

        if (m_settings->value("ShowPopOutController", true).toBool())
        {
            ActionController_triggered();
            m_popOutController->move(m_settings->value("PopOutControllerPos", m_popOutController->pos()).toPoint());
        }
    }

    this->move(m_settings->value("PopOutMainWindow", this->pos()).toPoint());

    // move serial port layout to above camera
    ui->VL_Camera->insertWidget(0, ui->HL_SerialPorts);

    delete ui->line;
    delete ui->frame;
    if (ui->CB_AudioDisplayMode->currentIndex() == ADM_None)
    {
        setFixedSize(700,560);
    }
    else
    {
        setFixedSize(700,670);
    }
}

void RemoteControllerWindow::DeletePopOut()
{
    if (m_smartSetting->isVisible())
    {
        m_smartSetting->close();
    }

    if (m_videoEffectSetting->isVisible())
    {
        m_videoEffectSetting->close();
    }

    if (m_discordSettings->isVisible())
    {
        m_discordSettings->close();
    }

    if (m_popOutSmartProgram && m_popOutSmartProgram->isVisible())
    {
        m_settings->setValue("PopOutSmartProgramPos", m_popOutSmartProgram->pos());
        m_settings->setValue("PopOutSmartProgramSize", m_popOutSmartProgram->size());
        m_popOutSmartProgram->close();
    }

    if (m_popOutCommandLog && m_popOutCommandLog->isVisible())
    {
        m_settings->setValue("PopOutCommandLogPos", m_popOutCommandLog->pos());
        m_settings->setValue("PopOutCommandLogSize", m_popOutCommandLog->size());
        m_popOutCommandLog->close();
    }

    if (m_popOutController && m_popOutController->isVisible())
    {
        m_settings->setValue("ShowPopOutController", true);
        m_settings->setValue("PopOutControllerPos", m_popOutController->pos());
        m_popOutController->close();
    }
    else
    {
        m_settings->setValue("ShowPopOutController", m_actionController->isChecked());
    }

    m_settings->setValue("PopOutMainWindow", this->pos());
}

//---------------------------------------------------------------------------
// Smart program slots
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_SP1_SB_X_valueChanged(int arg1)
{
    ui->SP1_SB_Width->setMaximum(1280 - arg1);
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Y_valueChanged(int arg1)
{
    ui->SP1_SB_Height->setMaximum(720 - arg1);
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Width_valueChanged(int arg1)
{
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Height_valueChanged(int arg1)
{
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP2_SB_X_valueChanged(int arg1)
{
    QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP2_SB_Y_valueChanged(int arg1)
{
    QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
    m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP5_CB_Raid_toggled(bool checked)
{
    ui->SP5_SB_Skips->setEnabled(!checked);
    ui->SP5_LE_PokemonList->setEnabled(checked);
}

void RemoteControllerWindow::on_SP6_CB_Skips_clicked()
{
    ui->SP6_SB_Skips->setEnabled(ui->SP6_CB_Skips->isChecked());
}

void RemoteControllerWindow::on_SP9_CB_Mode_currentIndexChanged(int index)
{
    SmartSVEggOperation::EggOperationType type = SmartSVEggOperation::EggOperationType(index);
    ui->SP9_SB_Sandwich->setEnabled(type == SmartSVEggOperation::EggOperationType::EOT_Collector);
    ui->SP9_CB_Sound->setEnabled(type != SmartSVEggOperation::EggOperationType::EOT_Collector);
    ui->SP9_SB_Column->setEnabled(type == SmartSVEggOperation::EggOperationType::EOT_Hatcher);
    ui->SP9_CB_HatchSandwich->setEnabled(type != SmartSVEggOperation::EggOperationType::EOT_Collector);
    ui->SP9_CB_ShinySave->setEnabled(type != SmartSVEggOperation::EggOperationType::EOT_Collector);
    ui->SP9_CB_BackupSave->setEnabled(type == SmartSVEggOperation::EggOperationType::EOT_Shiny);
    ui->SP9_CB_Wingull->setEnabled(type != SmartSVEggOperation::EggOperationType::EOT_Collector);
}

void RemoteControllerWindow::on_SP19_SP_Count_valueChanged(int arg1)
{
    ui->SP19_LE_Code->setMaxLength(arg1);
}

void RemoteControllerWindow::on_SP19_CB_OCR_toggled(bool checked)
{
    ui->SP19_LE_Code->clear();
    ui->SP19_LE_Code->setEnabled(!checked);

    ui->SP19_SB_X->setEnabled(checked);
    ui->SP19_SB_Y->setEnabled(checked);
    ui->SP19_SB_Width->setEnabled(checked);
    ui->SP19_SB_Height->setEnabled(checked);
    ui->SP19_SP_Select->setEnabled(checked);
}

void RemoteControllerWindow::on_SP19_SP_Select_clicked()
{
    SelectorWidget w;
    if (w.exec() == QDialog::Accepted)
    {
        ui->SP19_SB_X->setValue(w.selectedRect.x());
        ui->SP19_SB_Y->setValue(w.selectedRect.y());
        ui->SP19_SB_Width->setValue(w.selectedRect.width());
        ui->SP19_SB_Height->setValue(w.selectedRect.height());
    }
}

void RemoteControllerWindow::on_SP20_CB_Mode_currentIndexChanged(int index)
{
    SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(ui->LW_SmartProgram->currentItem()->text());

    if (sp == SP_EggOperation)
    {
        SmartEggOperation::EggOperationType type = SmartEggOperation::EggOperationType(index);
        ui->SP20_SB_Collect->setEnabled(type == SmartEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_SB_Column->setEnabled(type == SmartEggOperation::EggOperationType::EOT_Hatcher);
        ui->SP20_CB_HatchExtra->setEnabled(type != SmartEggOperation::EggOperationType::EOT_Collector && type != SmartEggOperation::EggOperationType::EOT_Remainder && type != SmartEggOperation::EggOperationType::EOT_Parent && type != SmartEggOperation::EggOperationType::EOT_Multi);
        ui->SP20_RB_ShinyDisable->setEnabled(type != SmartEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_RB_ShinySound->setEnabled(type != SmartEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_RB_ShinyDelay->setEnabled(type != SmartEggOperation::EggOperationType::EOT_Collector && type != SmartEggOperation::EggOperationType::EOT_Remainder);
        if (type == SmartEggOperation::EggOperationType::EOT_Remainder && ui->SP20_RB_ShinyDelay->isChecked())
        {
            ui->SP20_RB_ShinyDisable->setChecked(true);
        }
        ui->SP20_TW_Keep->setEnabled(type != SmartEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_CB_ParentGender->setEnabled(type == SmartEggOperation::EggOperationType::EOT_Parent);

        switch (type)
        {
        case SmartEggOperation::EggOperationType::EOT_Shiny:
            ui->SP20_TW_Keep->SetMode(PokemonStatTableWidget::Mode::Shiny);
            break;
        case SmartEggOperation::EggOperationType::EOT_Multi:
            ui->SP20_TW_Keep->SetMode(PokemonStatTableWidget::Mode::Multi);
            ui->SP20_CB_HatchExtra->setChecked(true);
            break;
        case SmartEggOperation::EggOperationType::EOT_Parent:
            ui->SP20_TW_Keep->SetMode(PokemonStatTableWidget::Mode::Parent);
            break;
        default:
            ui->SP20_TW_Keep->SetMode(PokemonStatTableWidget::Mode::Default);
            break;
        }
    }
    else if (sp == SP_BDSP_EggOperation)
    {
        SmartBDSPEggOperation::EggOperationType type = SmartBDSPEggOperation::EggOperationType(index);
        if (type >= SmartBDSPEggOperation::EggOperationType::EOT_COUNT)
        {
            ui->SP20_CB_Mode->setCurrentIndex(0);
            return;
        }

        if (ui->SP20_RB_ShinyDelay->isChecked())
        {
            ui->SP20_RB_ShinyDisable->setChecked(true);
        }

        ui->SP20_SB_Collect->setEnabled(type == SmartBDSPEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_SB_Column->setEnabled(type == SmartBDSPEggOperation::EggOperationType::EOT_Hatcher);
        ui->SP20_CB_HatchExtra->setEnabled(false);
        ui->SP20_RB_ShinyDisable->setEnabled(type != SmartBDSPEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_RB_ShinySound->setEnabled(type != SmartBDSPEggOperation::EggOperationType::EOT_Collector);
        ui->SP20_RB_ShinyDelay->setEnabled(false);
        ui->SP20_TW_Keep->setEnabled(type == SmartBDSPEggOperation::EggOperationType::EOT_Shiny);
        ui->SP20_TW_Keep->SetMode(PokemonStatTableWidget::Mode::SingleShiny);
        ui->SP20_CB_ParentGender->setEnabled(false);
    }
}

void RemoteControllerWindow::on_SP22_SB_Seed_valueChanged(const QString &arg1)
{
    QDateTime temp;
    temp.setTimeSpec(Qt::UTC);
    temp.setDate(QDate(1970,1,1));
    temp.setTime(QTime(0,0));
    temp = temp.addSecs(arg1.toLongLong());
    ui->SP22_DateTime->setDateTime(temp);
}

void RemoteControllerWindow::on_SP22_DateTime_dateTimeChanged(const QDateTime &dateTime)
{
    QDateTime from, to;
    from.setTimeSpec(Qt::UTC);
    from.setDate(QDate(1970,1,1));
    from.setTime(QTime(0,0));
    to.setTimeSpec(Qt::UTC);
    to.setDate(dateTime.date());
    to.setTime(dateTime.time());
    ui->SP22_SB_Seed->setValue(static_cast<double>(from.secsTo(to)));
}

void RemoteControllerWindow::on_SoundDetection_required(int min, int max)
{
    ui->CB_AudioDisplayMode->setCurrentIndex(ADM_Spectrogram);
    ui->CB_AudioDisplayMode->setEnabled(false);
    ui->SB_AudioFreqLow->setEnabled(false);
    ui->SB_AudioFreqHigh->setEnabled(false);

    // Set min twice in case min & max both larger than previous max
    ui->SB_AudioFreqLow->setValue(min);
    ui->SB_AudioFreqHigh->setValue(max);
    ui->SB_AudioFreqLow->setValue(min);
}

void RemoteControllerWindow::on_SmartProgram_printLog(const QString log, QColor color)
{
    if (!CanRunSmartProgram() || !m_smartProgram)
    {
        PrintLog(log, color);
        return;
    }

    PrintLog("[" + m_smartProgram->getProgramName() + "]: " + log, color);

    QString const logFileName = m_smartProgram->getLogFileName();
    if (!logFileName.isEmpty())
    {
        QFile file(LOG_PATH + logFileName);
        if(file.open(QIODevice::Append))
        {
            QTextStream stream(&file);

            QString tag;
            if (color == LOG_SUCCESS) tag = "[SUCCESS] ";
            if (color == LOG_WARNING) tag = "[WARNING] ";
            if (color == LOG_ERROR) tag = "[ERROR] ";
            stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss - ") + tag + log + "\n";

            file.close();
        }
    }
}

void RemoteControllerWindow::on_SmartProgram_completed()
{
    if (!CanRunSmartProgram()) return;

    SmartProgram sp = m_smartProgram->getProgramEnum();
    switch (sp)
    {
    default: break;
    }

    // Tell user where log file has been saved
    QString const logFileName = m_smartProgram->getLogFileName();
    if (!logFileName.isEmpty())
    {
        PrintLog("Log file saved: " + logFileName);
    }

    delete m_smartProgram;
    m_smartProgram = Q_NULLPTR;

    // Play sound
    if (m_smartSetting->isSoundEnabled())
    {
        m_smartSetting->playSound();
    }

    // Call this to enable UI
    StopSmartProgram();
}

void RemoteControllerWindow::on_SmartProgram_runSequence(const QString sequence)
{
    if (!CanRunSmartProgram())
    {
        StopSmartProgram();
        return;
    }

    if (!SendCommand(sequence))
    {
        PrintLog("[" + m_smartProgram->getProgramName() + "]: Failed to run command");
        StopSmartProgram();
        return;
    }
}

void RemoteControllerWindow::on_PB_StartSmartProgram_clicked()
{
    if (!CanRunSmartProgram()) return;

    if (m_smartProgram)
    {
        StopSmartProgram();
        return;
    }

    // Check for black border
    QImage frame;
    m_vlcWrapper->getVideoManager()->getFrame(frame);
    bool border = true;
    for (int y = 0; y < frame.height(); y++)
    {
        if ((frame.pixel(0,y) & 0x00FFFFFF) != 0 || (frame.pixel(frame.width() - 1,y) & 0x00FFFFFF) != 0) // Not black
        {
            border = false;
            break;
        }
    }

    if (border)
    {
        QMessageBox::critical(this, "Error", "Black border detected, please go to Switch Settings->TV Settings->Adjust Screen Size and set to 100%.");
        return;
    }

    QString name = ui->LW_SmartProgram->currentItem()->text();
    SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(name);
    if (sp == SP_COUNT)
    {
        QMessageBox::critical(this, "Error", "No code exist for this Smart Program!");
        return;
    }
    else
    {
        RunSmartProgram(sp);
    }
}

void RemoteControllerWindow::on_PB_SmartSettings_clicked()
{
    m_smartSetting->show();
    m_smartSetting->raise();
}

void RemoteControllerWindow::on_PB_ModifySmartCommands_clicked()
{
    QString name = ui->LW_SmartProgram->currentItem()->text();
    SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(name);
    QString programName = SmartProgramBase::getProgramInternalNameFromEnum(sp);
    if (QFile::exists(SMART_COMMAND_PATH + programName + ".xml"))
    {
        system((QString("start notepad++ ") + SMART_COMMAND_PATH + programName + ".xml").toStdString().c_str());
    }
    else if (QFile::exists(SMART_COMMAND_PATH + programName + ".ini"))
    {
        system((QString("start notepad++ ") + SMART_COMMAND_PATH + programName + ".ini").toStdString().c_str());
    }
}

void RemoteControllerWindow::on_PB_EditStats_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(SMART_STATS_INI));
}

void RemoteControllerWindow::on_PB_ResetStats_clicked()
{
    if (ui->L_CurrentStats->text() == "N/A") return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Reset Stats", "This will reset all stats for current program to 0, continue?\nIf you want to edit individual stats, press Edit.", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(ui->LW_SmartProgram->currentItem()->text());
        UpdateStats(sp, true);
    }
}

void RemoteControllerWindow::EnableResetStats(bool enabled)
{
    ui->PB_ResetStats->setEnabled(enabled);
}

void RemoteControllerWindow::on_LW_SmartProgram_currentTextChanged(const QString &currentText)
{
    SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(currentText);
    if (sp == SP_COUNT)
    {
        QMessageBox::critical(this, "Error", "Missing mapping for this Smart Program!");
        m_vlcWrapper->getVideoManager()->setDefaultAreaEnabled(false);
        return;
    }

    QString programName = SmartProgramBase::getProgramInternalNameFromEnum(sp);
    ui->PB_ModifySmartCommands->setEnabled(QFile::exists(SMART_COMMAND_PATH + programName + ".xml") || QFile::exists(SMART_COMMAND_PATH + programName + ".ini"));

    int tabIndex = SmartProgramBase::getProgramTabID(sp);
    if (tabIndex < 0 || tabIndex >= ui->SW_Settings->count())
    {
        QMessageBox::critical(this, "Error", "Invalid tab ID for this Smart Program!");
        m_vlcWrapper->getVideoManager()->setDefaultAreaEnabled(false);
        return;
    }
    ui->SW_Settings->setCurrentIndex(tabIndex);

    // Enable capture area preview
    bool useArea = false;
    switch (sp)
    {
    case SP_BrightnessMeanFinder:
    {
        useArea = true;
        QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
        m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
        break;
    }
    case SP_PurpleBeamFinder:
    {
        useArea = true;
        QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
        m_vlcWrapper->getVideoManager()->setDefaultArea(rect);
        break;
    }
    case SP_MaxRaidBattler:
    {
        ui->SPGeneric1_Note->setText("");
        ui->SPGeneric1_Label->setText("Wishing Pieces to Spend:");
        ui->SPGeneric1_Count->setRange(1,999);
        break;
    }
    case SP_EggOperation:
    {
        on_SP20_CB_Mode_currentIndexChanged(ui->SP20_CB_Mode->currentIndex());
        break;
    }
    case SP_BDSP_BoxDuplication:
    {
        ui->SPGeneric1_Note->setText("Note: You MUST have Menu Glitch active, which has been patched in the newest version.\nThis is for duplicating Pokemon, for items, use Box Operation program instead.");
        ui->SPGeneric1_Label->setText("No. of Boxes to Duplicate:");
        ui->SPGeneric1_Count->setRange(1,39);
        break;
    }
    case SP_BDSP_EggOperation:
    {
        on_SP20_CB_Mode_currentIndexChanged(ui->SP20_CB_Mode->currentIndex());
        break;
    }
    case SP_SV_ItemDuplication:
    {
        ui->SPGeneric1_Note->setText("Note: This requires you to have duplicated Koraidon/Miraidon before v1.1.0.");
        ui->SPGeneric1_Label->setText("No. of Items:");
        ui->SPGeneric1_Count->setRange(1,998);
        break;
    }
    case SP_SV_SurpriseTrade:
    {
        ui->SPGeneric1_Note->setText("");
        ui->SPGeneric1_Label->setText("No. of Pokemon to Trade:");
        ui->SPGeneric1_Count->setRange(1,960);
        break;
    }
    case SP_SV_BoxRelease:
    {
        ui->SPGeneric1_Note->setText("Note: Pokemon sort from left to right, top to bottom");
        ui->SPGeneric1_Label->setText("No. of Pokemon to Release:");
        ui->SPGeneric1_Count->setRange(1,960);
        break;
    }
    case SP_BoxRelease:
    case SP_SV_BoxReleaseSafe:
    {
        ui->SPGeneric1_Note->setText("");
        ui->SPGeneric1_Label->setText("No. of Boxes to Release:");
        ui->SPGeneric1_Count->setRange(1,32);
        break;
    }
    case SP_TOTK_BowFuseDuplication:
    {
        ui->SPGeneric1_Note->setText("Inconsistent, will not duplicate exact target amount. This is patched after v1.1.0");
        ui->SPGeneric1_Label->setText("Target No. of Duplication:");
        ui->SPGeneric1_Count->setRange(1,999);
        break;
    }
    case SP_TOTK_MineruDuplication:
    case SP_TOTK_ShieldSurfDuplication:
    case SP_TOTK_ZonaiDeviceDuplication:
    {
        ui->SPGeneric1_Note->setText("Duplicate amount is " + QString(sp == SP_TOTK_ZonaiDeviceDuplication ? "10" : "5") + "x the target. This is patched after v1.1.0");
        ui->SPGeneric1_Label->setText("Target No. of Duplication:");
        ui->SPGeneric1_Count->setRange(1,2000);
        break;
    }
    default: break;
    }

    m_vlcWrapper->getVideoManager()->setDefaultAreaEnabled(useArea);
    UpdateStats(sp);
}

void RemoteControllerWindow::on_CB_SmartProgram_currentIndexChanged(int index)
{
    for (int row = 0; row < ui->LW_SmartProgram->count(); ++row)
    {
        QListWidgetItem* item = ui->LW_SmartProgram->item(row);
        SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(item->text());
        item->setHidden(ui->CB_SmartProgram->currentText() != SmartProgramBase::getProgramGamePrefix(sp));
    }

    // Force set to first smart program in the list
    for (int row = 0; row < ui->LW_SmartProgram->count(); ++row)
    {
        QListWidgetItem const* item = ui->LW_SmartProgram->item(row);
        if (!item->isHidden())
        {
            ui->SW_Settings->setEnabled(true);
            ui->LW_SmartProgram->setCurrentRow(row);
            return;
        }
    }

    // If we are here, no program exist for this game
    ui->L_CurrentStats->setText("N/A");
    ui->PB_ResetStats->setEnabled(false);
    ui->SW_Settings->setEnabled(false);
}

void RemoteControllerWindow::ActionBreakLayout_triggered()
{
    if (m_actionBreakLayout->isChecked())
    {
        m_settings->setValue("PopOut", true);
        PopOut();
    }
    else
    {
        QMessageBox::information(this, "Break Layout", "Next restart of Smart Program Manager will return to original layout.");
        m_settings->setValue("PopOut", false);
        m_actionBreakLayout->setChecked(true);
    }
}

void RemoteControllerWindow::ActionSmartProgram_triggered()
{
    m_popOutSmartProgram->show();
    m_popOutSmartProgram->raise();
}

void RemoteControllerWindow::ActionCommandLog_triggered()
{
    m_popOutCommandLog->show();
    m_popOutCommandLog->raise();
}

void RemoteControllerWindow::ActionController_triggered()
{
    if (m_popOutController)
    {
        ui->GB_Controller->setHidden(false);
        m_popOutController->show();
        m_popOutController->raise();
    }
    else if (m_actionController->isChecked())
    {
        ui->GB_Controller->setHidden(false);
    }
    else
    {
        ui->GB_Controller->setHidden(true);
    }
}

void RemoteControllerWindow::ActionDiscord_triggered()
{
    m_discordSettings->show();
    m_discordSettings->raise();
}

void RemoteControllerWindow::DiscordScreenshot(snowflake_t id)
{
    if (m_vlcWrapper->getVideoManager()->isStarted())
    {
        QImage frame;
        m_vlcWrapper->getVideoManager()->getFrame(frame);
        m_discordSettings->sendImageMessage(id, frame);
    }
    else
    {
        m_discordSettings->sendMessage(id, "Camera not started");
    }
}

void RemoteControllerWindow::DiscordStatus(snowflake_t id)
{
    if (m_smartProgram)
    {
        m_smartProgram->sendDiscordMessage("Program Status", false);
    }
    else
    {
        m_discordSettings->sendMessage(id, "No Smart Program is running");
    }
}

void RemoteControllerWindow::DiscordCommand(snowflake_t id, const QString command)
{
    if (m_smartProgram)
    {
        m_discordSettings->sendMessage(id, "Cannot run command while Smart Program is running");
    }
    else
    {
        SendCommand(command);
        m_discordSettings->sendMessage(id, "Command sent. Note: no message return when command is finished");
    }
}

void RemoteControllerWindow::SetCaptureAreaPos(QMouseEvent *event)
{
    // Can't change while smart program is running
    if (m_smartProgram)
    {
        if (!SmartProgramBase::getProgramEnableUI(m_smartProgram->getProgramEnum()))
        {
            return;
        }
    }

    QWidget* widget = this->childAt(event->pos());
    VideoManager* videoManager = m_vlcWrapper->getVideoManager();
    if (widget == videoManager)
    {
        QPoint areaPoint = (event->pos() - videoManager->mapTo(this, QPoint(0,0))) * 2;
        if (areaPoint.x() > 1278 || areaPoint.y() > 718) return;

        QString name = ui->LW_SmartProgram->currentItem()->text();
        SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(name);
        if (sp == SP_COUNT)
        {
            return;
        }

        switch (sp)
        {
        case SP_BrightnessMeanFinder:
        {
            ui->SP1_SB_X->setValue(areaPoint.x());
            ui->SP1_SB_Y->setValue(areaPoint.y());
            break;
        }
        case SP_PurpleBeamFinder:
        {
            ui->SP2_SB_X->setValue(areaPoint.x());
            ui->SP2_SB_Y->setValue(areaPoint.y());
            break;
        }
        default: break;
        }
    }
}

//---------------------------------------------------------------------------
// Smart program functions
//---------------------------------------------------------------------------
void RemoteControllerWindow::ValidateSmartProgramCommands()
{
    if (m_smartProgram)
    {
        return;
    }

    // Validate all available smart commands xml
    int count = 0;
    QDirIterator it(SMART_COMMAND_PATH);
    while (it.hasNext())
    {
        QString dir = it.next();
        QString fileName = dir.mid(dir.lastIndexOf('/') + 1);
        if (fileName.endsWith(".xml"))
        {
            count++;
            QFile file(dir);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                PrintLog("Fail to load " + fileName, LOG_ERROR);
            }
            else
            {
                QDomDocument programCommands;
                programCommands.setContent(&file);
                file.close();

                QDomNodeList commandList = programCommands.firstChildElement().childNodes();
                for (int j = 0; j < commandList.count(); j++)
                {
                    QDomElement commandElement = commandList.at(j).toElement();
                    QString const commandName = commandElement.tagName();
                    QString const commandString = commandElement.text();

                    QString error;
                    if (!SmartProgramBase::validateCommand(commandString, error))
                    {
                        PrintLog(fileName + " &#60;" + commandName + "&#62; error: " + error, LOG_ERROR);
                    }
                }
            }
        }
    }

    PrintLog(QString::number(count) + " Smart Commands xml files validated", LOG_SUCCESS);
}

void RemoteControllerWindow::EnableSmartProgram()
{
    bool canRun = CanRunSmartProgram();
    //ui->GB_SmartProgram->setEnabled(canRun);
    ui->PB_StartSmartProgram->setEnabled(canRun);

    // if program is running...
    if (!canRun && m_smartProgram)
    {
        StopSmartProgram();
    }
}

bool RemoteControllerWindow::CanRunSmartProgram()
{
    return m_serialPort.isOpen()
        && m_serialState == SS_Connect
        && m_vlcWrapper->isPlaying();
}

void RemoteControllerWindow::RunSmartProgram(SmartProgram sp)
{
    if (!CanRunSmartProgram())
    {
        PrintLog("Cannot start smart program, this require serial port connected AND camera on", LOG_ERROR);
        return;
    }

    // Delete previous program if not already
    if (m_smartProgram)
    {
        delete m_smartProgram;
        m_smartProgram = Q_NULLPTR;
    }

    SmartProgramParameter parameter(m_vlcWrapper, m_discordSettings, m_smartSetting, ui->L_CurrentStats, this);
    bool enableUI = SmartProgramBase::getProgramEnableUI(sp);

    switch (sp)
    {
    case SP_DelayCalibrator:
    {
        m_smartProgram = new SmartDelayCalibrator(parameter);
        break;
    }
    case SP_BrightnessMeanFinder:
    {
        QVector<QSpinBox*> spinBoxes{ui->SP1_SB_X, ui->SP1_SB_Y, ui->SP1_SB_Width, ui->SP1_SB_Height,
                                    ui->SP1_SB_MinH, ui->SP1_SB_MinS, ui->SP1_SB_MinV,
                                    ui->SP1_SB_MaxH, ui->SP1_SB_MaxS, ui->SP1_SB_MaxV};
        parameter.preview = m_SP1_graphicScene;
        parameter.previewMasked = m_SP1_graphicSceneMasked;
        m_smartProgram = new SmartBrightnessMeanFinder
                (
                    spinBoxes,
                    ui->SP1_L_Mean,
                    ui->SP1_PB_MatchImage,
                    ui->SP1_L_MatchResult,
                    ui->SP1_CB_MatchScale,
                    ui->SP1_PB_OCR,
                    ui->SP1_LE_OCR,
                    ui->SP1_PB_OCRNumber,
                    ui->SP1_PB_LoadImage,
                    ui->SP1_CB_UseImage,
                    parameter
                );
        break;
    }
    case SP_PurpleBeamFinder:
    {
        QPoint pos(ui->SP2_SB_X->value(), ui->SP2_SB_Y->value());
        parameter.preview = m_SP2_graphicScene;
        parameter.previewMasked = m_SP2_graphicSceneMasked;
        m_smartProgram = new SmartPurpleBeamFilder(pos, parameter);
        break;
    }
    case SP_ColorCalibrator:
    {
        m_smartProgram = new SmartColorCalibrator(ui->SP8_L_Battle, ui->SP8_L_Pokemon, ui->SP8_L_Bag, ui->SP8_L_Run, parameter);
        break;
    }
    case SP_SoundDetection:
    {
        m_smartProgram = new SmartSoundDetection(ui->SP17_LE_SoundFile->text(), float(ui->SP17_SP_MinScore->value()), ui->SP17_SP_LPF->value(), parameter);
        break;
    }
    case SP_Test:
    {
        m_smartProgram = new SmartTestProgram(parameter);
        break;
    }
    case SP_CodeEntry:
    {
        SmartCodeEntry::Settings settings;
        settings.m_type = SmartCodeEntry::InputType(ui->SP19_CB_Type->currentIndex());
        settings.m_codeSize = ui->SP19_SP_Count->value();
        settings.m_lineEdit = ui->SP19_LE_Code;
        settings.m_clipboard = QGuiApplication::clipboard();
        settings.m_useOCR = ui->SP19_CB_OCR->isChecked();
        if (settings.m_useOCR)
        {
            settings.m_ocrImage = QGuiApplication::primaryScreen()->grabWindow(0).copy(ui->SP19_SB_X->value(), ui->SP19_SB_Y->value(), ui->SP19_SB_Width->value(), ui->SP19_SB_Height->value()).toImage();
        }
        m_smartProgram = new SmartCodeEntry(settings, parameter);
        break;
    }
    case SP_HomeSorter:
    {
        SmartHomeSorter::Settings settings;
        settings.m_count = ui->SP14_SB_Count->value();
        settings.m_livingDex = ui->SP14_CB_Shiny->isChecked();
        settings.m_livingDexShiny = ui->SP14_CB_Shiny->isChecked();
        m_smartProgram = new SmartHomeSorter(settings, parameter);
        break;
    }
    case SP_YCommGlitch:
    {
        m_smartProgram = new SmartYCommGlitch(parameter);
        break;
    }
    case SP_SurpriseTrade:
    {
        m_smartProgram = new SmartSurpriseTrade(ui->SP3_SB_Box->value(), ui->SP3_PB_Calibrate, parameter);
        break;
    }
    case SP_MaxRaidBattler:
    {
        m_smartProgram = new SmartMaxRaidBattler(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_DaySkipper:
    {
        m_smartProgram = new SmartDaySkipper(ui->SP5_SB_Skips->value(), ui->SP5_CB_Raid->isChecked(), ui->SP5_LE_PokemonList->text(), ui->SP5_TimeLeft, parameter);
        break;
    }
    case SP_BattleTower:
    case SP_S3_TableturfSkip:
    {
        m_smartProgram = new SmartSimpleProgram(sp, -1, parameter);
        break;
    }
    case SP_Loto:
    {
        m_smartProgram = new SmartLoto(ui->SP6_CB_Skips->isChecked() ? ui->SP6_SB_Skips->value() : 0, parameter);
        break;
    }
    case SP_DailyHighlight:
    {
        m_smartProgram = new SmartDailyHighlight(ui->SP6_CB_Skips->isChecked() ? ui->SP6_SB_Skips->value() : 0, parameter);
        break;
    }
    case SP_BerryFarmer:
    {
        m_smartProgram = new SmartBerryFarmer(ui->SP6_CB_Skips->isChecked() ? ui->SP6_SB_Skips->value() : 0, parameter);
        break;
    }
    case SP_WattFarmer:
    {
        m_smartProgram = new SmartWattFarmer(ui->SP6_CB_Skips->isChecked() ? ui->SP6_SB_Skips->value() : 0, parameter);
        break;
    }
    case SP_EggOperation:
    {
        SmartEggOperation::Settings settings;
        settings.m_operation = SmartEggOperation::EggOperationType(ui->SP20_CB_Mode->currentIndex());
        settings.m_targetEggCount = ui->SP20_SB_Collect->value();
        settings.m_columnsToHatch = ui->SP20_SB_Column->value();
        settings.m_isHatchExtra = ui->SP20_CB_HatchExtra->isChecked();
        settings.m_statTable = ui->SP20_TW_Keep;
        settings.m_parentGender = GenderType(ui->SP20_CB_ParentGender->currentIndex());
        if (ui->SP20_RB_ShinySound->isChecked())
        {
            settings.m_shinyDetection = SmartEggOperation::ShinyDetectionType::SDT_Sound;
        }
        else if (ui->SP20_RB_ShinyDelay->isChecked())
        {
            settings.m_shinyDetection = SmartEggOperation::ShinyDetectionType::SDT_Delay;
        }
        else
        {
            settings.m_shinyDetection = SmartEggOperation::ShinyDetectionType::SDT_Disable;
        }
        m_smartProgram = new SmartEggOperation(settings, parameter);
        break;
    }
    case SP_BoxRelease:
    {
        m_smartProgram = new SmartBoxRelease(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_TradePartnerFinder:
    {
        m_smartProgram = new SmartTradePartnerFinder(ui->SP18_LE_Name->text(), ui->SP18_LE_LinkCode->text(), ui->SP18_CB_Spam->isChecked(), parameter);
        break;
    }
    case SP_MaxLair:
    {
        SmartMaxLair::Settings settings;
        settings.m_legendIndex = ui->SP21_CB_Legend->currentIndex();
        settings.m_legendDownPress = ui->SP21_SB_Slot->value() - 1;
        settings.m_legendBall = (BallType)ui->SP21_CB_LegendBall->currentIndex();
        settings.m_bossBall = (BallType)ui->SP21_CB_BossBall->currentIndex();
        m_smartProgram = new SmartMaxLair(settings, parameter);
        break;
    }
    case SP_BDSP_Starter:
    {
        m_smartProgram = new SmartBDSPStarter(ui->SP7_CB_Starter->currentIndex(), parameter);
        break;
    }
    case SP_BDSP_MenuGlitch113:
    {
        m_smartProgram = new SmartSimpleProgram(SP_BDSP_MenuGlitch113, 1, parameter);
        break;
    }
    case SP_BDSP_BoxDuplication:
    {
        m_smartProgram = new SmartSimpleProgram(SP_BDSP_BoxDuplication, ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_BDSP_BoxOperation:
    {
        m_smartProgram = new SmartBDSPBoxOperation((SmartBDSPBoxOperation::BoxOperationType)ui->SP10_CB_Operation->currentIndex(), ui->SP10_SB_Box->value(), parameter);
        break;
    }
    case SP_BDSP_ShinyLegendary:
    {
        m_smartProgram = new SmartBDSPShinyLegendary((SmartBDSPShinyLegendary::LegendaryType)ui->SP11_CB_LegendaryType->currentIndex(), parameter);
        break;
    }
    case SP_BDSP_DuplicateItem1to30:
    {
        m_smartProgram = new SmartBDSPDuplicateItem1to30(parameter);
        break;
    }
    case SP_BDSP_EggOperation:
    {
        SmartBDSPEggOperation::Settings settings;
        settings.m_operation = SmartBDSPEggOperation::EggOperationType(ui->SP20_CB_Mode->currentIndex());
        settings.m_targetEggCount = ui->SP20_SB_Collect->value();
        settings.m_columnsToHatch = ui->SP20_SB_Column->value();
        settings.m_statTable = ui->SP20_TW_Keep;
        if (ui->SP20_RB_ShinySound->isChecked())
        {
            settings.m_shinyDetection = SmartBDSPEggOperation::ShinyDetectionType::SDT_Sound;
        }
        else
        {
            settings.m_shinyDetection = SmartBDSPEggOperation::ShinyDetectionType::SDT_Disable;
        }
        m_smartProgram = new SmartBDSPEggOperation(settings, parameter);
        break;
    }
    case SP_PLA_NuggetFarmer:
    {
        m_smartProgram = new SmartPLANuggetFarmer(ui->SP15_CB_Shiny->isChecked(), parameter);
        break;
    }
    case SP_PLA_ResetAlphaPokemon:
    {
        m_smartProgram = new SmartPLAResetAlphaPokemon((SmartPLAResetAlphaPokemon::AlphaType)ui->SP12_CB_AlphaType->currentIndex(), ui->SP12_CB_IgnoreNonAlpha->isChecked(), parameter);
        break;
    }
    case SP_PLA_BraviaryGainHeight:
    {
        m_smartProgram = new SmartSimpleProgram(SP_PLA_BraviaryGainHeight, 15, parameter);
        break;
    }
    case SP_PLA_DistortionWaiter:
    {
        m_smartProgram = new SmartPLADistortionWaiter(parameter);
        break;
    }
    case SP_PLA_OutbreakFinder:
    {
        m_smartProgram = new SmartPLAOutbreakFinder(ui->SP13_LE_PokemonList->text(), parameter);
        break;
    }
    case SP_PLA_PastureSorter:
    {
        SmartPLAPastureSorter::Settings settings;
        settings.m_pastureCount = ui->SP14_SB_Count->value();
        settings.m_livingDex = ui->SP14_CB_LivingDex->isChecked();
        settings.m_livingDexShiny = ui->SP14_CB_Shiny->isChecked();
        settings.m_livingDexAlpha = ui->SP14_CB_Alpha->isChecked();
        m_smartProgram = new SmartPLAPastureSorter(settings, parameter);
        break;
    }
    case SP_PLA_StaticSpawn:
    {
        m_smartProgram = new SmartPLAStaticSpawn(ui->SP16_CB_StaticPokemon->currentText(), ui->SP16_CB_IgnoreEarly->isChecked(), parameter);
        break;
    }
    case SP_PLA_MMORespawn:
    {
        m_smartProgram = new SmartPLAMMORespawn(parameter);
        break;
    }
    case SP_SV_ItemDuplication:
    {
        m_smartProgram = new SmartSimpleProgram(SP_SV_ItemDuplication, ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_SV_SurpriseTrade:
    {
        m_smartProgram = new SmartSVSurpriseTrade(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_SV_BoxRelease:
    {
        m_smartProgram = new SmartSVBoxRelease(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_SV_BoxReleaseSafe:
    {
        m_smartProgram = new SmartSVBoxReleaseSafe(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_SV_EggOperation:
    {
        SmartSVEggOperation::Settings settings;
        settings.m_operation = SmartSVEggOperation::EggOperationType(ui->SP9_CB_Mode->currentIndex());
        settings.m_sandwichCount = ui->SP9_SB_Sandwich->value();
        settings.m_sandwichPosX = ui->SP9_SB_X->value();
        settings.m_sandwichPosY = ui->SP9_SB_Y->value();
        settings.m_columnsToHatch = ui->SP9_SB_Column->value();
        settings.m_isHatchWithSandwich = ui->SP9_CB_HatchSandwich->isChecked();
        settings.m_isShinyDetection = ui->SP9_CB_Sound->isChecked();
        settings.m_isErrorCapture = ui->SP9_CB_Error->isChecked();
        settings.m_isSaveShiny = ui->SP9_CB_ShinySave->isChecked();
        settings.m_isUseBackupSave = ui->SP9_CB_BackupSave->isChecked();
        settings.m_isShinyWingull = ui->SP9_CB_Wingull->isChecked();
        m_smartProgram = new SmartSVEggOperation(settings, parameter);
        break;
    }
    case SP_SV_GimmighoulFarmer:
    {
        m_smartProgram = new SmartSVGimmighoulFarmer(parameter);
        break;
    }
    case SP_SV_TradePartnerFinder:
    {
        m_smartProgram = new SmartSVTradePartnerFinder(ui->SP18_LE_Name->text(), ui->SP18_CB_Spam->isChecked(), parameter);
        break;
    }
    case SP_SV_ItemPrinter:
    {
        SmartSVItemPrinter::Settings settings;
        settings.m_seed = static_cast<qint64>(ui->SP22_SB_Seed->value());
        settings.m_delay = ui->SP22_SB_Delay->value();
        settings.m_jobs = ui->SP22_CB_Jobs->currentIndex();
        settings.m_syncTime = ui->SP22_CB_Sync->isChecked();
        m_smartProgram = new SmartSVItemPrinter(settings, parameter);
        break;
    }
    case SP_TOTK_BowFuseDuplication:
    {
        m_smartProgram = new SmartTOTKBowFuseDuplication(ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_TOTK_MineruDuplication:
    case SP_TOTK_ShieldSurfDuplication:
    case SP_TOTK_ZonaiDeviceDuplication:
    {
        m_smartProgram = new SmartSimpleProgram(sp, ui->SPGeneric1_Count->value(), parameter);
        break;
    }
    case SP_S3_TableturfCloneJelly:
    {
        SmartS3TableturfCloneJelly::Settings settings;
        settings.m_mode = TableTurfAI::Mode::ThreeTwelveSp;
        m_smartProgram = new SmartS3TableturfCloneJelly(settings, parameter);
        break;
    }
    default:
    {
        QMessageBox::critical(this, "Error", "No code exist for this Smart Program!");
        return;
    }
    }

    connect(m_smartProgram, &SmartProgramBase::printLog, this, &RemoteControllerWindow::on_SmartProgram_printLog);
    connect(m_smartProgram, &SmartProgramBase::completed, this, &RemoteControllerWindow::on_SmartProgram_completed);
    connect(m_smartProgram, &SmartProgramBase::runSequence, this, &RemoteControllerWindow::on_SmartProgram_runSequence);
    connect(m_smartProgram, &SmartProgramBase::enableResetStats, this, &RemoteControllerWindow::EnableResetStats);
    connect(this, &RemoteControllerWindow::commandFinished, m_smartProgram, &SmartProgramBase::commandFinished);

    // Clear log
    on_PB_ClearLog_clicked();

    if (m_smartProgram->run())
    {
        ui->PB_StartSmartProgram->setText("Stop Smart Program");
        //ui->PB_SmartSettings->setEnabled(false);
        ui->CB_SmartProgram->setEnabled(false);
        ui->LW_SmartProgram->setEnabled(false);
        ui->PB_ModifySmartCommands->setEnabled(false);

        ui->SW_Settings->setEnabled(enableUI);
        SetEnableNonExceptionButtons(false);

        ui->LE_CommandSender->setEnabled(false);
        ui->PB_SendCommand->setEnabled(false);
    }
    else
    {
        StopSmartProgram();
    }
}

void RemoteControllerWindow::StopSmartProgram()
{
    // Stopped by user/program
    if (m_smartProgram)
    {
        PrintLog("[" + m_smartProgram->getProgramName() + "]: Stopped by user or program", LOG_WARNING);

        m_smartProgram->stop();
        delete m_smartProgram;
        m_smartProgram = Q_NULLPTR;

        // Stop any on-going command
        m_commandMutex->lock();
        m_infiniteLoop = false;
        m_executeCommands.clear();
        m_displayFlags.clear();
        m_commandMutex->unlock();

        ClearButtonFlags();
    }

    ui->PB_StartSmartProgram->setText("Start Smart Program");
    //ui->PB_SmartSettings->setEnabled(true);
    ui->CB_SmartProgram->setEnabled(true);
    ui->LW_SmartProgram->setEnabled(true);
    ui->PB_ModifySmartCommands->setEnabled(true);

    ui->SW_Settings->setEnabled(true);
    SetEnableNonExceptionButtons(true);

    ui->LE_CommandSender->setEnabled(true);
    ui->PB_SendCommand->setText("Send");
    ui->PB_SendCommand->setEnabled(!ui->LE_CommandSender->text().isEmpty());

    ui->CB_AudioDisplayMode->setEnabled(true);
    ui->SB_AudioFreqLow->setEnabled(true);
    ui->SB_AudioFreqHigh->setEnabled(true);
}

void RemoteControllerWindow::SetEnableNonExceptionButtons(bool enabled)
{
    // This is for when we allow enble UI when smart program is running but only allow some UI enabled within it
    ui->SP3_SB_Box->setEnabled(enabled);
    ui->SP19_CB_Type->setEnabled(enabled);
    ui->SP19_SP_Count->setEnabled(enabled);
    ui->SP19_SP_Select->setEnabled(enabled);
}
