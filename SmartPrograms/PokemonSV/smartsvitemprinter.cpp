#include "smartsvitemprinter.h"

SmartSVItemPrinter::SmartSVItemPrinter
(
    Settings settings,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(settings)
{
    init();

    // convert index to actual job counts
    switch (m_programSettings.m_jobs)
    {
        case 0: m_programSettings.m_jobs = 1; break;
        case 1: m_programSettings.m_jobs = 5; break;
        case 2: m_programSettings.m_jobs = 10; break;
    }
}

void SmartSVItemPrinter::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartSVItemPrinter::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;

    m_targetDateTime.setTimeSpec(Qt::UTC);
    m_targetDateTime.setDate(QDate(1970,1,1));
    m_targetDateTime.setTime(QTime(0,0));

    m_addMinute = false;
    m_bonusActive = false;
}

void SmartSVItemPrinter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_targetDateTime = m_targetDateTime.addSecs(m_programSettings.m_seed);
        emit printLog("Seed = " + QString::number(m_programSettings.m_seed) + ", Date Time = " + m_targetDateTime.toString("yyyy-MM-dd hh:mm:ss"));

        m_substage = SS_Talk;
        setState_runCommand(C_Talk);
        break;
    }
    case SS_Talk:
    {
        if (state == S_CommandFinished)
        {

            // Not enough time from pressing OK to accepting NPC
            if (m_targetDateTime.time().second() <= 3)
            {
                emit printLog("Target seconds less than 3 seconds, will be setting one minue early", LOG_WARNING);
                m_targetDateTime = m_targetDateTime.addSecs(-60);
                m_addMinute = true;
            }
            else
            {
                m_addMinute = false;
            }

            m_substage = SS_ToTime;
            setState_runCommand(C_ToTime);
        }
        break;
    }
    case SS_ToTime:
    {
        if (state == S_CommandFinished)
        {
            m_currentDateTime = QDateTime::currentDateTime();
            if (m_currentDateTime.time().second() > 55)
            {
                // last 5s of a minute, delay
                emit printLog("Close to last 5s of a minute, delaying until minute completes...", LOG_WARNING);
                runNextStateDelay(6000);
            }
            else
            {
                emit printLog("Syncing time...");
                setState_runCommand(C_SyncTime);

                switch (m_settings->getDateArrangement())
                {
                    case DA_JP: m_substage = SS_ChangeYear; break;
                    case DA_EU: m_substage = SS_ChangeDay; break;
                    case DA_US: m_substage = SS_ChangeMonth; break;
                }
            }
        }
        break;
    }
    case SS_ChangeYear:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.date().year() - m_currentDateTime.date().year();
            setState_runCommand(getCommandFromDiff(diff));
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: m_substage = SS_ChangeMonth; break;
                case DA_EU: m_substage = SS_ChangeHour; break;
                case DA_US: m_substage = SS_ChangeHour; break;
            }
        }
        break;
    }
    case SS_ChangeMonth:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.date().month() - m_currentDateTime.date().month();
            setState_runCommand(getCommandFromDiff(diff));
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: m_substage = SS_ChangeDay; break;
                case DA_EU: m_substage = SS_ChangeYear; break;
                case DA_US: m_substage = SS_ChangeDay; break;
            }
        }
        break;
    }
    case SS_ChangeDay:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.date().day() - m_currentDateTime.date().day();
            setState_runCommand(getCommandFromDiff(diff));
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: m_substage = SS_ChangeHour; break;
                case DA_EU: m_substage = SS_ChangeMonth; break;
                case DA_US: m_substage = SS_ChangeYear; break;
            }
        }
        break;
    }
    case SS_ChangeHour:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.time().hour() - m_currentDateTime.time().hour();
            if (m_settings->getDateArrangement() == DA_US)
            {
                if (diff > 12)
                {
                    diff -= 12;
                }
                else if (diff <= -12)
                {
                    diff += 12;
                }
            }

            m_substage = SS_ChangeMin;
            setState_runCommand(getCommandFromDiff(diff));
        }
        break;
    }
    case SS_ChangeMin:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.time().minute() - m_currentDateTime.time().minute();
            setState_runCommand(getCommandFromDiff(diff));
            switch (m_settings->getDateArrangement())
            {
            case DA_JP: m_substage = SS_Delay; break;
            case DA_EU: m_substage = SS_Delay; break;
            case DA_US: m_substage = SS_ChangeAP; break;
            }
        }
        break;
    }
    case SS_ChangeAP:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_Delay;

            if ((m_targetDateTime.time().hour() >= 12 && m_currentDateTime.time().minute() < 12)
             || (m_targetDateTime.time().hour() < 12 && m_currentDateTime.time().minute() >= 12))
            {
                setState_runCommand(getCommandFromDiff(1));
            }
            else
            {
                setState_runCommand(getCommandFromDiff(0));
            }
        }
        break;
    }
    case SS_Delay:
    {
        if (state == S_CommandFinished)
        {
            int targetSecond = m_targetDateTime.time().second();
            if (m_addMinute)
            {
                targetSecond += 60;
            }

            int delay = qCeil((double(targetSecond) - m_programSettings.m_delay + 0.3) * 1000.0 / 48.05);
            emit printLog("Waiting " + QString::number(targetSecond) + "s until black screen...");

            m_substage = SS_Jobs;
            setState_runCommand("A,1,Nothing,2,Home,1,Nothing,26,Home,1,Nothing," + QString::number(delay - 30) + ",ASpam,10");
        }
        break;
    }
    case SS_Jobs:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Blue});
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 10000)
            {
                setState_error("Unable to detect material screen for too long");
            }
            else if (checkBrightnessMeanTarget(A_Blue.m_rect, C_Color_Blue, 230))
            {
                m_substage = SS_JobsStart;
                setState_runCommand("Nothing,20");
                m_videoManager->setAreas({A_Jobs});
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_JobsStart:
    {
        if (state == S_CommandFinished)
        {
            if (m_bonusActive)
            {
                setState_ocrRequest(A_JobsBonus.m_rect, C_Color_TextW);
            }
            else
            {
                setState_ocrRequest(A_Jobs.m_rect, C_Color_TextW);
            }
        }
        else if (state == S_OCRReady)
        {
            int jobs = 0;
            getOCRNumber(jobs);

            if (jobs != 1 && jobs != 5 && jobs != 10)
            {
                if (!m_bonusActive)
                {
                    // bonus may already be active
                    m_bonusActive = true;
                    m_programSettings.m_jobs = 10;
                    emit printLog("Bonus maybe active, forcing 10 jobs", LOG_WARNING);

                    setState_runCommand("Nothing,10");
                    m_videoManager->setAreas({A_JobsBonus});
                    break;
                }

                setState_error("Unable to detect jobs count");
                break;
            }

            if (jobs != m_programSettings.m_jobs)
            {
                setState_runCommand("L,1,Nothing,20");
            }
            else
            {
                emit printLog("Jobs count set to " + QString::number(m_programSettings.m_jobs) + ", now printing items!");
                m_videoManager->clearCaptures();

                m_substage = SS_Print;
                setState_runCommand(C_Print);
            }
        }
        break;
    }
    case SS_Print:
    {
        if (state == S_CommandFinished)
        {
            if (m_programSettings.m_syncTime)
            {
                emit printLog("Syncing time...");
                m_substage = SS_Finish;
                setState_runCommand(m_commands[C_ToTime] + ",A,2,Nothing,1,A,6,Nothing,1,Home,1,Nothing,26,Home,1");
            }
            else
            {
                setState_completed();
            }
        }
        break;
    }
    case SS_Finish:
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

QString SmartSVItemPrinter::getCommandFromDiff(int diff)
{
    if (diff < 0)
    {
        return "DDown,1,Nothing,1,Loop," + QString::number(-diff) + ",DRight,1,Nothing,1";
    }
    else if (diff > 0)
    {
        return "DUp,1,Nothing,1,Loop," + QString::number(diff) + ",DRight,1,Nothing,1";
    }
    else
    {
        return "DRight,1,Nothing,1";
    }
}
