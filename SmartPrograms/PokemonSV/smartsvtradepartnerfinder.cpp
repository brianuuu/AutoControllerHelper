#include "smartsvtradepartnerfinder.h"

#include "smartsveggoperation.h"

SmartSVTradePartnerFinder::SmartSVTradePartnerFinder
(
    QString partnerName,
    bool spamA,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_partnerName(partnerName)
    , m_spamA(spamA)
{
    init();

    if (m_partnerName.isEmpty())
    {
        setState_error("Please enter partner name");
    }
}

void SmartSVTradePartnerFinder::init()
{
    SmartProgramBase::init();
}

void SmartSVTradePartnerFinder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartSVTradePartnerFinder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        emit printLog("Looking for partner name: " + m_partnerName);
        m_substage = SS_FindPartner;
        setState_runCommand("ASpam,60");
        m_videoManager->setAreas({A_Name,SmartSVEggOperation::GetBoxCaptureAreaOfPos(1,1).m_rect});
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_LinkTrade.m_rect, C_Color_Yellow, 200))
            {
                emit printLog("Start finding partner");
                m_substage = SS_FindPartner;
                setState_runCommand("ASpam,60");
                m_videoManager->setAreas({A_Name,SmartSVEggOperation::GetBoxCaptureAreaOfPos(1,1).m_rect});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_FindPartner:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 60000)
            {
                emit printLog("Unable to find partner for too long or communication error has occured, retrying", LOG_WARNING);
                setState_runCommand("ASpam,100");
            }
            else if (checkBrightnessMeanTarget(SmartSVEggOperation::GetBoxCaptureAreaOfPos(1,1).m_rect, C_Color_Yellow, 130))
            {
                emit printLog("Detected in Box menu, checking trade partner's name...");
                setState_ocrRequest(A_Name.m_rect, C_Color_Black);
                m_videoManager->setAreas({A_Name});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        else if (state == S_OCRReady)
        {
            PokemonDatabase::OCREntries const entry =
            {
                {"Found", {m_partnerName}}
            };

            if (matchStringDatabase(entry).isEmpty())
            {
                emit printLog("Not the partner we want, disconnecting", LOG_WARNING);
                m_substage = SS_WrongPartner;
                setState_runCommand("B,10");
                m_videoManager->setAreas({A_Dialog});
            }
            else
            {
                if (m_spamA)
                {
                    emit printLog("Partner matched! Spamming A until we have returned to Poke Portal", LOG_SUCCESS);
                    m_substage = SS_CorrectPartner;
                    setState_runCommand("A,3,Nothing,3,Loop,0",true);
                    m_videoManager->setAreas({A_LinkTrade});
                }
                else
                {
                    emit printLog("Partner matched!", LOG_SUCCESS);
                    setState_completed();
                }
            }
        }
        break;
    }
    case SS_WrongPartner:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_Dialog.m_rect, C_Color_Dialog, 220))
            {
                emit printLog("Leaving trade manu...");
                m_substage = SS_Restart;
                setState_runCommand("ASpam,40");
                m_videoManager->setAreas({A_LinkTrade});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CorrectPartner:
    {
        if (state == S_CommandFinished)
        {
            setState_completed();
        }
        else if (state == S_CaptureReady)
        {
            if (checkBrightnessMeanTarget(A_LinkTrade.m_rect, C_Color_Yellow, 200))
            {
                emit printLog("Completed!");
                setState_runCommand("Home,2");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}
