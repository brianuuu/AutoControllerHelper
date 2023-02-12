#include "smartcodeentry.h"

SmartCodeEntry::SmartCodeEntry
(
    Settings setting,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(setting)
{
    init();
}

bool SmartCodeEntry::getKeyboardLocation(QChar key, QPoint &o_point)
{
    static const QMap<QChar, QPoint> c_keyboardMapping =
    {
        {'1', {0,0}},
        {'2', {1,0}},
        {'3', {2,0}},
        {'4', {3,0}},
        {'5', {4,0}},
        {'6', {5,0}},
        {'7', {6,0}},
        {'8', {7,0}},
        {'9', {8,0}},
        {'0', {9,0}},

        {'q', {0,1}},
        {'w', {1,1}},
        {'e', {2,1}},
        {'r', {3,1}},
        {'t', {4,1}},
        {'y', {5,1}},
        {'u', {6,1}},
        //{'i', {7,1}},
        //{'o', {8,1}},
        {'p', {9,1}},

        {'a', {0,2}},
        {'s', {1,2}},
        {'d', {2,2}},
        {'f', {3,2}},
        {'g', {4,2}},
        {'h', {5,2}},
        {'j', {6,2}},
        {'k', {7,2}},
        {'l', {8,2}},

        //{'z', {0,3}},
        {'x', {1,3}},
        {'c', {2,3}},
        {'v', {3,3}},
        {'b', {4,3}},
        {'n', {5,3}},
        {'m', {6,3}},
    };

    if (c_keyboardMapping.contains(key))
    {
        o_point = c_keyboardMapping[key];
        return true;
    }

    return false;
}

bool SmartCodeEntry::getNumPadLocation(QChar key, QPoint &o_point)
{
    static const QMap<QChar, QPoint> c_numPadMapping =
    {
        {'1', {0,0}},
        {'2', {1,0}},
        {'3', {2,0}},
        {'4', {0,1}},
        {'5', {1,1}},
        {'6', {2,1}},
        {'7', {0,2}},
        {'8', {1,2}},
        {'9', {2,2}},
        {'0', {1,3}},
    };

    if (c_numPadMapping.contains(key))
    {
        o_point = c_numPadMapping[key];
        return true;
    }

    return false;
}

void SmartCodeEntry::cursorPositionChanged(int oldPos, int newPos)
{
    m_programSettings.m_lineEdit->setCursorPosition(m_programSettings.m_lineEdit->text().size());
}

void SmartCodeEntry::selectionChanged()
{
    m_programSettings.m_lineEdit->setCursorPosition(m_programSettings.m_lineEdit->text().size());
}

void SmartCodeEntry::textEdited(const QString &text)
{
    if (!text.isEmpty())
    {
        if (m_programSettings.m_type == IT_Keyboard)
        {
            QPoint pos;
            if (!getKeyboardLocation(text.back().toLower(), pos))
            {
                emit printLog(QString("\"") + text.back() + "\" is not a valid character!", LOG_WARNING);
                m_programSettings.m_lineEdit->setText(text.mid(0, text.size() - 1));
            }
        }
        else
        {
            if (!text.back().isNumber())
            {
                emit printLog(QString("\"") + text.back() + "\" is not a valid number!", LOG_WARNING);
                m_programSettings.m_lineEdit->setText(text.mid(0, text.size() - 1));
            }
        }
    }
}

void SmartCodeEntry::dataChanged()
{
    QString code = m_programSettings.m_lineEdit->text() + m_programSettings.m_clipboard->text();
    if (verifyCode(code))
    {
        m_programSettings.m_lineEdit->setText(code);
    }
    else
    {
        m_programSettings.m_lineEdit->setText("");
    }
}

void SmartCodeEntry::init()
{
    SmartProgramBase::init();

    if (m_programSettings.m_type >= IT_Count || m_programSettings.m_lineEdit == Q_NULLPTR)
    {
        setState_error("Invalid argument");
    }

    connect(m_programSettings.m_lineEdit, &QLineEdit::cursorPositionChanged, this, &SmartCodeEntry::cursorPositionChanged);
    connect(m_programSettings.m_lineEdit, &QLineEdit::selectionChanged, this, &SmartCodeEntry::selectionChanged);
    connect(m_programSettings.m_lineEdit, &QLineEdit::textEdited, this, &SmartCodeEntry::textEdited);

    if (m_programSettings.m_clipboard && !m_programSettings.m_useOCR)
    {
        connect(m_programSettings.m_clipboard, &QClipboard::dataChanged, this, &SmartCodeEntry::dataChanged);
    }

    m_pos = QPoint(0,0);
    m_code = "";
}

void SmartCodeEntry::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartCodeEntry::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        if (m_programSettings.m_useOCR)
        {
            m_substage = SS_Input;
            setState_ocrRequest(m_programSettings.m_ocrImage);
        }
        else
        {
            // verify initial code
            QString code = m_programSettings.m_lineEdit->text();
            if (!verifyCode(code))
            {
                m_programSettings.m_lineEdit->setText("");
            }

            // grab first character
            code = m_programSettings.m_lineEdit->text();
            if (!code.isEmpty())
            {
                m_code = code.front();
                emit printLog(QString("Adding character \"") + m_code.back() + "\"");
                runCommandToKey();
            }
            else
            {
                m_substage = SS_Input;
                setState_runCommand("Nothing,1");
            }
        }
        break;
    }
    case SS_Input:
    {
        if (state == S_OCRReady)
        {
            m_ocrCode = getOCRStringRaw();
            emit printLog("OCR returned string: " + m_ocrCode);

            // Fix some impossible characters
            for (QChar& c : m_ocrCode)
            {
                if (c.toLower() == 'i')
                {
                    emit printLog("I is probably a 1", LOG_WARNING);
                    c = '1';
                }
                else if (c.toLower() == 'o')
                {
                    emit printLog("O is probably a 0", LOG_WARNING);
                    c = '0';
                }
                else if (c == ')')
                {
                    emit printLog(") is probably a J", LOG_WARNING);
                    c = 'J';
                }
            }

            // verify if code is valid
            if (m_ocrCode.size() != m_programSettings.m_codeSize)
            {
                setState_error("Expected " + QString::number(m_programSettings.m_codeSize) + "-letter code, found " + QString::number(m_ocrCode.size()));
                break;
            }

            if (!verifyCode(m_ocrCode))
            {
                setState_error("Invalid code error");
                break;
            }

            // start input!
            m_code = m_ocrCode.front();
            emit printLog(QString("Adding character \"") + m_code.back() + "\"");
            runCommandToKey();
        }
        else if (state == S_CommandFinished)
        {
            QString const code = m_programSettings.m_useOCR ? m_ocrCode : m_programSettings.m_lineEdit->text();
            if (m_code.size() < code.size())
            {
                // append next character
                m_code.append(code[m_code.size()]);
                emit printLog(QString("Adding character \"") + m_code.back() + "\"");
                runCommandToKey();

            }
            else if (m_code.size() > code.size())
            {
                // remove character
                emit printLog(QString("Remove character \"") + m_code.back() + "\"");
                m_code.chop(1);
                setState_runCommand("B,1");
            }
            else
            {
                // wait for more/fewer characters
                runNextStateContinue();
            }
        }
        break;
    }
    case SS_Finish:
    {
        if (state == S_CommandFinished)
        {
            m_programSettings.m_lineEdit->setText("");
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

bool SmartCodeEntry::verifyCode(const QString &code)
{
    if (!code.isEmpty())
    {
        if (m_programSettings.m_type == IT_Keyboard)
        {
            for (QChar const& c : code)
            {
                QPoint pos;
                if (!getKeyboardLocation(c.toLower(), pos))
                {
                    emit printLog("Code contains invalid characters, please try again", LOG_WARNING);
                    return false;
                }
            }
        }
        else
        {
            for (QChar const& c : code)
            {
                if (!c.isNumber())
                {
                    emit printLog("Code contains non-number, please try again", LOG_WARNING);
                    return false;
                }
            }
        }
    }

    return true;
}

void SmartCodeEntry::runCommandToKey()
{
    m_substage = m_code.size() == m_programSettings.m_codeSize ? SS_Finish : SS_Input;
    QPoint targetPos;
    QString command = "";

    bool valid = false;
    bool isNumZero = false;
    if (m_programSettings.m_type == IT_Keyboard)
    {
        valid = getKeyboardLocation(m_code.back().toLower(), targetPos);
    }
    else
    {
        valid = getNumPadLocation(m_code.back(), targetPos);
        isNumZero = m_code.back() == '0';
    }

    if (!valid)
    {
        setState_error("Invalid character");
    }
    else if (m_pos != targetPos)
    {
        int vSteps = targetPos.y() - m_pos.y();
        if (vSteps != 0)
        {
            // can only go up to 3 steps
            switch (vSteps)
            {
            case -1:
                command += "LUp,1";
                break;
            case -2:
                command += "LUp,1,DUp,1";
                break;
            case -3:
                command += "LUp,1,DUp,1,LUp,1";
                break;
            case 1:
                command += "LDown,1";
                break;
            case 2:
                command += "LDown,1,DDown,1";
                break;
            case 3:
                command += "LDown,1,DDown,1,LDown,1";
                break;
            }

            command += ",Loop,1";
        }

        int hSteps = targetPos.x() - m_pos.x();
        if (hSteps != 0 && !isNumZero)
        {
            if (hSteps > 6)
            {
                hSteps -= 12;
            }
            else if (hSteps < -6)
            {
                hSteps += 12;
            }

            bool goRight = hSteps > 0;
            hSteps = qAbs(hSteps);

            if (!command.isEmpty())
            {
                command += ",";
            }

            command += goRight ? "LRight,1" : "LLeft,1";
            if (hSteps > 1)
            {
                command += goRight ? ",DRight,1" : ",DLeft,1";
                command += ",Loop," + QString::number(hSteps / 2);
                if (hSteps % 2 == 1)
                {
                    // extra single command for odd number
                    command += goRight ? ",LRight,1" : ",LLeft,1";
                }
            }
        }
    }

    // finally press A, and plus if finished
    if (!command.isEmpty())
    {
        command += ",";
    }
    command += "A,1";
    if (m_substage == SS_Finish)
    {
        command += ",Plus,1";
    }
    m_pos = targetPos;
    setState_runCommand(command);
}
