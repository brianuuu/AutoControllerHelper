#ifndef DISCORDSETTINGS_H
#define DISCORDSETTINGS_H

#include <QBuffer>
#include <QLayout>
#include <QRegExpValidator>
#include <QSettings>
#include <QWidget>

#include "QDiscord/Helpers/EmbedsClient.h"
#include "QDiscord/Discord/Client.h"

#include "autocontrollerdefines.h"

namespace Ui {
class DiscordSettings;
}

class DiscordSettings : public QWidget
{
    Q_OBJECT

public:
    explicit DiscordSettings(QWidget *parent = nullptr);
    ~DiscordSettings();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_LE_Owner_textChanged(const QString &arg1);
    void on_LE_Token_textChanged(const QString &arg1);
    void on_LE_Channel_textChanged(const QString &arg1);

    void on_PB_Connect_clicked();
    void on_PB_Channel_clicked();
    void on_PB_Owner_clicked();

    void handleMessage(const Discord::Message& message);

signals:
    void signalScreenshot(snowflake_t id);
    void signalStatus(snowflake_t id);
    void signalCommand(snowflake_t id, QString const command);

public:
    // getters
    QString getOwnerMention();
    Discord::Embed getEmbedTemplate(QString const& title);

    // senders
    bool canSendMessage();
    void sendMessage(Discord::Embed const& embed, bool isMention, QImage const* img = nullptr);
    void sendError(const Discord::Embed &embed, const QImage *img = nullptr);

    void sendMessage(snowflake_t channelId, QString const& content);
    void sendImageMessage(snowflake_t channelId, QImage const& img);

private:
    Ui::DiscordSettings *ui;
    QSettings *m_settings;

    EmbedsClient m_client;
    bool m_isLoggedIn;
};

#endif // DISCORDSETTINGS_H
