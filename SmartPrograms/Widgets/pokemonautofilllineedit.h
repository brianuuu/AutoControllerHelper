#ifndef PLAOUTBREAKLINEEDIT_H
#define PLAOUTBREAKLINEEDIT_H

#include <QAbstractItemView>
#include <QCompleter>
#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollBar>

class PokemonAutofillLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit PokemonAutofillLineEdit(QWidget *parent = nullptr);

    void InitCompleter(QStringList const& stringList);

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
};

#endif // PLAOUTBREAKLINEEDIT_H
