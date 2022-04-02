#include "smartplastaticspawn.h"

SmartPLAStaticSpawn::SmartPLAStaticSpawn
(
    QString const& staticPokemon,
    bool ignoreEarlyShiny,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_staticPokemon(staticPokemon)
    , m_ignoreEarlyShiny(ignoreEarlyShiny)
{
    init();

    QSettings settings(SMART_COMMAND_PATH + QString("SmartPLAStaticSpawn.ini"), QSettings::IniFormat);
    QStringList names = settings.childGroups();
    if (!names.contains(staticPokemon))
    {
        setState_error("Invalid static pokemon name");
        return;
    }

    settings.beginGroup(staticPokemon);
    int areaID = settings.value("AreaID", 0).toInt();
    if (areaID < 1 || areaID > 5)
    {
        setState_error("Invalid AreaID");
        return;
    }
    m_areaType = PLAAreaType(areaID - 1);

    m_campID = settings.value("CampID", 0).toInt();
    bool validCamp = false;
    PokemonDatabase::getPLACampString(m_areaType, m_campID, &validCamp);
    if (!validCamp)
    {
        setState_error("Invalid CampID");
        return;
    }

    m_navigateCommands = settings.value("Commands", "Nothing,10").toString();
    QString errorMsg;
    if (!SmartProgramBase::validateCommand(m_navigateCommands, errorMsg))
    {
        setState_error(errorMsg);
        return;
    }

    m_ignoreShinyTimeMS = settings.value("IgnoreShinyTimeMS", 0).toInt();
    if (!m_ignoreEarlyShiny || m_ignoreShinyTimeMS <= 0)
    {
        m_ignoreEarlyShiny = false;
        m_ignoreShinyTimeMS = 0;
    }
}

void SmartPLAStaticSpawn::populateStaticPokemon(QComboBox *cb)
{
    QSettings settings(SMART_COMMAND_PATH + QString("SmartPLAStaticSpawn.ini"), QSettings::IniFormat);
    QStringList names = settings.childGroups();
    names.sort();

    cb->clear();
    for (QString const& name : names)
    {
        cb->addItem(name);
    }
}

void SmartPLAStaticSpawn::init()
{
    SmartProgramBase::init();
}

void SmartPLAStaticSpawn::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartPLAStaticSpawn::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statAttempts, "Attempts");
        initStat(m_statShiny, "Shinies");
        initStat(m_statError, "Errors");

        emit printLog("Pokemon: " + m_staticPokemon
                      + ", Area: " + PokemonDatabase::PLAAreaTypeToString(m_areaType)
                      + ", Camp: " + PokemonDatabase::getPLACampString(m_areaType, m_campID)
                      + ", Ignore Shiny Time: " + QString::number(m_ignoreShinyTimeMS) + "ms");

        // Setup sound detection
        m_shinySoundID = m_audioManager->addDetection("PokemonLA/ShinySFX", 0.19f, 5000);
        m_shinyDetected = false;
        connect(m_audioManager, &AudioManager::soundDetected, this, &SmartPLAStaticSpawn::soundDetected);

        if (m_ignoreEarlyShiny)
        {
            m_timer.setSingleShot(true);
            connect(&m_timer, &QTimer::timeout, this, &SmartPLAStaticSpawn::ignoreShinyTimeout);
        }

        m_substage = SS_Restart;
        runRestartCommand();
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
            if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect black screen on restart, HOME button might have missed, retrying...", LOG_ERROR);
                runRestartCommand();
            }
            else
            {
                setState_frameAnalyzeRequest();
                m_videoManager->setAreas({A_Title});

                m_substage = SS_Title;
            }
        }
        break;
    }
    case SS_Title:
    case SS_GameStart:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_Title)
                {
                    emit printLog("Title detected!");
                    setState_runCommand("ASpam,60,Nothing,30");
                    m_substage = SS_GameStart;
                }
                else
                {
                    m_substage = SS_DetectMap;
                    setState_runCommand("A,1,Nothing,20");

                    m_elapsedTimer.restart();
                    m_videoManager->setAreas({A_Map});
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_DetectMap:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 10000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect map for too long...");
            }
            else if (checkBrightnessMeanTarget(A_Map.m_rect, C_Color_Map, 240))
            {
                m_substage = SS_EnterArea;
                QString const toAreaCommand = "DRight,1,Nothing,1,Loop," + QString::number(m_areaType + 1);

                if (PokemonDatabase::getIsPLACampSelectableAtVillage(m_areaType, m_campID))
                {
                    // Head to camp
                    emit printLog("Map detected, heading to " + PokemonDatabase::PLAAreaTypeToString(m_areaType) + " " + PokemonDatabase::getPLACampString(m_areaType, m_campID));
                    QString downPresses = "";
                    if (m_campID > 1)
                    {
                        downPresses = ",DDown,1,Nothing,1,Loop," + QString::number(m_campID - 1);
                    }
                    setState_runCommand(toAreaCommand + ",A,16,Loop,1" + downPresses + ",ASpam,80");
                }
                else
                {
                    // Can't head the camp from village, do it again after we entered area
                    emit printLog(PokemonDatabase::getPLACampString(m_areaType, m_campID) + " not accessable from village, heading to " + PokemonDatabase::PLAAreaTypeToString(m_areaType) + " first");
                    setState_runCommand(toAreaCommand + ",ASpam,80");
                }

                m_videoManager->clearAreas();
            }
            else
            {
                setState_runCommand("A,1,Nothing,20");
            }
        }
        break;
    }
    case SS_EnterArea:
    {
        if (state == S_CommandFinished)
        {
            m_elapsedTimer.restart();
            m_videoManager->setAreas({A_Loading});
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 5000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect loading screen for too long...");
                break;
            }
            else if (checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                // Detect loading screen
                m_substage = PokemonDatabase::getIsPLACampSelectableAtVillage(m_areaType, m_campID) ? SS_GotoCamp : SS_LoadingToArea;
            }
        }

        setState_frameAnalyzeRequest();
        break;
    }
    case SS_LoadingToArea:
    case SS_GotoCamp:
    {
        if (state == S_CommandFinished)
        {
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            // Detect entering area
            if (!checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                if (m_substage == SS_GotoCamp)
                {
                    incrementStat(m_statAttempts);
                    m_substage = SS_DetectShiny;
                    setState_runCommand(m_navigateCommands);

                    if (m_ignoreEarlyShiny)
                    {
                        m_timer.start(m_ignoreShinyTimeMS);
                    }

                    m_audioManager->startDetection(m_shinySoundID);
                    m_videoManager->clearCaptures();
                }
                else
                {
                    // Goto the arena/settlement that weren't accessible in village
                    m_substage = SS_GotoCamp;

                    emit printLog("Heading to " + PokemonDatabase::getPLACampString(m_areaType, m_campID));
                    QString downPresses = "";
                    if (m_campID > 1)
                    {
                        downPresses = ",DDown,1,Nothing,1,Loop," + QString::number(m_campID - 1);
                    }
                    setState_runCommand("Minus,1,Nothing,20,X,16,Loop,1" + downPresses + ",ASpam,60");
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_DetectShiny:
    {
        if (state == S_CommandFinished)
        {
            // We might have found a shiny but chose to ignore
            if (!m_shinyDetected)
            {
                emit printLog("No shiny found, restarting...", LOG_WARNING);
            }
            m_shinyDetected = false;

            m_substage = SS_Restart;
            runRestartCommand();

            m_timer.stop();
            m_audioManager->stopDetection(m_shinySoundID);
        }
        break;
    }
    case SS_Capture:
    {
        if (m_shinyDetected)
        {
            setState_runCommand("Capture,22,Minus,1,Nothing,20,Home,1");
            m_shinyDetected = false;
        }
        else if (state == S_CommandFinished)
        {
            setState_completed();
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartPLAStaticSpawn::runRestartCommand()
{
    setState_runCommand("Home,1,Nothing,21,X,1,ASpam,240");
}

void SmartPLAStaticSpawn::ignoreShinyTimeout()
{
    emit printLog("Shiny ignore timeout");
}

void SmartPLAStaticSpawn::soundDetected(int id)
{
    if (id != m_shinySoundID) return;

    if (m_substage == SS_DetectShiny)
    {
        incrementStat(m_statShiny);
        m_shinyDetected = true;

        if (m_ignoreEarlyShiny && m_timer.remainingTime() > 0)
        {
            emit printLog("Shiny sound detected but user has set ignore early shinies...", LOG_WARNING);
        }
        else
        {
            emit printLog("SHINY POKEMON FOUND!", LOG_SUCCESS);
            m_substage = SS_Capture;
            runNextStateContinue();
        }
    }
}
