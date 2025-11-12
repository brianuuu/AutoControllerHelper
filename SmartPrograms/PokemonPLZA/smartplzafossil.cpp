#include "smartplzafossil.h"

SmartPLZAFossil::SmartPLZAFossil
(
    Settings settings,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_programSettings(settings)
{
    init();
}

void SmartPLZAFossil::init()
{
    SmartProgramBase::init();
    inializeCommands(C_COUNT);
}

void SmartPLZAFossil::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
    setImageMatchFromResource("PLZA_Alpha", m_imageMatch_Alpha);
    setImageMatchFromResource("PLZA_Shiny", m_imageMatch_Shiny);

    m_boxIndex = 0;
    m_fossilCount = 0;
}

void SmartPLZAFossil::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        initStat(m_statFossils, "Fossils");
        initStat(m_statFound, "Found");
        initStat(m_statError, "Errors");

        m_substage = SS_GetFossil;
        setState_runCommand("Nothing,5");
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
                m_elapsedTimer.restart();
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
            m_elapsedTimer.restart();
            setState_frameAnalyzeRequest();
        }
        else if (state == S_CaptureReady)
        {
            if (m_elapsedTimer.elapsed() > 30000)
            {
                incrementStat(m_statError);
                emit printLog("Unable to detect title screen or game starting for too long, the game might have crashed, restarting...", LOG_ERROR);
                runRestartCommand();
            }
            else if (!checkAverageColorMatch(A_Title.m_rect, QColor(0,0,0)))
            {
                if (m_substage == SS_Title)
                {
                    emit printLog("Title detected!");
                    setState_runCommand("ASpam,20,Nothing,30");
                    m_substage = SS_GameStart;
                }
                else
                {
                    m_boxIndex = 0;
                    m_fossilCount = 0;

                    m_substage = SS_GetFossil;
                    setState_runCommand("Nothing,20");

                    m_videoManager->clearCaptures();
                }
            }
            else
            {
                setState_frameAnalyzeRequest();
            }
        }
        break;
    }
    case SS_GetFossil:
    {
        if (state == S_CommandFinished)
        {
            if (m_fossilCount >= m_programSettings.m_count)
            {
                emit printLog("Opening Boxes...");
                m_substage = SS_ScrollBoxStart;
                setState_runCommand(C_ToBox);
            }
            else
            {
                incrementStat(m_statFossils);
                m_fossilCount++;
                emit printLog("Collecting fossil #" + QString::number(m_statFossils.first));

                QString command = "A,20,Nothing,1,Loop,4,";
                if (m_programSettings.m_index > 0)
                {
                    command += "LDown,1,Nothing,1,Loop," + QString::number(m_programSettings.m_index) + ",";
                }
                command += "A,1,BSpam,270";
                setState_runCommand(command);
            }
        }
        break;
    }
    case SS_ScrollBoxStart:
    {
        if (state == S_CommandFinished)
        {
            emit printLog("Checking Pokemon in Box " + QString::number(m_boxIndex + 1));
            m_substage = SS_ScrollBox;
            setState_runCommand(C_ScrollBox, true);
            m_videoManager->setAreas({A_Detect});
        }
        break;
    }
    case SS_ScrollBox:
    {
        if (state == S_CommandFinished)
        {
            if (m_boxIndex < m_programSettings.m_count / 30)
            {
                m_boxIndex++;
                m_substage = SS_ScrollBoxStart;
                setState_runCommand("R,1,Nothing,20", true);
            }
            else
            {
                emit printLog("No target Pokemon found, restarting game...", LOG_WARNING);
                runRestartCommand();
            }
        }
        else if (state == S_CaptureReady)
        {
            bool found = false;
            switch (m_programSettings.m_fossilType)
            {
            case FossilType::FT_AlphaOnly:
            {
                found = detect(A_Detect, m_imageMatch_Shiny, C_Color_Shiny);
                break;
            }
            case FossilType::FT_ShinyOnly:
            {
                found = detect(A_Detect, m_imageMatch_Alpha, C_Color_Alpha);
                break;
            }
            case FossilType::FT_ShinyOrAlpha:
            {
                found = detect(A_Detect, m_imageMatch_Shiny, C_Color_Shiny)
                     || detect(A_Detect, m_imageMatch_Alpha, C_Color_Alpha);
                break;
            }
            case FossilType::FT_ShinyAndAlpha:
            {
                found = detect(A_Detect, m_imageMatch_Shiny, C_Color_Shiny)
                     && detect(A_Detect, m_imageMatch_Alpha, C_Color_Alpha);
                break;
            }
            }

            if (found)
            {
                incrementStat(m_statFound);
                emit printLog("Target Found!", LOG_SUCCESS);
                m_substage = SS_Finish;
                setState_runCommand("Home,1");
            }
            else
            {
                setState_frameAnalyzeRequest();
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

void SmartPLZAFossil::runRestartCommand()
{
    m_substage = SS_Restart;
    setState_runCommand(C_Restart);
    m_videoManager->clearCaptures();
}

bool SmartPLZAFossil::detect(const CaptureArea &area, const QImage &image, const HSVRange &hsvRange)
{
    for (int x = 0; x <= area.m_rect.width() - image.width(); x++)
    {
        QRect cropRect(area.m_rect.left() + x, area.m_rect.top(), image.width(), image.height());
        if (getImageMatch(cropRect, hsvRange, image) > 0.5)
        {
            return true;
        }
    }

    return false;
}
