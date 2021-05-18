#include "commandsender.h"

CommandSender::CommandSender(QWidget *parent) : QLineEdit(parent)
{
    m_cursorStart = this->cursorPosition();
    m_cursorEnd = this->cursorPosition();
    m_previousCommandIndex = -1;
}

void CommandSender::InitCompleter(const QStringList &stringList)
{
    if (m_completer)
    {
        delete m_completer;
    }

    m_completer = new QCompleter(stringList, this);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated), this, &CommandSender::insertCompletion);
}

void CommandSender::SaveCommand()
{
    // Remember last used commands, up to 10
    m_previousCommandIndex = -1;
    m_previousCommands.push_front(this->text());
    if (m_previousCommands.size() > 10)
    {
        m_previousCommands.pop_back();
    }
}

void CommandSender::keyPressEvent(QKeyEvent *e)
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

    // Show previous commands
    if (this->cursorPosition() == 0 && (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down))
    {
        int index = m_previousCommandIndex;
        if (e->key() == Qt::Key_Up)
        {
            m_previousCommandIndex++;
        }
        else if (e->key() == Qt::Key_Down)
        {
            m_previousCommandIndex--;
        }
        m_previousCommandIndex = qBound(-1, m_previousCommandIndex, m_previousCommands.size() - 1);

        if (index == 0 && m_previousCommandIndex == -1)
        {
            this->clear();
        }
        else if (m_previousCommandIndex != -1)
        {
            this->setText(m_previousCommands[m_previousCommandIndex]);
            this->setCursorPosition(0);
        }
    }

    if (this->text().isEmpty())
    {
        m_previousCommandIndex = -1;
    }

    PopUpCompleter();
}

void CommandSender::mousePressEvent(QMouseEvent *e)
{
    QLineEdit::mousePressEvent(e);
    PopUpCompleter();
}

void CommandSender::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != this)
    {
        return;
    }

    QString text = this->text();
    text.remove(m_cursorStart, m_cursorEnd - m_cursorStart);
    bool addComma = (m_cursorStart == text.size());
    text.insert(m_cursorStart, completion + (addComma ? "," : ""));
    this->setText(text);
    this->setCursorPosition(m_cursorStart + completion.length() + (addComma ? 1 : 0));
}

void CommandSender::PopUpCompleter()
{
    // Force popup if edit is empty and every 2nd comma
    QString const text = this->text();
    QString const textLeft = text.left(this->cursorPosition());
    if (textLeft.count(',') % 2 == 0 && this->cursorPosition() != 0)
    {
        m_cursorStart = textLeft.lastIndexOf(',') + 1;
        m_cursorEnd = text.indexOf(',', this->cursorPosition());
        if (m_cursorEnd == -1)
        {
            m_cursorEnd = text.size();
        }
    }
    else
    {
        m_completer->popup()->hide();
        return;
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
