#ifndef SMARTCODEENTRY_H
#define SMARTCODEENTRY_H

#include <QClipboard>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartCodeEntry : public SmartProgramBase
{
public:
    enum InputType : uint8_t
    {
        IT_Keyboard = 0,
        IT_NumPad,
        IT_Count
    };

    struct Settings
    {
        InputType m_type;
        int m_codeSize;
        QLineEdit* m_lineEdit;
        QClipboard* m_clipboard;

        bool m_useOCR;
        QImage m_ocrImage;
    };

public:
    explicit SmartCodeEntry(
            Settings setting,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_CodeEntry; }

    static bool getKeyboardLocation(QChar key, QPoint& o_point);
    static bool getNumPadLocation(QChar key, QPoint& o_point);

private slots:
    void cursorPositionChanged(int oldPos, int newPos);
    void selectionChanged();
    void textEdited(const QString &text);
    void dataChanged();

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    bool verifyCode(QString const& code);
    void runCommandToKey();

    // Command indices

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Input,
        SS_Finish,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_timer;
    Settings m_programSettings;
    QPoint m_pos;
    QString m_code;
    QString m_ocrCode;
};

#endif // SMARTCODEENTRY_H
