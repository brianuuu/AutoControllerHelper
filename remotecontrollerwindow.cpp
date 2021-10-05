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

    // Special case
    m_commandToFlagMap.insert("ASpam", m_PBToFlagMap[ui->PB_A] | m_turboFlag);
    m_commandToFlagMap.insert("BSpam", m_PBToFlagMap[ui->PB_B] | m_turboFlag);
    m_commandToFlagMap.insert("Loop", 0);

    QStringList validCommands;
    for (QMap<QString, ButtonFlag>::iterator iter = m_commandToFlagMap.begin(); iter != m_commandToFlagMap.end(); iter++)
    {
        validCommands << iter.key();
    }
    validCommands.sort();
    ui->LE_CommandSender->InitCompleter(validCommands);

    // Buttons
    on_PB_MapDefault_clicked();
    m_buttonFlag = 0;
    m_buttonMapChange = Q_NULLPTR;
    ui->L_MapWait->setVisible(false);
    qApp->installEventFilter(this);
    m_pauseKeyEventFilter = false;

    // Camera
    m_vlcWidget = new QLabel(this);
    m_vlcWidget->setMinimumSize(640,360);
    m_vlcWidget->setMaximumSize(640,360);
    m_vlcWidget->hide();
    m_vlcWrapper = new VLCWrapper(m_vlcWidget, ui->S_Volume, this);
    connect(m_vlcWrapper, &VLCWrapper::stateChanged, this, &RemoteControllerWindow::on_VLCState_changed);
    QVBoxLayout* layout = reinterpret_cast<QVBoxLayout*>(ui->GB_CameraView->layout());
    layout->insertWidget(0, m_vlcWidget);

    /*
    m_cameraView = new QCameraViewfinder(this);
    m_cameraView->setMinimumSize(640,360);
    m_cameraView->setMaximumSize(640,360);
    m_cameraView->hide();
    layout->insertWidget(0, m_cameraView);
    */

    if (!QDir(SCREENSHOT_PATH).exists())
    {
        QDir().mkdir(SCREENSHOT_PATH);
    }
    if (!QDir(LOG_PATH).exists())
    {
        QDir().mkdir(LOG_PATH);
    }

    // Smart program
    m_commandMutex = new QMutex(QMutex::RecursionMode::Recursive);
    for (int i = 0 ; i < SP_COUNT; i++)
    {
        SmartProgram sp = (SmartProgram)i;
        if (SmartProgramBase::getProgramTabID(sp) >= 0)
        {
            ui->LW_SmartProgram->addItem(SmartProgramBase::getProgramNameFromEnum(sp));
        }
    }

    // Load all timings from .xml file
    LoadSmartProgramCommands();

    m_SP1_graphicScene = new QGraphicsScene(this);
    m_SP1_graphicSceneMasked = new QGraphicsScene(this);
    ui->SP1_GV_Preview->setScene(m_SP1_graphicScene);
    ui->SP1_GV_PreviewMasked->setScene(m_SP1_graphicSceneMasked);

    m_SP2_graphicScene = new QGraphicsScene(this);
    m_SP2_graphicSceneMasked = new QGraphicsScene(this);
    ui->SP2_GV_Preview->setScene(m_SP2_graphicScene);
    ui->SP2_GV_PreviewMasked->setScene(m_SP2_graphicSceneMasked);
    ui->SP2_Frame_Hide->setVisible(false);

    // Call this after all smart program values are set up above
    ui->LW_SmartProgram->setCurrentRow(0);

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

    ui->S_Volume->setValue(m_settings->value("Volume", 100).toInt());

    m_smartSetting = new SmartProgramSetting();
}

RemoteControllerWindow::~RemoteControllerWindow()
{
    delete m_smartSetting;
    delete m_commandMutex;
    delete ui;
}

bool RemoteControllerWindow::eventFilter(QObject *object, QEvent *event)
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget)
    {
        QString className = QString(focusWidget->metaObject()->className());
        if (className == "QLineEdit" || className == "CommandSender" || className == "QSpinBox")
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

    m_settings->setValue("Volume", ui->S_Volume->value());

    if (m_smartSetting->isVisible())
    {
        m_smartSetting->close();
    }

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
}

void RemoteControllerWindow::UpdateStatus(QString status, QColor color)
{
    ui->L_Status->setText("Status: " + status);

    QString styleSheet = "font-size: 13px; color: rgb(";
    styleSheet += QString::number(color.red()) + ",";
    styleSheet += QString::number(color.green()) + ",";
    styleSheet += QString::number(color.blue()) + ");";
    ui->L_Status->setStyleSheet(styleSheet);
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
            m_serialState = SS_FeedbackOK;
        }
        return;
    }

    m_commandMutex->lock();
    if (!ba.isEmpty() && !m_executeCommands.isEmpty())
    {
        for (int i = 0; i < ba.size(); i++)
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
        UpdateStatus("Connecting...", QColor(0, 0, 0));
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
        PrintLog("Serial connected", LOG_SUCCESS);
        UpdateStatus("Connected", LOG_SUCCESS);
        ui->PB_Connect->setEnabled(true);
        ui->PB_Connect->setText("Disconnect");
        ui->PB_Refresh->setEnabled(false);
        ui->CB_SerialPorts->setEnabled(false);
        ui->LE_CommandSender->setEnabled(true);
        ui->PB_SendCommand->setText("Send");
        ui->PB_SendCommand->setEnabled(!ui->LE_CommandSender->text().isEmpty());

        EnableSmartProgram();
    }
    else
    {
        QString msg = "Failed to receive feedback from Arduino/Teensy, make sure you have done the following:";
        msg += "\n1. Installed RemoteControl.hex to the board";
        msg += "\n2. Have correct wire connection between board and CP210X chip";
        msg += "\n3. Board connect to Switch after disconnecting other controllers";
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

    UpdateStatus("Disconnected", LOG_ERROR);
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

void RemoteControllerWindow::on_PB_HideController_clicked()
{
    ui->GB_Controller->setHidden(!ui->GB_Controller->isHidden());
    ui->PB_HideController->setText(QString(ui->GB_Controller->isHidden() ? "Unhide" : "Hide") + " Virtual Controler");
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
}

bool RemoteControllerWindow::ValidateCommand(const QString &commands, QString &errorMsg)
{
    bool valid = true;
    int count = 0;

    QStringList list = commands.split(',');
    if (list.isEmpty() || list.size() % 2 == 1)
    {
        errorMsg = "Invalid syntax, it should be \"COMMAND,DURATION,COMMAND,DURATION,...\"";
        valid = false;
    }
    else
    {
        bool isLoop = false;
        for (int i = 0; i < list.size(); i++)
        {
            QString const& str = list[i];
            if (i % 2 == 0)
            {
                bool found = false;

                QString commandLower = str.toLower();
                for (QMap<QString, ButtonFlag>::iterator iter = m_commandToFlagMap.begin(); iter != m_commandToFlagMap.end(); iter++)
                {
                    isLoop = commandLower == "loop";
                    if (commandLower == iter.key().toLower())
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    errorMsg = "\"" + str + "\" is not a recognized command";
                    valid = false;
                    break;
                }
            }
            else
            {
                bool durationValid;
                int duration = str.toInt(&durationValid);
                if (!durationValid)
                {
                    errorMsg = "Duration is not a number";
                    valid = false;
                    break;
                }
                else if (duration > 65534)
                {
                    errorMsg = "Duration cannot be larger than 65534";
                    valid = false;
                    break;
                }
                else if (duration < 0)
                {
                    errorMsg = "Duration cannot be negative";
                    valid = false;
                    break;
                }
                else if (!isLoop && duration == 0)
                {
                    errorMsg = "Only 'Loop' command can use duration 0, other commands have no effect";
                    valid = false;
                    break;
                }

                count++;
            }
        }
    }

    if (count > COMMAND_MAX)
    {
        valid = false;
        errorMsg = "Number of commands exceed maximum of " + QString::number(COMMAND_MAX);
    }

    return valid;
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
    if (!ValidateCommand(commands, errorMsg))
    {
        PrintLog(errorMsg, LOG_ERROR);
        return false;
    }

    // We assume command is valid after here
    m_commandMutex->lock();
    m_infiniteLoop = false;
    m_displayFlags.clear();
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
            ui->L_NoVideo->setHidden(true);
            m_vlcWidget->show();
        }
        else
        {
            QMessageBox::critical(this, "Error", "An error occured when starting camera!", QMessageBox::Ok);
        }

        /*
        // Remove previous camera instance (if not already)
        if (m_camera)
        {
            delete m_camera;
            m_camera = Q_NULLPTR;
        }

        bool found = false;
        for (QCameraInfo const& info : QCameraInfo::availableCameras())
        {
            if (ui->CB_Cameras->currentText() == info.description())
            {
                m_camera = new QCamera(info, this);
                m_cameraCapture = new QCameraImageCapture(m_camera, this);
                m_camera->setCaptureMode(QCamera::CaptureStillImage);
                m_camera->setViewfinder(m_cameraView);
                m_camera->start();
                found = true;
                break;
            }
        }

        if (found)
        {
            // Handle error
            if (m_camera->error() != QCamera::NoError)
            {
                delete m_camera;
                m_camera = Q_NULLPTR;
                delete m_cameraCapture;
                m_cameraCapture = Q_NULLPTR;

                PrintLog("Failed to start camera", LOG_ERROR);
                QMessageBox::critical(this, "Error", "Failed to start camera!", QMessageBox::Ok);
            }
            else
            {
                PrintLog("Camera on", LOG_SUCCESS);
                ui->PB_StartCamera->setText("Stop Camera");

                ui->CB_Cameras->setEnabled(false);
                ui->PB_RefreshCamera->setEnabled(false);
                ui->PB_Screenshot->setEnabled(true);
                ui->L_NoVideo->setVisible(false);
                m_cameraView->show();
            }
        }
        else
        {
            on_PB_RefreshCamera_clicked();

            PrintLog("Camera not found, refreshing list...", LOG_ERROR);
            QMessageBox::critical(this, "Error", "Selected camera no longer exist, list is now refreshed.", QMessageBox::Ok);
        }
        */
    }
    else
    {
        if (m_vlcWrapper->isStarted())
        {
            m_vlcWrapper->stop();
            PrintLog("Camera off");
        }

        /*
        if (m_camera)
        {
            m_camera->stop();
            delete m_camera;
            m_camera = Q_NULLPTR;
            delete m_cameraCapture;
            m_cameraCapture = Q_NULLPTR;
            PrintLog("Camera off");
        }
        */

        ui->PB_StartCamera->setText("Start Camera");
        ui->PB_StartCamera->setEnabled(true);

        m_vlcWidget->hide();
        ui->RB_DetectedDevice->setEnabled(true);
        ui->RB_CustomDevice->setEnabled(true);
        ui->GB_DetectedDevice->setEnabled(true);
        ui->GB_CustomDevice->setEnabled(true);
        ui->PB_Screenshot->setEnabled(false);
        ui->L_NoVideo->setHidden(false);
    }

    EnableSmartProgram();
}

void RemoteControllerWindow::CameraCaptureToFile(QString name)
{
    QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + name + ".png";

    // Grab frame and save from QT draw
    QImage frame;
    m_vlcWrapper->getFrame(frame);
    frame.save(SCREENSHOT_PATH + nameWithTime);
    PrintLog("Screenshot saved: " + nameWithTime);

    /*
    // Use VLC internal to take screenshot
    if (m_vlcWrapper->takeSnapshot(SCREENSHOT_PATH + nameWithTime))
    {
        PrintLog("Screenshot saved: " + nameWithTime);
    }
    else
    {
        PrintLog("Screenshot failed, please try again later", LOG_ERROR);
    }
    */

    /*
    // Old QCamera implementation
    if (m_cameraCapture->isReadyForCapture())
    {
        QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + name + ".jpg";
        m_cameraCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
        m_cameraCapture->capture(SCREENSHOT_PATH + nameWithTime);
        PrintLog("Screenshot saved: " + nameWithTime);
    }
    else
    {
        PrintLog("Screenshot failed, please try again later", LOG_ERROR);
    }
    */
}

//---------------------------------------------------------------------------
// Smart program slots
//---------------------------------------------------------------------------
void RemoteControllerWindow::on_SP1_SB_X_valueChanged(int arg1)
{
    ui->SP1_SB_Width->setMaximum(1280 - arg1);
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Y_valueChanged(int arg1)
{
    ui->SP1_SB_Height->setMaximum(720 - arg1);
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Width_valueChanged(int arg1)
{
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP1_SB_Height_valueChanged(int arg1)
{
    QRect rect(ui->SP1_SB_X->value(),ui->SP1_SB_Y->value(),ui->SP1_SB_Width->value(),ui->SP1_SB_Height->value());
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP2_SB_X_valueChanged(int arg1)
{
    QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP2_SB_Y_valueChanged(int arg1)
{
    QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
    m_vlcWrapper->setDefaultArea(rect);
}

void RemoteControllerWindow::on_SP6_CB_Skips_clicked()
{
    ui->SP6_SB_Skips->setEnabled(ui->SP6_CB_Skips->isChecked());
}

void RemoteControllerWindow::on_SmartProgram_printLog(const QString log, QColor color)
{
    if (!CanRunSmartProgram()) return;

    PrintLog("[" + m_smartProgram->getProgramName() + "]: " + log, color);
}

void RemoteControllerWindow::on_SmartProgram_completed()
{
    if (!CanRunSmartProgram()) return;

    SmartProgram sp = m_smartProgram->getProgramEnum();
    switch (sp)
    {
    default: break;
    }

    // Save log
    SaveLog(m_smartProgram->getProgramInternalName());

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
    m_vlcWrapper->getFrame(frame);
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
        QMessageBox::critical(this, "Error", "Black border detected, please goto Settings->TV Settings->Adjust Screen Size and set to 100%.");
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

void RemoteControllerWindow::on_PB_ReloadSmartCommands_clicked()
{
    LoadSmartProgramCommands();
    EnableSmartProgram();
}

void RemoteControllerWindow::on_LW_SmartProgram_currentTextChanged(const QString &currentText)
{
    SmartProgram const sp = SmartProgramBase::getProgramEnumFromName(currentText);
    if (sp == SP_COUNT)
    {
        QMessageBox::critical(this, "Error", "Missing mapping for this Smart Program!");
        m_vlcWrapper->setDefaultAreaEnabled(false);
        return;
    }

    int tabIndex = SmartProgramBase::getProgramTabID(sp);
    if (tabIndex < 0 || tabIndex >= ui->SW_Settings->count())
    {
        QMessageBox::critical(this, "Error", "Invalid tab ID for this Smark Program!");
        m_vlcWrapper->setDefaultAreaEnabled(false);
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
        m_vlcWrapper->setDefaultArea(rect);
        break;
    }
    case SP_PurpleBeamFinder:
    {
        useArea = true;
        QRect rect(ui->SP2_SB_X->value(),ui->SP2_SB_Y->value(),140,80);
        m_vlcWrapper->setDefaultArea(rect);
        break;
    }
    default: break;
    }

    m_vlcWrapper->setDefaultAreaEnabled(useArea);
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
    if (widget == m_vlcWidget)
    {
        QPoint areaPoint = (event->pos() - m_vlcWidget->mapTo(this, QPoint(0,0))) * 2;
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
void RemoteControllerWindow::LoadSmartProgramCommands()
{
    if (m_smartProgram)
    {
        return;
    }

    m_smartProgramCommandsValid = true;
    QFile file
    (
#if DEBUG_ENABLED
        "../AutoControllerHelper/Database/SmartCommands.xml"
#else
        QString(BOT_PATH) + "RemoteControl/SmartCommands.xml"
#endif
    );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Fail to load SmartCommands.xml!", QMessageBox::Ok);
        m_smartProgramCommandsValid = false;
    }
    else
    {
        m_smartProgramCommands.clear();
        m_smartProgramCommands.setContent(&file);
        file.close();

        QDomNodeList programList = m_smartProgramCommands.firstChildElement().childNodes();
        for (int i = 0; i < programList.count(); i++)
        {
            QDomElement programElement = programList.at(i).toElement();
            QString const programName = programElement.tagName();

            QDomNodeList commandList = programElement.childNodes();
            for (int j = 0; j < commandList.count(); j++)
            {
                QDomElement commandElement = commandList.at(j).toElement();
                QString const commandName = commandElement.tagName();
                QString const commandString = commandElement.text();

                QString error;
                if (!ValidateCommand(commandString, error))
                {
                    PrintLog(programName + " " + commandName + " Error: " + error, LOG_ERROR);
                    m_smartProgramCommandsValid = false;
                }
            }
        }
    }

    if (!m_smartProgramCommandsValid)
    {
        PrintLog("Cannot enable Smart Programs due to invalid SmartCommands.xml!", LOG_ERROR);
    }
    else
    {
        PrintLog("SmartProgramCommands.xml validation completed!", LOG_SUCCESS);
    }
}

void RemoteControllerWindow::EnableSmartProgram()
{
    bool canRun = CanRunSmartProgram();
    ui->GB_SmartProgram->setEnabled(canRun);

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
        && m_vlcWrapper->isPlaying()
        && m_vlcWidget->isVisible()
        && m_smartProgramCommandsValid;
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

    SmartProgramParameter parameter(&m_smartProgramCommands, m_vlcWrapper, m_smartSetting, this);
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
        m_smartProgram = new SmartBrightnessMeanFinder(spinBoxes, ui->SP1_L_Mean, parameter);
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
        m_smartProgram = new SmartMaxRaidBattler(ui->SP4_SB_Count->value(), parameter);
        break;
    }
    case SP_DaySkipper:
    {
        m_smartProgram = new SmartDaySkipper(ui->SP5_SB_Skips->value(), ui->SP5_TimeLeft, parameter);
        break;
    }
    case SP_BattleTower:
    {
        m_smartProgram = new SmartBattleTower(parameter);
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
    default:
    {
        QMessageBox::critical(this, "Error", "No code exist for this Smart Program!");
        return;
    }
    }

    connect(m_smartProgram, &SmartProgramBase::printLog, this, &RemoteControllerWindow::on_SmartProgram_printLog);
    connect(m_smartProgram, &SmartProgramBase::completed, this, &RemoteControllerWindow::on_SmartProgram_completed);
    connect(m_smartProgram, &SmartProgramBase::runSequence, this, &RemoteControllerWindow::on_SmartProgram_runSequence);
    connect(this, &RemoteControllerWindow::commandFinished, m_smartProgram, &SmartProgramBase::commandFinished);

    // Clear log
    if (m_smartSetting->isLogAutosave())
    {
        ui->TB_Log->clear();
    }

    if (m_smartProgram->run())
    {
        ui->PB_StartSmartProgram->setText("Stop");
        ui->PB_SmartSettings->setEnabled(false);
        ui->LW_SmartProgram->setEnabled(false);
        ui->PB_ReloadSmartCommands->setEnabled(false);

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

    ui->PB_StartSmartProgram->setText("Start");
    ui->PB_SmartSettings->setEnabled(true);
    ui->LW_SmartProgram->setEnabled(true);
    ui->PB_ReloadSmartCommands->setEnabled(true);

    ui->SW_Settings->setEnabled(true);
    SetEnableNonExceptionButtons(true);

    ui->LE_CommandSender->setEnabled(true);
    ui->PB_SendCommand->setText("Send");
    ui->PB_SendCommand->setEnabled(!ui->LE_CommandSender->text().isEmpty());
}

void RemoteControllerWindow::SetEnableNonExceptionButtons(bool enabled)
{
    // This is for when we allow enble UI when smart program is running but only allow some UI enabled within it
    ui->SP3_SB_Box->setEnabled(enabled);
}
