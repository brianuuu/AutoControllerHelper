#ifndef DISCORDSETTINGS_H
#define DISCORDSETTINGS_H

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

private:
    QString getOwnerMention();
    Discord::Embed getEmbedTemplate(QString const& title);

private:
    Ui::DiscordSettings *ui;
    QSettings *m_settings;

    EmbedsClient m_client;
    bool m_isLoggedIn;
};

#endif // DISCORDSETTINGS_H
