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
        emit printLog("-----------Started-----------");
        m_runNextState = true;
        m_runStateTimer.start();
        m_parameters.vlcWrapper->clearCaptures();
        return true;
    }

    // S_Error
    emit printLog(m_errorMsg, QColor(255,0,0));
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
            emit printLog("Error ouccured cpaturing frame (" + errorString + ")", QColor(255,0,0));
            stop();
        }
    }
}

void SmartProgramBase::init()
{
    reset();

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
    return checkColorMatch(testColor, targetColor, threshold);
}

QColor SmartProgramBase::getAverageColor(QRect rectPos)
{
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

    //qDebug() << "Average color: " << "(" + QString::number(testColor.red()) + "," + QString::number(testColor.green()) + "," + QString::number(testColor.blue()) + ")";
    return checkColorMatch(avgColor, targetColor, threshold);
}

double SmartProgramBase::checkBrightnessMean(QRect rectPos, HSVRange hsvRange)
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
    qDebug() << "Brightness mean =" << mean;
    return mean;
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
        emit printLog(m_errorMsg, QColor(255,0,0));
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
        if (m_customCommand.isEmpty())
        {
            qDebug() << m_commands[m_commandIndex];
            emit runSequence(m_commands[m_commandIndex]);
        }
        else
        {
            qDebug() << m_customCommand;
            emit runSequence(m_customCommand);
        }
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
