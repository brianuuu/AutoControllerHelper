#include "smartplaoutbreakfinder.h"

SmartPLAOutbreakFinder::SmartPLAOutbreakFinder
(
    QString const& pokemonString,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
{
    init();

    if (pokemonString.isEmpty())
    {
        setState_error("No Pokemon is listed");
        return;
    }

    m_outbreakPokemonList = pokemonString.split(',');
    for (QString& pokemon : m_outbreakPokemonList)
    {
        if (pokemon.isEmpty())
        {
            setState_error("Empty name detected");
            return;
        }
        else if (m_outbreakPokemonList.count(pokemon) > 1)
        {
            setState_error(pokemon + " is duplicated");
            return;
        }
        else if (!pokemon.front().isUpper())
        {
            pokemon[0] = pokemon[0].toUpper();
        }

        if (!PokemonDatabase::getList_PLAMassOutbreak().contains(pokemon))
        {
            setState_error(pokemon + " is not a valid Mass Outbreak Pokemon");
            return;
        }
    }

    // Get outbreak entries for OCR
    m_allOutbreakEntries = PokemonDatabase::getEntries_PLAMassOutbreak(m_settings->getGameLanguage());
}

void SmartPLAOutbreakFinder::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartPLAOutbreakFinder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    m_firstCheck = true;
    m_areaType = PLAAT_AlabasterIcelands;
    m_readyNextCheck = false;
}

void SmartPLAOutbreakFinder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statChecks, "Checks");
        initStat(m_statError, "Errors");

        emit printLog("Game Language = " + PokemonDatabase::getGameLanguageName(m_settings->getGameLanguage()));

        // Start by talking to Laventon
        m_substage = SS_TalkToLaventon;
        setState_runCommand(C_TalkToLaventon);
        break;
    }
    case SS_WalkToLaventon:
    {
        if (state == S_CommandFinished)
        {
            m_substage = SS_TalkToLaventon;
            setState_runCommand(C_TalkToLaventon);
        }
        break;
    }
    case SS_TalkToLaventon:
    case SS_EnterObsidian:
    {
        if (state == S_CommandFinished)
        {
            m_timer.restart();
            m_videoManager->setAreas({A_Loading});
        }
        else if (state == S_CaptureReady)
        {
            if (m_timer.elapsed() > 5000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect loading screen for too long...");
                break;
            }
            else if (checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                // Detect loading screen
                m_substage = (m_substage == SS_TalkToLaventon) ? SS_LoadingToVillage : SS_LoadingToObsidian;
            }
        }

        setState_frameAnalyzeRequest();
        break;
    }
    case SS_LoadingToVillage:
    case SS_LoadingToObsidian:
    {
        if (state == S_CaptureReady)
        {
            // Detect entering village/obsidian fieldlands
            if (!checkBrightnessMeanTarget(A_Loading.m_rect, C_Color_Loading, 240))
            {
                m_videoManager->clearAreas();
                if (m_substage == SS_LoadingToVillage)
                {
                    // Detect map
                    m_substage = SS_DetectMap;
                    setState_runCommand("LDown,60");

                    m_timer.restart();
                    m_videoManager->setAreas({A_Map});
                }
                else
                {
                    // Walk up to Laventon
                    m_substage = SS_WalkToLaventon;
                    setState_runCommand("LUpRight,22");
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
            if (m_timer.elapsed() > 10000)
            {
                incrementStat(m_statError);
                setState_error("Unable to detect map for too long...");
            }
            else if (checkBrightnessMeanTarget(A_Map.m_rect, C_Color_Map, 240))
            {
                emit printLog("Map detected, checking each area for outbreaks...");

                // Goto Alabaster Icelands on map
                m_substage = SS_GotoMap;
                setState_runCommand(QString(m_firstCheck ? "LUpRight,30," : "") + "DLeft,1,Nothing,1,DLeft,1,Nothing,1,DLeft,1,Nothing,21");

                m_firstCheck = false;
                m_areaType = PLAAT_AlabasterIcelands;
                m_readyNextCheck = true;

                m_videoManager->clearAreas();
            }
            else
            {
                setState_runCommand("A,1,Nothing,20");
            }
        }
        break;
    }
    case SS_GotoMap:
    {
        if (state == S_CommandFinished || state == S_OCRReady)
        {
            if (state == S_OCRReady)
            {
                // Check if OCR text matches with list
                QString result = matchStringDatabase(m_allOutbreakEntries);
                if (!result.isEmpty() && m_outbreakPokemonList.contains(result))
                {
                    emit printLog("Mass Outbreak for \"" + result + "\" is found in " + PokemonDatabase::PLAAreaTypeToString(m_areaType) + "!", LOG_SUCCESS);
                    setState_completed();
                    break;
                }

                emit printLog ("No matching outbreak found in " + PokemonDatabase::PLAAreaTypeToString(m_areaType));
                if (m_areaType == PLAAT_ObsidianFieldlands)
                {
                    emit printLog("No desired outbreak found, entering Obsidian Fieldlands...", LOG_WARNING);

                    // No matching outbreak, go to Obsidian Fieldlands
                    m_substage = SS_EnterObsidian;
                    setState_runCommand("ASpam,80");

                    m_videoManager->clearAreas();
                    break;
                }
                else
                {
                    // Goto next area
                    m_areaType = (PLAAreaType)((int)m_areaType - 1);
                }
            }

            if (m_readyNextCheck)
            {
                // Continue next state so we can send command
                setState_ocrRequest(A_Text.m_rect, C_Color_Text);
                runNextStateContinue();

                m_videoManager->setAreas({A_Text});
            }
            else
            {
                // Move to next area command or OCR is finished
                m_readyNextCheck = true;
            }
        }
        else if (state == S_OCRRequested)
        {
            // Immediately goto next area while we wait for OCR result to save time
            incrementStat(m_statChecks);
            setState_runCommand(m_areaType == PLAAT_ObsidianFieldlands ? "Nothing,1" : "DLeft,1,Nothing,21");
            m_readyNextCheck = false;
        }
    }
    }

    SmartProgramBase::runNextState();
}
