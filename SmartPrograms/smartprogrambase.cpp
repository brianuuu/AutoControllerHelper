#include "smartprogrambase.h"

#include "autocontrollerwindow.h"

SmartProgramBase::SmartProgramBase(SmartProgramParameter parameter)
    : QWidget(parameter.parent)
    , m_parameters(parameter)
{
    init();
}

bool SmartProgramBase::run()
{
    if (m_state == S_NotStarted)
    {
        if (m_parameters.settings->isLogAutosave())
        {
            m_logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + getProgramInternalName() + ".log";
        }

        emit printLog("-----------Started-----------");
        m_runNextState = true;
        m_runStateTimer.start();
        m_parameters.vlcWrapper->clearCaptures();
        return true;
    }

    // S_Error
    emit printLog(m_errorMsg, LOG_ERROR);
    return false;
}

void SmartProgramBase::stop()
{
    m_state = S_NotStarted;
    m_runNextState = false;
    m_runStateTimer.stop();
    m_runStateDelayTimer.stop();
    m_parameters.vlcWrapper->clearCaptures();
}

void SmartProgramBase::commandFinished()
{
    if (m_state == S_CommandRunning)
    {
        qDebug() << "command finish";
        m_state = S_CommandFinished;
        m_runNextState = true;
    }
}

void SmartProgramBase::imageReady(int id, const QImage &preview)
{
    Q_UNUSED(id)
    if (m_state == S_CaptureRequested)
    {
        QSize size(1280,720);
        m_capture = (preview.size() == size) ? preview : preview.scaled(size);

        m_state = S_CaptureReady;
        m_runNextState = true;
    }
    else if (m_state == S_TakeScreenshot)
    {
        m_state = S_TakeScreenshotFinished;
        m_runNextState = true;
    }
}

void SmartProgramBase::imageError(int id, QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id)
    if (m_state == S_CaptureRequested || m_state == S_TakeScreenshot)
    {
        if (error == QCameraImageCapture::NoError || error == QCameraImageCapture::NotReadyError)
        {
            // nothing
        }
        else
        {
            emit printLog("Error ouccured cpaturing frame (" + errorString + ")", LOG_ERROR);
            stop();
        }
    }
}

void SmartProgramBase::init()
{
    reset();

    m_logFileName.clear();
    m_errorMsg = "An error has occured";
    m_commands.clear();

    /*
    Q_ASSERT(m_cameraCapture && m_cameraView);
    connect(m_cameraCapture, &QCameraImageCapture::imageCaptured, this, &SmartProgramBase::imageReady);
    connect(m_cameraCapture, SIGNAL(error(int,QCameraImageCapture::Error,const QString&)), this, SLOT(imageError(int,QCameraImageCapture::Error,const QString&)));
    */

    m_runNextState = false;
    connect(&m_runStateTimer, &QTimer::timeout, this, &SmartProgramBase::runStateLoop);
    m_runStateDelayTimer.setSingleShot(true);
    connect(&m_runStateDelayTimer, &QTimer::timeout, this, [&](){m_runNextState = true;});

    // ---------Child class should init their variables AFTER this (commands)---------
}

void SmartProgramBase::reset()
{
    m_state = S_NotStarted;
    m_commandIndex = -1;
    m_customCommand = "";

    // ---------Child class should reset their variables AFTER this---------
}

bool SmartProgramBase::checkColorMatch(QColor testColor, QColor targetColor, int threshold)
{
    double r = targetColor.red() - testColor.red();
    double g = targetColor.green() - testColor.green();
    double b = targetColor.blue() - testColor.blue();
    return r*r + g*g + b*b < threshold * threshold;
}

bool SmartProgramBase::checkColorMatchHSV(QColor testColor, HSVRange hsvRange)
{
    testColor = testColor.toHsv();

    // Test value and saturation first
    bool matched = testColor.value() >= hsvRange.min().value() && testColor.value() <= hsvRange.max().value()
                && testColor.hsvSaturation() >= hsvRange.min().hsvSaturation() && testColor.hsvSaturation() <= hsvRange.max().hsvSaturation();

    // For achromatic colors it should be filltered in saturation and value
    if (matched && testColor.hsvHue() != -1)
    {
        int h = testColor.hsvHue();
        int h0 = hsvRange.min().hsvHue();
        int h1 = hsvRange.max().hsvHue();

        if (h0 > h1)
        {
            // 0-----------------359
            //     ^max     ^min
            //    <---        --->
            matched &= (h >= h0 || h <= h1);
        }
        else
        {
            // 0-----------------359
            //     ^max     ^min
            //       ---> <---
            matched &= (h >= h0 && h <= h1);
        }
    }

    return matched;
}

bool SmartProgramBase::checkPixelColorMatch(QPoint pixelPos, QColor targetColor, int threshold)
{
    // m_frameAnalyze must be ready before calling this!

    QColor testColor = m_capture.pixelColor(pixelPos);
    bool success = checkColorMatch(testColor, targetColor, threshold);

    if (m_parameters.settings->isLogDebugColor())
    {
        QString logStr = "Pixel(" + QString::number(testColor.red()) + "," + QString::number(testColor.green()) + "," + QString::number(testColor.blue()) + ")";
        logStr += "~= Target(" + QString::number(targetColor.red()) + "," + QString::number(targetColor.green()) + "," + QString::number(targetColor.blue()) + ") = ";
        logStr += success ? "TRUE" : "FALSE";

        emit printLog(logStr, success ? LOG_SUCCESS : LOG_ERROR);
    }

    return success;
}

QColor SmartProgramBase::getAverageColor(QRect rectPos)
{
    // m_frameAnalyze must be ready before calling this!

    QImage cropped = m_capture.copy(rectPos);
    qreal r = 0;
    qreal g = 0;
    qreal b = 0;
    for (int y = 0; y < cropped.height(); y++)
    {
        for (int x = 0; x < cropped.width(); x++)
        {
            QColor color = cropped.pixelColor(x,y);
            r += color.redF();
            g += color.greenF();
            b += color.blueF();
        }
    }

    qreal pixelCount = cropped.height() * cropped.width();
    r /= pixelCount;
    g /= pixelCount;
    b /= pixelCount;
    QColor testColor;
    testColor.setRgbF(r,g,b);

    return testColor;
}

bool SmartProgramBase::checkAverageColorMatch(QRect rectPos, QColor targetColor, int threshold)
{
    // m_frameAnalyze must be ready before calling this!

    QColor avgColor = getAverageColor(rectPos);
    bool success = checkColorMatch(avgColor, targetColor, threshold);

    if (m_parameters.settings->isLogDebugColor())
    {
        QString logStr = "Average(" + QString::number(avgColor.red()) + "," + QString::number(avgColor.green()) + "," + QString::number(avgColor.blue()) + ")";
        logStr += ", Target(" + QString::number(targetColor.red()) + "," + QString::number(targetColor.green()) + "," + QString::number(targetColor.blue()) + ") = ";
        logStr += success ? "TRUE" : "FALSE";

        emit printLog(logStr, success ? LOG_SUCCESS : LOG_ERROR);
    }

    return success;
}

double SmartProgramBase::getBrightnessMean(QRect rectPos, HSVRange hsvRange)
{
    // m_frameAnalyze must be ready before calling this!

    QImage cropped = m_capture.copy(rectPos);
    if (m_parameters.preview)
    {
        m_parameters.preview->clear();
        m_parameters.preview->setSceneRect(cropped.rect());
        m_parameters.preview->addPixmap(QPixmap::fromImage(cropped));
    }

    QImage masked = QImage(cropped.size(), QImage::Format_Mono);
    masked.setColorTable({0xFF000000,0xFFFFFFFF});

    double mean = 0;
    for (int y = 0; y < cropped.height(); y++)
    {
        for (int x = 0; x < cropped.width(); x++)
        {
            // Mask the target color
            bool matched = checkColorMatchHSV(cropped.pixelColor(x,y), hsvRange);
            if (matched)
            {
                mean += 255;
            }

            if (m_parameters.previewMasked)
            {
                masked.setPixel(x, y, matched ? 1 : 0);
            }
        }
    }

    if (m_parameters.previewMasked)
    {
        m_parameters.previewMasked->clear();
        m_parameters.previewMasked->setSceneRect(masked.rect());
        m_parameters.previewMasked->addPixmap(QPixmap::fromImage(masked));
    }

    // Get average value of brightness
    mean /= (cropped.height() * cropped.width());
    return mean;
}

bool SmartProgramBase::checkBrightnessMeanTarget(QRect rectPos, SmartProgramBase::HSVRange hsvRange, double target)
{
    // m_frameAnalyze must be ready before calling this!

    double mean = getBrightnessMean(rectPos, hsvRange);
    bool success = mean > target;

    QString logStr = "Mean (" + QString::number(mean) + ") > target (" + QString::number(target) + ") = ";
    logStr += success ? "TRUE" : "FALSE";
    if (m_parameters.settings->isLogDebugColor())
    {
        emit printLog(logStr, success ? LOG_SUCCESS : LOG_ERROR);
    }
    else
    {
        qDebug() << logStr;
    }

    return success;
}

bool SmartProgramBase::inializeCommands(int size)
{
    m_commands.clear();
    if (!m_parameters.smartProgramCommands)
    {
        m_errorMsg = "Commands .xml file missing!";
        m_state = S_Error;
        return false;
    }

    bool valid = false;
    bool found = false;
    QDomNodeList programList = m_parameters.smartProgramCommands->firstChildElement().childNodes();
    for (int i = 0; i < programList.count(); i++)
    {
        QDomElement programElement = programList.at(i).toElement();
        QString const programName = programElement.tagName();

        if (programName == this->getProgramInternalName())
        {
            found = true;

            QDomNodeList commandList = programElement.childNodes();
            if (size > commandList.count())
            {
                m_errorMsg = "Program require " + QString::number(size) + " commands but only " + QString::number(commandList.count()) + " found";
                break;
            }

            for (int j = 0; j < commandList.count(); j++)
            {
                QDomElement commandElement = commandList.at(j).toElement();
                QString const commandString = commandElement.text();
                m_commands.insert(j, commandString);
            }

            valid = true;
            break;
        }
    }

    if (!found)
    {
        m_errorMsg = "Commands for this program is not found in .xml";
    }

    if (!valid)
    {
        m_state = S_Error;
    }

    return valid;
}

void SmartProgramBase::setState_runCommand(Command commandIndex)
{
    m_customCommand.clear();
    if (m_commands.contains(commandIndex))
    {
        m_commandIndex = commandIndex;
        m_state = S_CommandRunning;
    }
    else
    {
        qDebug() << "Invalid command index";
        m_state = S_Error;
    }
}

void SmartProgramBase::setState_runCommand(const QString &customCommand)
{
    if (!customCommand.isEmpty())
    {
        m_customCommand = customCommand;
        m_state = S_CommandRunning;
    }
    else
    {
        qDebug() << "Invalid custom command";
        m_state = S_Error;
    }
}

void SmartProgramBase::setState_frameAnalyzeRequest()
{
    m_state = S_CaptureRequested;
}

void SmartProgramBase::runStateLoop()
{
    if (m_runNextState)
    {
        m_runNextState = false;
        runNextState();
    }
}

void SmartProgramBase::runNextStateDelay(int milliseconds)
{
    m_runStateDelayTimer.start(milliseconds);
}

void SmartProgramBase::runNextState()
{
    // ---------Child class should set the state BEFORE calling this---------

    switch (m_state)
    {
    case S_Error:
    case S_NotStarted:
    {
        stop();
        emit printLog(m_errorMsg, LOG_ERROR);
        emit completed();
        break;
    }
    case S_Completed:
    {
        stop();
        emit printLog("-----------Finished-----------");
        emit completed();
        break;
    }
    case S_CommandRunning:
    {
        // If sequence failed to run, the client is responsible to stop this smart program
        // otherwise it will stuck in this state forever
        QString command;
        if (m_customCommand.isEmpty())
        {
            command = m_commands[m_commandIndex];
        }
        else
        {
            command = m_customCommand;
        }

        QString const logStr = "Running command: [" + command + "]";
        if (m_parameters.settings->isLogDebugCommand())
        {
            emit printLog(logStr);
        }
        else
        {
            qDebug() << logStr;
        }

        emit runSequence(command);
        break;
    }
    case S_TakeScreenshot:
    {
        QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + m_screenshotName + ".jpg";
        emit printLog("Saving screenshot: " + nameWithTime);
        m_parameters.vlcWrapper->takeSnapshot(SCREENSHOT_PATH + nameWithTime);

        m_state = S_TakeScreenshotFinished;
        m_runNextState = true;

        /*
        if (m_cameraCapture->isReadyForCapture())
        {
            if (!QDir(SCREENSHOT_PATH).exists())
            {
                QDir().mkdir(SCREENSHOT_PATH);
            }

            QString nameWithTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + m_screenshotName + ".jpg";
            emit printLog("Saving screenshot: " + nameWithTime);
            m_cameraCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
            m_cameraCapture->capture(SCREENSHOT_PATH + nameWithTime);
        }
        else
        {
            // Try again later
            m_runNextState = true;
        }
        */
        break;
    }
    case S_CaptureRequested:
    {
        m_parameters.vlcWrapper->getFrame(m_capture);
        m_state = S_CaptureReady;
        m_runNextState = true;

        /*
        if (m_cameraCapture->isReadyForCapture())
        {
            m_cameraCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
            m_cameraCapture->capture();
        }
        else
        {
            // Try again later
            m_runNextState = true;
        }
        */
        break;
    }
    default:
    {
        break;
    }
    }
}

void SmartProgramBase::initStat(Stat &stat, QString const &key)
{
    if (key.isEmpty()) return;

    QSettings stats(SMART_STATS_INI, QSettings::IniFormat, this);
    stats.beginGroup(getProgramInternalName());

    // Set key and grab value from ini, resave it ini file doesn't exist
    stat.second = key;
    stat.first = stats.value(key, 0).toInt();
    stats.setValue(stat.second, stat.first);

    updateStats();
}

void SmartProgramBase::incrementStat(Stat &stat, int addCount)
{
    if (stat.second.isEmpty())
    {
        emit printLog("Uninitialzed stat key!", LOG_ERROR);
        return;
    }

    QSettings stats(SMART_STATS_INI, QSettings::IniFormat, this);
    stats.beginGroup(getProgramInternalName());

    // Grab value from ini, increment and save
    stat.first = stats.value(stat.second).toInt();
    stat.first += addCount;
    stats.setValue(stat.second, stat.first);

    updateStats();
}

void SmartProgramBase::updateStats()
{
    if (!m_parameters.statsLabel) return;

    QSettings stats(SMART_STATS_INI, QSettings::IniFormat, this);
    stats.beginGroup(getProgramInternalName());

    // Update label
    QStringList list = stats.allKeys();
    QString statsStr;
    if (list.isEmpty())
    {
        statsStr = "N/A";
        emit enableResetStats(false);
    }
    else
    {
        emit enableResetStats(true);
        for (int i = 0; i < list.size(); i++)
        {
            QString const& key = list[i];
            if (i != 0)
            {
                statsStr += ", ";
            }

            int count = stats.value(key, 0).toInt();
            statsStr += key + ": " + QString::number(count);

            // Write to individual files for each stat
            if (m_parameters.settings->isStreamCounterEnabled())
            {
                QFile file(STREAM_COUNTER_PATH + key + ".txt");
                if(file.open(QIODevice::WriteOnly))
                {
                    QTextStream stream(&file);
                    stream << key + ": " << count;
                    file.close();
                }
            }
        }
    }
    m_parameters.statsLabel->setText(statsStr);
}
