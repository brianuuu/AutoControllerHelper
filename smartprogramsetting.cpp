#include "smartprogramsetting.h"
#include "ui_smartprogramsetting.h"

SmartProgramSetting::SmartProgramSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SmartProgramSetting)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    m_settings = new QSettings("brianuuu", "AutoControllerHelper", this);
    m_path = m_settings->value("DefaultDirectory", QString()).toString();

    // System
    ui->CB_DateArrangement->setCurrentIndex(m_settings->value("DateArrangement", 0).toInt());
    ui->CurrentDate->setDate(QDate::currentDate());

    // Sound
    m_defaultPlayer.setMedia(QUrl("qrc:/resources/Sounds/06-caught-a-pokemon.mp3"));
    ui->GB_Sound->setChecked(m_settings->value("SoundEnable", false).toBool());
    ui->LE_Sound->setText(m_settings->value("SoundFileDirectory", QString()).toString());
    bool customSound = m_settings->value("CustomSound", false).toBool();
    if (customSound)
    {
        ui->RB_CustomSound->setChecked(true);
    }
    else
    {
        ui->RB_DefaultSound->setChecked(true);
    }
    SetCustomSoundEnabled();

    // Stream Counter
    ui->GB_Stream->setChecked(m_settings->value("StreamCounterEnable", false).toBool());
    ui->LE_Prefix->setText(m_settings->value("TextFilePrefix", QString()).toString());
    ui->LE_File->setText(m_settings->value("TextFileDirectory", QString()).toString());
    ReadCounterFromText();

    // Others
    ui->CB_LogSave->setChecked(m_settings->value("LogAutosave", true).toBool());
    ui->CB_BypassFeedback->setChecked(m_settings->value("BypassFeedback", false).toBool());
}

SmartProgramSetting::~SmartProgramSetting()
{
    delete ui;
}

void SmartProgramSetting::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("DefaultDirectory", m_path);

    // System
    m_settings->setValue("DateArrangement", ui->CB_DateArrangement->currentIndex());

    // Sound
    m_settings->setValue("SoundEnable", ui->GB_Sound->isChecked());
    m_settings->setValue("SoundFileDirectory", ui->LE_Sound->text());
    m_settings->setValue("CustomSound", ui->RB_CustomSound->isChecked());
    if (m_customSound)
    {
        m_customSound->stop();
    }
    m_defaultPlayer.stop();

    // Stream Counter
    m_settings->setValue("StreamCounterEnable", ui->GB_Stream->isChecked());
    m_settings->setValue("TextFilePrefix", ui->LE_Prefix->text());
    m_settings->setValue("TextFileDirectory", ui->LE_File->text());

    // Others
    m_settings->setValue("LogAutosave", ui->CB_LogSave->isChecked());
    m_settings->setValue("BypassFeedback", ui->CB_BypassFeedback->isChecked());
}

DateArrangement SmartProgramSetting::getDateArrangement()
{
    return (DateArrangement)ui->CB_DateArrangement->currentIndex();
}

bool SmartProgramSetting::isSoundEnabled()
{
    return ui->GB_Sound->isChecked();
}

void SmartProgramSetting::playSound()
{
    if (!isSoundEnabled()) return;

    // Stop sound
    if (m_customSound)
    {
        m_customSound->stop();
    }
    m_defaultPlayer.stop();

    // Play sound
    if (ui->RB_CustomSound->isChecked())
    {
        QString const sound = ui->LE_Sound->text();
        if (!sound.isEmpty() && QFile::exists(sound))
        {
            if (m_customSound && m_customSound->fileName() != sound)
            {
                delete m_customSound;
            }
            if (!m_customSound)
            {
                m_customSound = new QSound(sound, this);
            }
            m_customSound->play();
        }
    }
    else
    {
        m_defaultPlayer.play();
    }
}

bool SmartProgramSetting::isStreamCounterEnabled()
{
    return ui->GB_Stream->isChecked();
}

int SmartProgramSetting::getStreamCounterCount()
{
    return ui->SB_Count->value();
}

void SmartProgramSetting::setStreamCounterCount(int count)
{
    if (isStreamCounterEnabled())
    {
        ui->SB_Count->setValue(count);
    }
}

bool SmartProgramSetting::isLogAutosave()
{
    return ui->CB_LogSave->isChecked();
}

bool SmartProgramSetting::isLogDebugCommand()
{
    return ui->CB_LogDebugCommands->isChecked();
}

bool SmartProgramSetting::isLogDebugColor()
{
    return ui->CB_LogDebugColor->isChecked();
}

bool SmartProgramSetting::isBypassFeedback()
{
    return ui->CB_BypassFeedback->isChecked();
}

void SmartProgramSetting::on_CB_DateArrangement_currentIndexChanged(int index)
{
    DateArrangement da = (DateArrangement)index;
    switch (da)
    {
    case DA_JP:
        ui->CurrentDate->setDisplayFormat("yyyy/MM/dd");
        break;
    case DA_EU:
        ui->CurrentDate->setDisplayFormat("dd/MM/yyyy");
        break;
    case DA_US:
        ui->CurrentDate->setDisplayFormat("MM/dd/yyyy");
        break;
    }
}

void SmartProgramSetting::on_PB_CurrentDate_clicked()
{
    ui->CurrentDate->setDate(QDate::currentDate());
}

void SmartProgramSetting::on_RB_DefaultSound_clicked()
{
    SetCustomSoundEnabled();
}

void SmartProgramSetting::on_RB_CustomSound_clicked()
{
    SetCustomSoundEnabled();
}

void SmartProgramSetting::on_PB_Sound_clicked()
{
    QString path = "";
    if (!m_path.isEmpty())
    {
        path = m_path;
    }

    QString file = QFileDialog::getOpenFileName(this, tr("Select Custom Sound"), path, "Sounds (*.mp3 *.wav)");
    if (file == Q_NULLPTR) return;

    // Save directory
    QFileInfo info(file);
    m_path = info.dir().absolutePath();
    ui->LE_Sound->setText(file);

    int index = file.lastIndexOf('\\');
    if (index == -1) index = file.lastIndexOf('/');

    SetCustomSoundEnabled();
}

void SmartProgramSetting::on_PB_PlaySound_clicked()
{
    playSound();
}

void SmartProgramSetting::on_PB_File_clicked()
{
    QString path = "";
    if (!m_path.isEmpty())
    {
        path = m_path;
    }

    QString file = QFileDialog::getOpenFileName(this, tr("Select Counter File"), path, "Text Documents (*.txt)");
    if (file == Q_NULLPTR) return;

    // Save directory
    QFileInfo info(file);
    m_path = info.dir().absolutePath();
    ui->LE_File->setText(file);

    int index = file.lastIndexOf('\\');
    if (index == -1) index = file.lastIndexOf('/');

    ReadCounterFromText();
}

void SmartProgramSetting::on_SB_Count_valueChanged(int arg1)
{
    QString file = ui->LE_File->text();
    if (file.isEmpty()) return;

    QFile textFile(file);
    if(!textFile.open(QIODevice::WriteOnly))
    {
        ui->LE_File->setText("");
        return;
    }

    QString number = QString::number(arg1);
    QTextStream textStream(&textFile);
    textStream << ui->LE_Prefix->text() << number;
    textFile.close();
}

void SmartProgramSetting::on_LE_Prefix_textEdited(const QString &arg1)
{
    on_SB_Count_valueChanged(ui->SB_Count->value());
}

void SmartProgramSetting::on_CB_BypassFeedback_clicked(bool checked)
{
    if (checked)
    {
        QMessageBox::warning(this, "Warning", "Enabling this will unable to detect serial disconnection while commands are running, only enable this if it is false disconnecting!", QMessageBox::Ok);
    }
}

void SmartProgramSetting::SetCustomSoundEnabled()
{
    if (ui->RB_CustomSound->isChecked())
    {
        // TODO: read media file

        ui->LE_Sound->setEnabled(true);
        ui->PB_Sound->setEnabled(true);
    }
    else
    {
        ui->LE_Sound->setEnabled(false);
        ui->PB_Sound->setEnabled(false);
    }
}

void SmartProgramSetting::ReadCounterFromText()
{
    QString file = ui->LE_File->text();
    if (file.isEmpty()) return;

    QFile textFile(file);
    if(!textFile.open(QIODevice::ReadOnly))
    {
        ui->LE_File->setText("");
        QMessageBox::critical(this, "Error", textFile.errorString());
        return;
    }

    QTextStream textStream(&textFile);
    QString line = textStream.readAll();
    textFile.close();

    QString noPrefix = line.mid(ui->LE_Prefix->text().size());
    if (!noPrefix.isEmpty())
    {
        bool success = false;
        int count = noPrefix.toInt(&success);
        if (success)
        {
            ui->SB_Count->setValue(count);
        }
    }
}
