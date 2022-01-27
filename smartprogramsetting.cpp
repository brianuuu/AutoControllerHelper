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
    ui->CB_PreventUpdate->setChecked(m_settings->value("PreventUpdate", false).toBool());
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

    // Others
    ui->CB_LogSave->setChecked(m_settings->value("LogAutosave", true).toBool());
}

SmartProgramSetting::~SmartProgramSetting()
{
    delete ui;
}

void SmartProgramSetting::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("DefaultDirectory", m_path);

    // System
    m_settings->setValue("PreventUpdate", ui->CB_PreventUpdate->isChecked());
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

    // Others
    m_settings->setValue("LogAutosave", ui->CB_LogSave->isChecked());
}

bool SmartProgramSetting::isPreventUpdate()
{
    return ui->CB_PreventUpdate->isChecked();
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
