#ifndef SMARTBDSPBOXOPERATION_H
#define SMARTBDSPBOXOPERATION_H

#include <QWidget>
#include "smartprogrambase.h"

class SmartBDSPBoxOperation : public SmartProgramBase
{
public:
    enum BoxOperationType : uint8_t
    {
        BOT_Release,
        BOT_BagItem,
        BOT_DupItem,
        BOT_COUNT,
    };

public:
    explicit SmartBDSPBoxOperation(
            BoxOperationType type,
            int boxCount,
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_BoxOperation; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void gotoNextPokemon();
    QString getLocation();

    // Command indices
    Command const C_Start        = 0;
    Command const C_StartItem    = 1;
    Command const C_Release      = 2;
    Command const C_BagItem      = 3;
    Command const C_DupItemStart = 4;
    Command const C_DupItemEnd   = 5;
    Command const C_COUNT        = 6;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Start,
        SS_NextPokemon,
        SS_Release,
        SS_BagItem,
        SS_DupItemStart,
        SS_DupItemEnd,
    };
    Substage m_substage;

    // Members
    BoxOperationType m_type;
    int m_boxCount;
    int m_box;
    QPoint m_pos;
};

#endif // SMARTBDSPBOXOPERATION_H
