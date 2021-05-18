#ifndef COMMANDSENDER_H
#define COMMANDSENDER_H

#include <QAbstractItemView>
#include <QCompleter>
#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollBar>

class CommandSender : public QLineEdit
{
    Q_OBJECT
public:
    explicit CommandSender(QWidget *parent = nullptr);

    void InitCompleter(QStringList const& stringList);
    void SaveCommand();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    void PopUpCompleter();

private:
    QCompleter* m_completer = Q_NULLPTR;
    int m_cursorStart;
    int m_cursorEnd;

    QStringList m_previousCommands;
    int m_previousCommandIndex;
};

#endif // COMMANDSENDER_H
