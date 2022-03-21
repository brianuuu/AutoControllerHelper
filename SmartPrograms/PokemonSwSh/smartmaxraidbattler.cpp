#include "smartmaxraidbattler.h"

SmartMaxRaidBattler::SmartMaxRaidBattler
(
    int maxBattle,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_maxBattle(maxBattle)
{
    init();
}

void SmartMaxRaidBattler::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);

    for (int i = 0; i < 4; i++)
    {
        A_RaidMenu.push_back(CaptureArea(1166, 440+i*57, 32, 32));
    }
}

void SmartMaxRaidBattler::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_curBattle = 0;
}

void SmartMaxRaidBattler::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_ASpam;
        setState_runCommand(C_ASpam);

        m_videoManager->setAreas(A_RaidMenu);
        break;
    }
    case SS_ASpam:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            // Check where we are at raid start up screen
            // Should be expecting we have pressed Invite Others due to camera delay
            // Catch screen will have screen completely white too, so we check other parts of the screen
            /*
            bool atRaidScreen = checkBrightnessMean(A_RaidWhite.m_rect, C_Color_Raid) > 250    // top right = white
                            && (checkBrightnessMean(A_RaidRare.m_rect, C_Color_Common) > 250    // common beam ~= red
                             || checkBrightnessMean(A_RaidRare.m_rect, C_Color_Rare) > 250);    // rare beam ~= orange
                             */
            bool atRaidScreen = checkAverageColorMatch(A_RaidMenu[0].m_rect, C_Color_Black)
                             && checkAverageColorMatch(A_RaidMenu[1].m_rect, C_Color_White)
                             && checkAverageColorMatch(A_RaidMenu[2].m_rect, C_Color_White)
                             && checkAverageColorMatch(A_RaidMenu[3].m_rect, C_Color_White);
            if (atRaidScreen)
            {
                m_substage = SS_APress;
                setState_frameAnalyzeRequest();
                emit printLog("Inviting others...");

                m_videoManager->clearCaptures();
                m_videoManager->setAreas({A_RaidMenu[1],A_RaidMenu[2],A_RaidMenu[3]});
            }
            else
            {
                setState_runCommand(C_ASpam);
            }
        }
        break;
    }
    case SS_APress:
    {
        if (state == S_CaptureReady)
        {
            bool atInviteOthers = checkAverageColorMatch(A_RaidMenu[1].m_rect, C_Color_White)
                               && checkAverageColorMatch(A_RaidMenu[2].m_rect, C_Color_Black)
                               && checkAverageColorMatch(A_RaidMenu[3].m_rect, C_Color_White);
            if (atInviteOthers)
            {
                m_curBattle++;
                if (m_curBattle > m_maxBattle)
                {
                    m_substage = SS_QuitRaid;
                    setState_runCommand(C_QuitRaid);
                }
                else
                {
                    m_substage = SS_StartRaid;
                    setState_runCommand(C_StartRaid);
                    emit printLog("Starting raid (" + QString::number(m_curBattle) + ")");
                }

                m_videoManager->clearCaptures();
            }
            else
            {
                setState_runCommand(C_APress);
            }
        }
        else if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        break;
    }
    case SS_StartRaid:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_ASpam;
            setState_runCommand(C_ASpam);

            m_videoManager->setAreas(A_RaidMenu);
        }
        break;
    }
    case SS_QuitRaid:
    {
        if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
