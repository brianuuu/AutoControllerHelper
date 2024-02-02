#include "discordsettings.h"
#include "ui_discordsettings.h"

DiscordSettings::DiscordSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DiscordSettings)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);

    m_isLoggedIn = false;
    QRegExpValidator reg(QRegExp("[0-9]*"));
    ui->LE_Channel->setValidator(&reg);
    ui->LE_Owner->setValidator(&reg);

    m_settings = new QSettings("brianuuu", "AutoControllerHelper", this);
    ui->LE_Token->setText(m_settings->value("DiscordToken", "").toString());
    ui->LE_Channel->setText(m_settings->value("DiscordChannel", "").toString());
    ui->LE_Owner->setText(m_settings->value("DiscordOwner", "").toString());
    ui->CB_Launch->setChecked(m_settings->value("DiscordStart", false).toBool());
    if (ui->CB_Launch->isChecked())
    {
        on_PB_Connect_clicked();
    }
}

DiscordSettings::~DiscordSettings()
{
    delete ui;
}

void DiscordSettings::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("DiscordToken", ui->LE_Token->text());
    m_settings->setValue("DiscordChannel", ui->LE_Channel->text());
    m_settings->setValue("DiscordOwner", ui->LE_Owner->text());
    m_settings->setValue("DiscordStart", ui->CB_Launch->isChecked());
}

void DiscordSettings::on_LE_Token_textChanged(const QString &arg1)
{
    ui->GB_Settings->setEnabled(!arg1.isEmpty());
    ui->PB_Connect->setEnabled(!arg1.isEmpty());
}

void DiscordSettings::on_LE_Owner_textChanged(const QString &arg1)
{
    ui->PB_Owner->setEnabled(!arg1.isEmpty());
}

void DiscordSettings::on_LE_Channel_textChanged(const QString &arg1)
{
    ui->PB_Channel->setEnabled(!arg1.isEmpty());
}

void DiscordSettings::on_PB_Connect_clicked()
{
    if (ui->LE_Token->text().isEmpty()) return;

    if (m_isLoggedIn)
    {
        m_client.logout();

        m_isLoggedIn = false;
        ui->PB_Connect->setText("Start Bot");
    }
    else
    {
        Discord::Token token;
        token.generate(ui->LE_Token->text(), Discord::Token::Type::BOT);
        m_client.login(token);

        m_isLoggedIn = true;
        ui->PB_Connect->setText("Stop Bot");
    }

    ui->LE_Token->setEnabled(!m_isLoggedIn);
    ui->GB_Settings->setEnabled(!m_isLoggedIn);
}

void DiscordSettings::on_PB_Channel_clicked()
{
    if (!m_isLoggedIn || ui->LE_Channel->text().isEmpty()) return;

    snowflake_t id = ui->LE_Channel->text().toULongLong();
    m_client.createMessage(id, getEmbedTemplate("Test Channel Notification"), getOwnerMention());
}

void DiscordSettings::on_PB_Owner_clicked()
{
    if (!m_isLoggedIn || ui->LE_Owner->text().isEmpty()) return;

    snowflake_t id = ui->LE_Owner->text().toULongLong();
    m_client.createDm(id).then(
        [this](Discord::Channel const& c)
        {
            m_client.createMessage(c.id(), getEmbedTemplate("Test Owner DM"), getOwnerMention());
        }
    );
}

QString DiscordSettings::getOwnerMention()
{
    if (ui->LE_Owner->text().isEmpty()) return "";
    return "<@" + ui->LE_Owner->text() + ">\n";
}

Discord::Embed DiscordSettings::getEmbedTemplate(const QString &title)
{
    Discord::Embed embed;
    embed.setTitle(title);
    embed.setDescription("By Auto Controller v" + QString(VERSION) + " ([GitHub](https://github.com/brianuuu/AutoController_swsh)/[Discord](https://discord.gg/GWEurpGZNM))");
    return embed;
}
