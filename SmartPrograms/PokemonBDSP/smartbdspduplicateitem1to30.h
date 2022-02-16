#ifndef SMARTBDSPDUPLICATEITEM1TO30_H
#define SMARTBDSPDUPLICATEITEM1TO30_H

#include <QWidget>
#include "../smartprogrambase.h"

class SmartBDSPDuplicateItem1to30 : public SmartProgramBase
{
public:
    explicit SmartBDSPDuplicateItem1to30(
            SmartProgramParameter parameter
            );

    virtual SmartProgram getProgramEnum() { return SP_BDSP_DuplicateItem1to30; }

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();
    void gotoNextPokemon(QPoint& pos);

    // Command indices
    Command const C_Start        = 0;
    Command const C_GiveItem     = 1;
    Command const C_BagItem      = 2;
    Command const C_2ndMenuEnter = 3;
    Command const C_2ndMenuExit  = 4;
    Command const C_COUNT        = 5;

    // List of test color

    // List of test point/area

    // Substages
    enum Substage
    {
        SS_Init,
        SS_Start,

        SS_GiveItem,
        SS_1stNextPokemon,

        SS_2ndMenuEnter,
        SS_BagItem,
        SS_2ndNextPokemon,
        SS_2ndMenuExit,
    };
    Substage m_substage;

    // Members
    int m_bagItemCount;     // counter for bagging items on 2nd menu
    int m_giveItemCount;    // counter for giving items on 1st menu
    int m_itemCount;        // how many item we have, 1->2->4->8->16
    QPoint m_pos1st;        // pos on 1st box, will persist till (6,5)
    QPoint m_pos2nd;        // pos on 2nd box, reset to (1,1) when leaving
};

#endif // SMARTBDSPDUPLICATEITEM1TO30_H
