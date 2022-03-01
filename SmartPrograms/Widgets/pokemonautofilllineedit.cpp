#include "pokemonautofilllineedit.h"

PokemonAutofillLineEdit::PokemonAutofillLineEdit(QWidget *parent) : QLineEdit(parent)
{
    m_cursorStart = this->cursorPosition();
    m_cursorEnd = this->cursorPosition();
}

void PokemonAutofillLineEdit::InitCompleter(const QStringList &stringList)
{
    if (m_completer)
    {
        delete m_completer;
    }

    m_completer = new QCompleter(stringList, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated), this, &PokemonAutofillLineEdit::insertCompletion);
}

void PokemonAutofillLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (!m_completer)
    {
        QLineEdit::keyPressEvent(e);
        return;
    }

    bool completerVisible = m_completer->popup()->isVisible();
    if (completerVisible)
    {
        // The following keys are forwarded by the completer to the widget
       switch (e->key())
       {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
       default:
           break;
       }
    }
    else
    {
        m_cursorStart = this->cursorPosition();
    }

    // Process key press
    QLineEdit::keyPressEvent(e);
    PopUpCompleter();
}

void PokemonAutofillLineEdit::mousePressEvent(QMouseEvent *e)
{
    QLineEdit::mousePressEvent(e);
    PopUpCompleter();
}

void PokemonAutofillLineEdit::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this)
    {
        return;
    }

    QString text = this->text();
    text.remove(m_cursorStart, m_cursorEnd - m_cursorStart);
    text.insert(m_cursorStart, completion);
    this->setText(text);
    this->setCursorPosition(m_cursorStart + completion.length());
}

void PokemonAutofillLineEdit::PopUpCompleter()
{
    QString const text = this->text();
    QString const textLeft = text.left(this->cursorPosition());
    m_cursorStart = textLeft.lastIndexOf(',') + 1;
    m_cursorEnd = text.indexOf(',', this->cursorPosition());
    if (m_cursorEnd == -1)
    {
        m_cursorEnd = text.size();
    }

    QString completionPrefix;
    if (m_cursorEnd > m_cursorStart)
    {
        completionPrefix = text.mid(m_cursorStart, m_cursorEnd - m_cursorStart);
        //qDebug() << completionPrefix;
    }

    m_completer->setCompletionPrefix(completionPrefix);
    m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));

    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0) + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(cr); // popup it up!
}
