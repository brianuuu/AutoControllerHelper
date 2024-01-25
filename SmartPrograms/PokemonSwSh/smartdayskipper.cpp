#include "smartdayskipper.h"

QMap<QString, QImage> SmartDaySkipper::m_imageTests;

SmartDaySkipper::SmartDaySkipper
(
    int skips,
    bool raidMode,
    QString const& pokemonList,
    QLabel* estimateLabel,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_skipsLeft(skips)
    , m_skippedDays(0)
    , m_raidMode(raidMode)
    , m_dateArrangement(parameter.settings->getDateArrangement())
    , m_estimateLabel(estimateLabel)
{
    init();

    if (m_raidMode)
    {
        if (pokemonList.isEmpty())
        {
            setState_error("No Pokemon listed");
            return;
        }

        m_pokemonList = pokemonList.split(',');
        for (QString& pokemon : m_pokemonList)
        {
            if (pokemon.isEmpty())
            {
                setState_error("Empty name detected");
                return;
            }
            else if (m_pokemonList.count(pokemon) > 1)
            {
                setState_error(pokemon + " is duplicated");
                return;
            }
            else if (!pokemon.front().isUpper())
            {
                pokemon[0] = pokemon[0].toUpper();
            }

            if (!PokemonDatabase::getList_SwShSprites().contains(pokemon))
            {
                setState_error(pokemon + " is not a valid Pokemon");
                return;
            }
        }

        // this is not thread-safe, but should definitely be finished by the time we start using them
        QtConcurrent::run(this, &SmartDaySkipper::loadImages);
    }
}

void SmartDaySkipper::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartDaySkipper::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_skippedDays = 0;
    m_isFound = false;
    m_imageTestIndex = -1;
}

void SmartDaySkipper::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        if (m_raidMode)
        {
            m_substage = SS_StartRaid;
            setState_runCommand("ASpam,300", true);
            emit printLog("Raid 3 Day Skip Mode Enabled");
            m_videoManager->setAreas({A_Invite, A_Switch});
            break;
        }

        switch (m_dateArrangement)
        {
            case DA_JP: emit printLog("Date Arrangement: JP is set"); break;
            case DA_EU: emit printLog("Date Arrangement: EU is set"); break;
            case DA_US: emit printLog("Date Arrangement: US is set"); break;
        }

        m_substage = SS_ToOK;
        setState_runCommand(C_StartOK);
        emit printLog("Moving to OK button...");

        break;
    }
    case SS_ToOK:
    {
        m_substage = SS_Back2000;
        switch (m_dateArrangement)
        {
            case DA_JP: setState_runCommand(C_Back2000JP); break;
            case DA_EU: setState_runCommand(C_Back2000EU); break;
            case DA_US: setState_runCommand(C_Back2000US); break;
        }
        emit printLog("Rolling back to year 2000...");

        break;
    }
    case SS_Back2000:
    {
        // Sync time, this will skip a day since year is advanced from 2000
        m_skipsLeft--;
        m_skippedDays++;
        m_startDateTime = QDateTime::currentDateTime();

        runSyncTime(); // updated m_date
        m_substage = SS_Skip;

        QString msg = "Syncing time (1 day skipped), current date: ";
        switch (m_dateArrangement)
        {
            case DA_JP: msg += m_date.toString("yyyy/MM/dd"); break;
            case DA_EU: msg += m_date.toString("dd/MM/yyyy"); break;
            case DA_US: msg += m_date.toString("MM/dd/yyyy"); break;
        }
        emit printLog(msg);

        break;
    }
    case SS_Skip:
    {
        if (state == S_CommandFinished)
        {
            // Only print every 100, 10, and last 10 to prevent spamming
            if (m_skipsLeft % 100 == 0 || (m_skipsLeft < 100 && m_skipsLeft % 10 == 0) || (m_skipsLeft < 10))
            {
                emit printLog("Days left = " + QString::number(m_skipsLeft));
            }

            // Show estimated time and days left on label
            double timeLeft = ((double)m_startDateTime.secsTo(QDateTime::currentDateTime())) * (double)m_skipsLeft / (double)m_skippedDays;
            m_estimateLabel->setText(secondsToString(timeLeft) + " (" + QString::number(m_skipsLeft) + " skips left)");

            if (m_skipsLeft == 0)
            {
                m_substage = SS_BackToGame;
                setState_runCommand(m_commands[C_SyncTime] + "," + m_commands[C_BackToGame]);
                emit printLog("Skip completed, syncing time and going back to game");
            }
            else
            {
                if (m_dateArrangement == DA_JP)
                {
                    if (m_date == QDate(2060,12,31))
                    {
                        // Sync time if we reach max date
                        runSyncTime();
                        emit printLog("Syncing time, no skip");
                    }
                    else
                    {
                        // For JP, do normal year/month/day skips after syncing
                        QDate tempDate = m_date.addDays(1);
                        if (tempDate.year() > m_date.year())
                        {
                            setState_runCommand(C_SkipJPYear);
                        }
                        else if (tempDate.month() > m_date.month())
                        {
                            setState_runCommand(C_SkipJPMonth);
                        }
                        else
                        {
                            setState_runCommand(C_SkipEU_JPDay);
                        }

                        m_date = tempDate;
                        m_skipsLeft--;
                        m_skippedDays++;
                    }
                }
                else
                {
                    // EU/US both skip year, roll back by syncing time if year is maxed
                    if (m_date.year() == 2060)
                    {
                        runSyncTime();
                        emit printLog("Syncing time, no skip");
                    }
                    else
                    {
                        m_skipsLeft--;
                        m_skippedDays++;
                        m_date = m_date.addYears(1);
                        setState_runCommand(m_dateArrangement == DA_EU ? C_SkipEU_JPDay : C_SkipUS);
                    }
                }
            }
        }

        break;
    }
    case SS_StartRaid:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Unable to detect raid start, restarting game", LOG_ERROR);
            m_substage = SS_RestartGame;
            setState_runCommand("Home,1,Nothing,16,X,4,ASpam,200");
        }
        else if (state == S_CaptureReady)
        {
            if (checkAverageColorMatch(A_Invite.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_Invite;
                setState_runCommand("Nothing,10");

                if (m_skippedDays == 3)
                {
                    m_videoManager->setAreas({A_Sprite, A_Switch});
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_Invite:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (checkAverageColorMatch(A_Switch.m_rect, QColor(0,0,0)))
            {
                if (m_skippedDays < 3)
                {
                    emit printLog("Raid start detected, skipping year...");
                    m_substage = SS_ToSyncTime;
                    setState_runCommand(C_ToSyncTime);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    // test image concurrently
                    m_substage = SS_CheckPokemon;
                    QtConcurrent::run(this, &SmartDaySkipper::testImages);
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_CheckPokemon:
    {
        if (state == S_CaptureReady)
        {
            // finished, what's the highest one?
            QStringList const& list = PokemonDatabase::getList_SwShSprites();
            int foundIndex = -1;
            for (int j = 0; j < m_pokemonList.size(); j++)
            {
                if (list[m_imageTestIndex].contains(m_pokemonList[j]))
                {
                    foundIndex = j;
                    break;
                }
            }

            if (foundIndex == -1)
            {
                emit printLog(list[m_imageTestIndex] + " is not in listed Pokemon, syncing time and restarting game...");
                m_substage = SS_ToSyncTime;
                setState_runCommand(C_ToSyncTime);
            }
            else
            {
                m_isFound = true;
                emit printLog(m_pokemonList[foundIndex] + " is found! Syncing time before finishing", LOG_SUCCESS);
                m_substage = SS_ToSyncTime;
                setState_runCommand(m_commands[C_ToSyncTime] + ",ASpam,4,Nothing,5," + m_commands[C_BackToGame]);
            }

            m_videoManager->clearCaptures();
        }
        break;
    }
    case SS_ToSyncTime:
    {
        if (state == S_CommandFinished)
        {
            if (m_isFound)
            {
                setState_completed();
            }
            else if (m_skippedDays == 3)
            {
                m_skippedDays = 0;
                m_substage = SS_RestartGame;
                setState_runCommand(C_RestartGame);
            }
            else
            {
                m_skippedDays++;
                m_substage = SS_SkipYearRaid;
                switch (m_dateArrangement)
                {
                    case DA_JP: setState_runCommand(C_SkipJPYearRaid); break;
                    case DA_EU: setState_runCommand(C_SkipEUYearRaid); break;
                    case DA_US: setState_runCommand(C_SkipUSYearRaid); break;
                }
            }
        }
        break;
    }
    case SS_SkipYearRaid:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_BackToGame;
            setState_runCommand(C_BackToGame);
            m_videoManager->setAreas({A_Quit});
        }
        break;
    }
    case SS_RestartGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen is not black anymore (intro plays)
            bool introStarted = !checkPixelColorMatch(P_Center.m_point, QColor(0,0,0));
            if (introStarted)
            {
                m_substage = SS_StartGame;
                setState_runCommand(C_StartGameA);
                emit printLog("Intro started, entering game...");
            }
            else
            {
                runNextStateContinue();
            }
        }
        break;
    }
    case SS_StartGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen to be black
            bool enteredBlackScreen = checkPixelColorMatch(P_Center.m_point, QColor(0,0,0));
            if (enteredBlackScreen)
            {
                m_substage = SS_EnterGame;

                m_videoManager->clearCaptures();
                m_videoManager->setAreas({A_EnterGame});
            }

            runNextStateContinue();
        }

        break;
    }
    case SS_EnterGame:
    {
        if (state == S_CommandFinished)
        {
            m_videoManager->getFrame(m_capture);

            // Wait until screen to be not black anymore
            bool enteredGame = !checkAverageColorMatch(A_EnterGame.m_rect, QColor(0,0,0));
            if (enteredGame)
            {
                m_substage = SS_StartRaid;
                setState_runCommand("ASpam,300", true);
                m_videoManager->setAreas({A_Invite, A_Switch});
            }
            else
            {
                runNextStateContinue();
            }
        }

        break;
    }
    case SS_BackToGame:
    {
        if (state == S_CommandFinished)
        {
            if (m_raidMode)
            {
                emit printLog("Quitting Raid...Currently at Day " + QString::number(m_skippedDays + 1));
                m_substage = SS_QuitRaid;
                setState_runCommand("BSpam,100", true);
                break;
            }

            setState_completed();
            emit printLog("Total time spent: " + secondsToString(m_startDateTime.secsTo(QDateTime::currentDateTime())));
        }
        break;
    }
    case SS_QuitRaid:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Unable to detect raid start, restarting game", LOG_ERROR);
            m_substage = SS_RestartGame;
            setState_runCommand("Home,1,Nothing,16,X,4,ASpam,200");
        }
        else if (state == S_CaptureReady)
        {
            if (checkAverageColorMatch(A_Quit.m_rect, QColor(0,0,0)))
            {
                m_substage = SS_StartRaid;
                setState_runCommand("ASpam,300", true);
                m_videoManager->setAreas({A_Invite, A_Switch});
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

void SmartDaySkipper::loadImages()
{
    if (m_imageTests.empty())
    {
        for (QString const& pokemon : PokemonDatabase::getList_SwShSprites())
        {
            QImage img = QImage(QString(RESOURCES_PATH) + "PokemonSwSh/Silhouettes/" + pokemon + ".bmp");
            m_imageTests[pokemon] = (img.scaled(A_Sprite.m_rect.size()).convertToFormat(QImage::Format_MonoLSB, Qt::MonoOnly));
        }
    }
}

void SmartDaySkipper::testImages()
{
    QStringList const& list = PokemonDatabase::getList_SwShSprites();
    double maxRatio = 0.0;
    for (int i = 0; i < list.size(); i++)
    {
        QImage const& img = m_imageTests[list[i]];
        double ratio = getImageMatch(A_Sprite.m_rect, C_Color_Black, img);
        if (ratio > maxRatio)
        {
            maxRatio = ratio;
            m_imageTestIndex = i;
        }
    }

    runNextStateContinue();
}

bool SmartDaySkipper::is1159PM()
{
    QTime const currentTime = QTime::currentTime();
    return currentTime.hour() == 23 && currentTime.minute() == 59;
}

void SmartDaySkipper::runSyncTime()
{
    bool is1159 = is1159PM();
    if (is1159)
    {
        emit printLog("Current time is 11:59PM, waiting it turns 12:00AM for a natural skip");
        m_skipsLeft--;
        m_skippedDays++;
    }

    setState_runCommand(is1159 ? C_SyncTimeWait : C_SyncTime);
    m_date = QDate::currentDate();
}

QString SmartDaySkipper::secondsToString(double seconds)
{
    QString str;

    qint64 day = 0;
    qint64 hour = 0;
    qint64 minute = 0;
    qint64 second = (qint64)seconds;

    minute = second / 60;
    second = second % 60;

    hour = minute / 60;
    minute = minute % 60;

    day = hour / 24;
    hour = hour % 24;

    str = day > 0 ? QString::number(day) + "d" : "";
    str += QString("%1h%2m%3s")
      .arg(hour, 2, 10, QChar('0'))
      .arg(minute, 2, 10, QChar('0'))
      .arg(second, 2, 10, QChar('0'));

    return str;
}

QString SmartDaySkipper::getToJanuary()
{
    QString cmd = "A,4,LLeft,1,RLeft,1,LLeft,1,";

    // if day is 29-31, we want to get to 28 first
    if (m_date.day() > 28)
    {
        for (int i = 0; i < m_date.day() - 28; i++)
        {
            cmd += "LDown,1,Nothing,1,";
        }
        emit printLog("Setting day to 28th...");
        m_date.setDate(m_date.year(), m_date.month(), 28);
    }

    cmd += "RLeft,1,";
    for (int i = 0; i < m_date.month() - 1; i++)
    {
        cmd += "LDown,1,Nothing,1,";
    }
    cmd += "RRight,1,LRight,1,RRight,1,LRightA,1,Nothing,2";
    m_date.setDate(m_date.year(), 1, m_date.day());
    return cmd;
}
