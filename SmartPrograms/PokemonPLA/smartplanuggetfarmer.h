#ifndef SMARTPLANUGGETFARMER_H
#define SMARTPLANUGGETFARMER_H

#include <QElapsedTimer>
#include <QWidget>
#include "../smartprogrambase.h"

class SmartPLANuggetFarmer : public SmartProgramBase
{
public:
    explicit SmartPLANuggetFarmer(bool isFindShiny, SmartProgramParameter parameter);

    virtual SmartProgram getProgramEnum() { return SP_PLA_NuggetFarmer; }

private slots:
    void soundDetected(int id);

private:
    virtual void init();
    virtual void reset();
    virtual void runNextState();

    // Command indices
    Command const C_Restart                 = 0;
    Command const C_FlyToHeightCamp         = 1;
    Command const C_WaitTillMorning         = 2;
    Command const C_Save                    = 3;
    Command const C_GetOnWyrdeer            = 4;
    Command const C_GetOnWyrdeerNoMove      = 5;
    Command const C_FindCoin                = 6;
    Command const C_FindCharm               = 7;
    Command const C_AfterBattle             = 8;
    Command const C_AfterBattleCharm        = 9;
    Command const C_TalkeToLaventon         = 10;
    Command const C_TalkeToLaventonFinish   = 11;
    Command const C_COUNT                   = 12;

    // List of test color
    HSVRange const C_Color_Royal = HSVRange(30,0,220,90,255,255);
    HSVRange const C_Color_RoyalWhite = HSVRange(30,0,220,90,150,255);
    HSVRange const C_Color_Dialog = HSVRange(180,50,50,240,190,110);
    HSVRange const C_Color_AConfirmReturn = HSVRange(0,0,200,259,80,255);
    HSVRange const C_Color_Loading = HSVRange(0,0,0,359,255,40);
    HSVRange const C_Color_Map = HSVRange(0,0,110,359,255,150);

    // List of test point/area
    CaptureArea const A_Title = CaptureArea(400,420,520,100);
    CaptureArea const A_Royal = CaptureArea(1175,479,65,68);
    CaptureArea const A_Dialog = CaptureArea(328,511,200,44);
    CaptureArea const A_BattleEnd = CaptureArea(1000,260,200,200);
    CaptureArea const A_AConfirmReturn = CaptureArea(983,416,64,28);
    CaptureArea const A_AConfirmReport = CaptureArea(983,447,64,28);
    CaptureArea const A_PokedexProgress = CaptureArea(600,490,120,30);
    CaptureArea const A_Loading = CaptureArea(560,654,200,42);
    CaptureArea const A_Map = CaptureArea(490,692,200,24);

    // Substages
    enum Substage
    {
        SS_Init,

        SS_Restart,
        SS_Title,
        SS_GameStart,

        SS_FlyToHeightCamp,
        SS_WaitTillMorning,
        SS_Save,

        SS_SelectWyrdeer,
        SS_GetOnWyrdeer,

        SS_FindSister,
        SS_StartBattle,
        SS_DuringBattle,
        SS_AfterBattle,

        SS_AlphaKnockOff,

        SS_TalkToLaventon,
        SS_LoadingToVillageStart,
        SS_LoadingToObsidianStart,
        SS_LoadingToVillageEnd,
        SS_LoadingToObsidianEnd,
        SS_DetectMap,

        SS_Capture,
    };
    Substage m_substage;

    // Members
    QElapsedTimer m_elapsedTimer;
    Substage m_substageAfterCamp;
    //QImage m_imageMatch_AConfirm;
    QImage m_imageMatch_RoyalWyrdeer;
    int m_searchRoyalCount;
    int m_searchSisterCount;
    bool m_isFirstTimeVillageReturn;
    bool m_isFirstTimeSave;
    bool m_isCoin;

    bool m_isFindShiny;
    int m_shinySoundID;
    bool m_shinyDetected;

    // Stats
    Stat m_statSearches;
    Stat m_statCharmFound;
    Stat m_statCoinFound;
    Stat m_statError;
    Stat m_statShiny;
};

#endif // SMARTPLANUGGETFARMER_H
