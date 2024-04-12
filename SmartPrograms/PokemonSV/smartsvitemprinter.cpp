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

    if (!getPreset(m_programSettings.m_presetName, m_preset))
    {
        setState_error("Unable to load preset " + m_programSettings.m_presetName);
        return;
    }

    if (m_programSettings.m_bonusType == BonusType::Double && !getPreset("*Double Bonus*", m_bonusPreset))
    {
        setState_error("Unable to load preset *Double Bonus*");
        return;
    }

    if (m_programSettings.m_bonusType == BonusType::Lotto && !getPreset("*Ball Lotto*", m_bonusPreset))
    {
        setState_error("Unable to load preset *Ball Lotto*");
        return;
    }
}

bool SmartSVItemPrinter::getPreset(const QString &presetName, Preset &preset)
{
    QSettings settings(SMART_COMMAND_PATH + QString("SmartSVItemPrinter.ini"), QSettings::IniFormat);
    QStringList names = settings.childGroups();
    if (presetName.isEmpty() || !names.contains(presetName))
    {
        return false;
    }

    settings.beginGroup(presetName);
    preset.m_name = presetName;

    QStringList const date = settings.value("Date", "").toString().split("-");
    if (date.size() != 3)
    {
        return false;
    }

    bool ok = false;
    int year = date[0].toInt(&ok);  if (!ok) return false;
    int month = date[1].toInt(&ok); if (!ok) return false;
    int day = date[2].toInt(&ok);   if (!ok) return false;

    QStringList const time = settings.value("Time", "").toString().split("-");
    if (time.size() != 3)
    {
        return false;
    }

    int hour = time[0].toInt(&ok);  if (!ok) return false;
    int min = time[1].toInt(&ok);   if (!ok) return false;
    int sec = time[2].toInt(&ok);   if (!ok) return false;
    preset.m_dateTime = QDateTime(QDate(year, month, day), QTime(hour, min, sec));
    preset.m_seed = QDateTime(QDate(1970,1,1), QTime(0,0)).secsTo(preset.m_dateTime) + 3600;

    preset.m_jobs = settings.value("Jobs", 1).toInt();
    if (preset.m_jobs != 1 && preset.m_jobs != 5 && preset.m_jobs != 10)
    {
        return false;
    }

    preset.m_rewards = settings.value("Rewards", "").toString();
    return true;
}

void SmartSVItemPrinter::populatePresets(QComboBox *cb)
{
    QSettings settings(SMART_COMMAND_PATH + QString("SmartSVItemPrinter.ini"), QSettings::IniFormat);
    QStringList names = settings.childGroups();
    names.sort();

    cb->clear();
    for (QString const& name : names)
    {
        cb->addItem(name);
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

    m_addMinute = false;
    m_bonusActive = false;
    m_useCount = 0;
}

void SmartSVItemPrinter::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        emit printLog("Use left: " + QString::number(m_programSettings.m_useCount));
        m_substage = SS_Talk;
        setState_runCommand(C_Talk);
        break;
    }
    case SS_Talk:
    {
        if (state == S_CommandFinished)
        {
            if (m_programSettings.m_bonusType != BonusType::None && !m_bonusActive)
            {
                // need to active bonus first
                emit printLog("Seed = " + QString::number(m_bonusPreset.m_seed));
                m_targetDateTime = m_bonusPreset.m_dateTime;
                m_targetJobs = m_bonusPreset.m_jobs;
            }
            else
            {
                emit printLog("Seed = " + QString::number(m_preset.m_seed));
                m_targetDateTime = m_preset.m_dateTime;
                m_targetJobs = m_preset.m_jobs;
            }

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
                emit printLog("Syncing time");
                m_substage = SS_SyncTime;
                setState_runCommand(C_SyncTime);
            }
        }
        break;
    }
    case SS_SyncTime:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Current time: " + m_currentDateTime.toString("yyyy-MM-dd hh:mm"));
            emit printLog("Setting date time to: " + m_targetDateTime.toString("yyyy-MM-dd hh:mm"));
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: m_substage = SS_ChangeYear; break;
                case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
                case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
            }
            runNextStateContinue();
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
                case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
                case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
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
                case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
                case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
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
                case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
                case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
            }
        }
        break;
    }
    case SS_ChangeHour:
    {
        if (state == S_CommandFinished)
        {
            int diff = m_targetDateTime.time().hour() - m_currentDateTime.time().hour();
            setState_runCommand(getCommandFromDiff(diff));
            switch (m_settings->getDateArrangement())
            {
                case DA_JP: m_substage = SS_ChangeMin; break;
                case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
                case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
            }
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
            case DA_EU: m_substage = SS_ChangeMonth; break; // TODO:
            case DA_US: m_substage = SS_ChangeMonth; break; // TODO:
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
            m_videoManager->setAreas({A_Blue,(m_bonusActive ? A_JobsBonus : A_Jobs)});
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
                    // bonus may already be active, skip to next date
                    m_bonusActive = true;
                    emit printLog("Bonus may already be active, tryinig again", LOG_WARNING);

                    if (m_programSettings.m_bonusType == BonusType::None)
                    {
                        setState_runCommand("Nothing,10");
                        m_videoManager->setAreas({A_Blue,(m_bonusActive ? A_JobsBonus : A_Jobs)});
                    }
                    else
                    {
                        m_substage = SS_Talk;
                        setState_runCommand("BSpam,10,Nothing,80," + m_commands[C_Talk]);
                        m_videoManager->clearCaptures();
                    }
                    break;
                }

                setState_error("Unable to detect jobs count");
                break;
            }

            if (jobs != m_targetJobs)
            {
                setState_runCommand("L,1,Nothing,20");
            }
            else
            {
                // TODO: same seed
                if (m_programSettings.m_bonusType == BonusType::None)
                {
                    m_useCount++;
                }
                else
                {
                    if (m_bonusActive)
                    {
                        m_useCount++;
                    }

                    // expecting bonus to be toggled
                    m_bonusActive = !m_bonusActive;
                }

                emit printLog("Jobs count set to " + QString::number(m_targetJobs) + ", now printing items!");
                QString command = m_commands[C_Print];
                if (m_useCount < m_programSettings.m_useCount)
                {
                    // return to selection menu
                    command += ",ASpam,240";
                }

                m_substage = SS_Print;
                setState_runCommand(command);
                m_videoManager->clearCaptures();
            }
        }
        break;
    }
    case SS_Print:
    {
        if (state == S_CommandFinished)
        {
            if (m_useCount >= m_programSettings.m_useCount)
            {
                setState_completed();
            }
            else
            {
                m_timer.restart();
                if (!m_bonusActive)
                {
                    emit printLog("Use left: " + QString::number(m_programSettings.m_useCount - m_useCount));
                }

                setState_frameAnalyzeRequest();
                m_videoManager->setAreas({A_Blue});
            }
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                setState_error("Unable to detect material screen for too long");
            }
            else if (checkBrightnessMeanTarget(A_Blue.m_rect, C_Color_Blue, 230))
            {
                m_substage = SS_Talk;
                setState_runCommand("BSpam,10,Nothing,80," + m_commands[C_Talk]);
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Restart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                emit printLog("Unable to detect black screen after restarting, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_Title;
                setState_frameAnalyzeRequest();
                m_timer.restart();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Title:
    {
        if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                emit printLog("Unable to detect title screen, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                emit printLog("Title detected!");
                m_substage = SS_GameStart;
                setState_runCommand("ASpam,310");
                m_videoManager->clearCaptures();
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
            m_videoManager->setAreas({A_Title});
            m_timer.restart();
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 30000)
            {
                emit printLog("Unable to detect game start, attempting restart again...", LOG_ERROR);
                setState_runCommand(C_Restart);
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_Talk;
                setState_runCommand(C_Talk);
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
