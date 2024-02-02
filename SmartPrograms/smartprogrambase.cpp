#include "smartprogrambase.h"

#include "autocontrollerwindow.h"

SmartProgramBase::SmartProgramBase(SmartProgramParameter parameter)
    : QWidget(parameter.parent)
    , m_ocrHSVRange(0,0,0,0,0,0)
{
    m_audioManager      = parameter.vlcWrapper->getAudioManager();
    m_videoManager      = parameter.vlcWrapper->getVideoManager();
    m_discordSettings   = parameter.discordSettings;
    m_settings          = parameter.settings;
    m_statsLabel        = parameter.statsLabel;
    m_preview           = parameter.preview;
    m_previewMasked     = parameter.previewMasked;
    m_startDateTime     = QDateTime::currentDateTime();

    init();
}

bool SmartProgramBase::run()
{
    if (m_state == S_NotStarted)
    {
        if (getProgramExportLog(getProgramEnum()) && m_settings->isLogAutosave())
        {
            m_logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + "_" + getProgramInternalName() + ".log";
        }

        emit printLog("-----------Started-----------");
        m_runNextState = true;
        m_runStateTimer.start();

        m_audioManager->stopDetection();
        m_videoManager->clearCaptures();

        // discord
        m_hadDiscordMessage = false;
        QTimer::singleShot(1000 * 60 * 60 + 1000, this, &SmartProgramBase::sendRegularDiscordMessage);
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

    m_audioManager->stopDetection();
    m_videoManager->clearCaptures();
}

void SmartProgramBase::commandFinished()
{
    //if (m_state == S_CommandRunning)
    {
        qDebug() << "command finish";
        m_state = S_CommandFinished;
        m_runNextState = true;
    }
}

void SmartProgramBase::init()
{
    reset();

    m_logFileName.clear();
    m_errorMsg = "An error has occured";
    m_commands.clear();

    m_runNextState = false;
    connect(&m_runStateTimer, &QTimer::timeout, this, &SmartProgramBase::runStateLoop);
    m_runStateDelayTimer.setSingleShot(true);
    connect(&m_runStateDelayTimer, &QTimer::timeout, this, [&](){m_runNextState = true;});

    // OCR
    m_ocrRect = QRect();
    m_ocrCustomImage = Q_NULLPTR;
    connect(&m_ocrProcess, &QProcess::errorOccurred, this, &SmartProgramBase::on_OCRErrorOccurred);
    connect(&m_ocrProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_OCRFinished()));

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
    Q_ASSERT(m_state == S_CaptureReady);

    QColor testColor = m_capture.pixelColor(pixelPos);
    bool success = checkColorMatch(testColor, targetColor, threshold);

    if (m_settings->isLogDebugColor())
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
    Q_ASSERT(m_state == S_CaptureReady);

    QImage cropped = m_capture.copy(rectPos);
    qreal r = 0;
    qreal g = 0;
    qreal b = 0;
    for (int y = 0; y < cropped.height(); y++)
    {
        QRgb *rowData = (QRgb*)cropped.scanLine(y);
        for (int x = 0; x < cropped.width(); x++)
        {
            QColor color = QColor::fromRgb(rowData[x]);
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
    Q_ASSERT(m_state == S_CaptureReady);

    QColor avgColor = getAverageColor(rectPos);
    bool success = checkColorMatch(avgColor, targetColor, threshold);

    if (m_settings->isLogDebugColor())
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
    Q_ASSERT(m_state == S_CaptureReady);

    QImage cropped = m_capture.copy(rectPos);
    if (m_preview)
    {
        m_preview->clear();
        m_preview->setSceneRect(cropped.rect());
        m_preview->addPixmap(QPixmap::fromImage(cropped));
    }

    QImage masked = QImage(cropped.size(), QImage::Format_MonoLSB);
    masked.setColorTable({0xFF000000,0xFFFFFFFF});

    double mean = 0;

    for (int y = 0; y < cropped.height(); y++)
    {
        QRgb *rowData = (QRgb*)cropped.scanLine(y);
        uint8_t *rowMaskedData = (uint8_t*)masked.scanLine(y);
        for (int x = 0; x < cropped.width(); x++)
        {
            // Mask the target color
            bool matched = checkColorMatchHSV(QColor::fromRgb(rowData[x]), hsvRange);
            if (matched)
            {
                mean += 255;
            }

            if (m_previewMasked)
            {
                matched ? SET_BIT(rowMaskedData[x / 8], x % 8) : CLEAR_BIT(rowMaskedData[x / 8], x % 8);
            }
        }
    }

    if (m_previewMasked)
    {
        m_previewMasked->clear();
        m_previewMasked->setSceneRect(masked.rect());
        m_previewMasked->addPixmap(QPixmap::fromImage(masked));
    }

    // Get average value of brightness
    mean /= (cropped.height() * cropped.width());
    return mean;
}

bool SmartProgramBase::checkBrightnessMeanTarget(QRect rectPos, HSVRange hsvRange, double target)
{
    // m_frameAnalyze must be ready before calling this!
    Q_ASSERT(m_state == S_CaptureReady);

    double mean = getBrightnessMean(rectPos, hsvRange);
    bool success = mean > target;

    QString logStr = "Mean (" + QString::number(mean) + ") > target (" + QString::number(target) + ") = ";
    logStr += success ? "TRUE" : "FALSE";
    if (m_settings->isLogDebugColor())
    {
        emit printLog(logStr, success ? LOG_SUCCESS : LOG_ERROR);
    }
    else
    {
        qDebug() << logStr;
    }

    return success;
}

QImage SmartProgramBase::getMonochromeImage(QRect rectPos, HSVRange hsvRange, bool whiteIsOne)
{
    QImage cropped = m_capture.copy(rectPos);
    QImage masked = QImage(cropped.size(), QImage::Format_MonoLSB);
    masked.setColorTable({0xFF000000,0xFFFFFFFF});

    for (int y = 0; y < cropped.height(); y++)
    {
        QRgb *rowData = (QRgb*)cropped.scanLine(y);
        uint8_t *rowMaskedData = (uint8_t*)masked.scanLine(y);
        for (int x = 0; x < cropped.width(); x++)
        {
            bool matched = checkColorMatchHSV(QColor::fromRgb(rowData[x]), hsvRange);
            if (whiteIsOne ^ matched)
            {
                CLEAR_BIT(rowMaskedData[x / 8], x % 8);
            }
            else
            {
                SET_BIT(rowMaskedData[x / 8], x % 8);
            }
        }
    }

    return masked;
}

bool SmartProgramBase::setImageMatchFromResource(const QString &name, QImage &image)
{
    image = QImage(":/resources/ImageMatch/" + name + ".bmp").convertToFormat(QImage::Format_MonoLSB, Qt::MonoOnly);

    if (image.isNull())
    {
        setState_error("Unable to find image match resource '" + name + "'");
        return false;
    }
    return true;
}

double SmartProgramBase::getImageSimilarRatio(const QImage &query, const QImage &database)
{
    double hitCount = 0;
    double missCount = 0;
    double count = 0;
    for (int y = 0; y < query.height(); y++)
    {
        uint8_t *rowData = (uint8_t*)query.scanLine(y);
        uint8_t *rowData2 = (uint8_t*)database.scanLine(y);
        for (int x = 0; x < query.width(); x++)
        {
            // Only counts query's white pixels
            // Remember, bit not set = white
            bool pixelNotSet = !CHECK_BIT(rowData[x/8], x%8);
            if (pixelNotSet)
            {
                count++;
                if (!CHECK_BIT(rowData2[x/8], x%8))
                {
                    hitCount++;
                }
                else
                {
                    missCount++;
                }
            }
        }
    }

    if (count <= 0)
    {
        return -1;
    }

    // Can be negative if we have >50% miss
    return (hitCount - missCount) / count;
}

double SmartProgramBase::getImageMatch(QRect rectPos, HSVRange hsvRange, const QImage &testImage)
{
    // m_frameAnalyze must be ready before calling this!
    Q_ASSERT(m_state == S_CaptureReady);

    if (rectPos.width() != testImage.width() || rectPos.height() != testImage.height())
    {
        emit printLog("Image size not matching capture area dimension", LOG_ERROR);
        return 0;
    }

    // Get filtered image (black is 1, white is 0)
    QImage masked = getMonochromeImage(rectPos, hsvRange, false);

    // Similar (query -> database)
    double sqd = getImageSimilarRatio(masked, testImage);
    if (sqd < 0) return 0;

    // Similar (database -> query)
    double sdq = getImageSimilarRatio(testImage, masked);
    if (sdq < 0) return 0;

    qDebug() << "sqd =" << sqd << ", sdq =" << sdq;
    return (sqd + sdq) / 2;
}

bool SmartProgramBase::checkImageMatchTarget(QRect rectPos, HSVRange hsvRange, const QImage &testImage, double target, QPoint* offset)
{
    // m_frameAnalyze must be ready before calling this!
    Q_ASSERT(m_state == S_CaptureReady);

    if (rectPos.width() < testImage.width() || rectPos.height() < testImage.height())
    {
        emit printLog("Image size is larger than capture area dimension", LOG_ERROR);
        return false;
    }

    if (testImage.format() != QImage::Format_MonoLSB)
    {
        emit printLog("Test image format is not monochrome", LOG_ERROR);
        return false;
    }

    bool success = false;
    double maxRatio = 0;
    QPoint maxRatioOffset(0,0);
    for (int y = 0; y <= rectPos.height() - testImage.height(); y++)
    {
        for (int x = 0; x <= rectPos.width() - testImage.width(); x++)
        {
            QRect cropRect(rectPos.left() + x, rectPos.top() + y, testImage.width(), testImage.height());
            double ratio = getImageMatch(cropRect, hsvRange, testImage);
            if (ratio > maxRatio)
            {
                maxRatio = ratio;
                maxRatioOffset = QPoint(x,y);
            }

            // Early out if we already found match
            if (maxRatio > target)
            {
                success = true;
                break;
            }
        }
    }

    QString posStr = success ? " at offset {" + QString::number(maxRatioOffset.x()) + "," + QString::number(maxRatioOffset.y()) + "}" : "";
    QString logStr = "Image Match Ratio (" + QString::number(maxRatio) + ") > target (" + QString::number(target) + ")" + posStr + " = ";
    logStr += success ? "TRUE" : "FALSE";
    if (m_settings->isLogDebugColor())
    {
        emit printLog(logStr, success ? LOG_SUCCESS : LOG_ERROR);
    }
    else
    {
        qDebug() << logStr;
    }

    if (offset)
    {
        *offset = maxRatioOffset;
    }
    return success;
}

bool SmartProgramBase::validateCommand(const QString &commands, QString &errorMsg)
{
    static QSet<QString> const availableCommands =
    {
        "nothing",
        "a",
        "b",
        "x",
        "y",
        "l",
        "r",
        "zl",
        "zr",
        "plus",
        "minus",
        "home",
        "capture",
        "lclick",
        "lup",
        "ldown",
        "lleft",
        "lright",
        "rclick",
        "rup",
        "rdown",
        "rleft",
        "rright",
        "dup",
        "ddown",
        "dleft",
        "dright",
        "triggers",
        "lupleft",
        "lupright",
        "ldownleft",
        "ldownright",
        "lupa",
        "ldowna",
        "lrighta",
        "drightr",
        "lupclick",
        "lleftb",
        "lrightb",
        "bxdup",
        "zrdup",
        "by",
        "zlbx",
        "zla",

        "aspam",
        "bspam",
        "loop",
    };

    bool valid = true;
    int count = 0;

    QStringList list = commands.split(',');
    if (list.isEmpty() || list.size() % 2 == 1)
    {
        errorMsg = "Invalid syntax, it should be \"COMMAND,DURATION,COMMAND,DURATION,...\"";
        valid = false;
    }
    else
    {
        bool isLoop = false;
        for (int i = 0; i < list.size(); i++)
        {
            QString const& str = list[i];
            if (i % 2 == 0)
            {
                QString commandLower = str.toLower();
                isLoop = commandLower == "loop";
                if (!availableCommands.contains(commandLower))
                {
                    errorMsg = "\"" + str + "\" is not a recognized command";
                    valid = false;
                    break;
                }
            }
            else
            {
                bool durationValid;
                int duration = str.toInt(&durationValid);
                if (!durationValid)
                {
                    errorMsg = "Duration is not an integer";
                    valid = false;
                    break;
                }
                else if (duration > 65534)
                {
                    errorMsg = "Duration cannot be larger than 65534";
                    valid = false;
                    break;
                }
                else if (duration < 0)
                {
                    errorMsg = "Duration cannot be negative";
                    valid = false;
                    break;
                }
                else if (!isLoop && duration == 0)
                {
                    errorMsg = "Only 'Loop' command can use duration 0, other commands have no effect";
                    valid = false;
                    break;
                }

                count++;
            }
        }
    }

    if (count > COMMAND_MAX)
    {
        valid = false;
        errorMsg = "Number of commands exceed maximum of " + QString::number(COMMAND_MAX);
    }

    return valid;
}

bool SmartProgramBase::inializeCommands(int size)
{
    m_commands.clear();

    // No command
    if (size <= 0)
    {
        return true;
    }

    QString const fileName = this->getProgramInternalName() + ".xml";
    if (!QFile::exists(SMART_COMMAND_PATH + fileName))
    {
        m_errorMsg = fileName + " file missing!";
        m_state = S_Error;
        return false;
    }

    QFile file(SMART_COMMAND_PATH + fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_errorMsg = "Fail to load " + fileName;
        m_state = S_Error;
        return false;
    }
    else
    {
        QDomDocument programCommands;
        programCommands.setContent(&file);
        file.close();

        QDomNodeList commandList = programCommands.firstChildElement().childNodes();
        if (size > commandList.count())
        {
            m_errorMsg = "Program require " + QString::number(size) + " commands but only " + QString::number(commandList.count()) + " found";
            m_state = S_Error;
            return false;
        }

        for (int j = 0; j < commandList.count(); j++)
        {
            QDomElement commandElement = commandList.at(j).toElement();
            QString const commandName = commandElement.tagName();
            QString const commandString = commandElement.text();

            QString error;
            if (!validateCommand(commandString, error))
            {
                m_errorMsg = fileName + " &#60;" + commandName + "&#62; error: " + error;
                m_state = S_Error;
                return false;
            }
            else
            {
                m_commands.insert(j, commandString);
            }
        }
    }

    return true;
}

void SmartProgramBase::setState_runCommand(Command commandIndex, bool requestFrameAnalyze)
{
    m_customCommand.clear();
    if (m_commands.contains(commandIndex))
    {
        m_commandIndex = commandIndex;
        m_state = requestFrameAnalyze ? S_CommandRunningCaptureRequested : S_CommandRunning;
    }
    else
    {
        setState_error("Invalid command index");
    }
}

void SmartProgramBase::setState_runCommand(const QString &customCommand, bool requestFrameAnalyze)
{
    if (!customCommand.isEmpty())
    {
        m_customCommand = customCommand;
        m_state = requestFrameAnalyze ? S_CommandRunningCaptureRequested : S_CommandRunning;
    }
    else
    {
        setState_error("Invalid custom command");
    }
}

void SmartProgramBase::setState_frameAnalyzeRequest()
{
    m_state = S_CaptureRequested;
}

void SmartProgramBase::setState_ocrRequest(QRect rect, HSVRange hsvRange)
{
    m_state = S_OCRRequested;
    m_ocrRect = rect;
    m_ocrHSVRange = hsvRange;
}

void SmartProgramBase::setState_ocrRequest(const QImage &image)
{
    m_state = S_OCRRequested;
    m_ocrCustomImage = new QImage(image);
}

void SmartProgramBase::runStateLoop()
{
    if (m_runNextState)
    {
        m_runNextState = false;
        runNextState();
    }
}

void SmartProgramBase::printLogExternal(const QString log, QColor color)
{
    emit printLog(log, color);
}

bool SmartProgramBase::startOCR(QRect rectPos, HSVRange hsvRange, bool isNumber)
{
    // Get filtered image, flip black/white since black text detects better
    if (m_ocrCustomImage)
    {
        m_ocrCustomImage->save(QString(RESOURCES_PATH) + "Tesseract/capture.png", "PNG");
        delete m_ocrCustomImage;
        m_ocrCustomImage = Q_NULLPTR;
    }
    else
    {
        QImage masked = getMonochromeImage(rectPos, hsvRange, false);
        masked.save(QString(RESOURCES_PATH) + "Tesseract/capture.png", "PNG");
    }

    // Check if .traineddata exist
    GameLanguage gameLanguage = isNumber ? GL_English : m_settings->getGameLanguage();
    if (!m_settings->ensureTrainedDataExist())
    {
        QString languageName = PokemonDatabase::getGameLanguageName(gameLanguage);
        setState_error("Language trained data for '" + languageName + "' for Tesseract is missing, please goto \"Resources/Tesseract\" folder and follow the instructions in README.md");

        m_runNextState = true;
        return false;
    }

    QString command = QString(RESOURCES_PATH) + "Tesseract/tesseract.exe ";
    command += "./capture.png ./output --tessdata-dir . ";
    command += "-l " + PokemonDatabase::getGameLanguagePrefix(gameLanguage);
    command += " --psm 7 --oem 2 -c tessedit_create_txt=1";
    m_ocrProcess.setWorkingDirectory(QString(RESOURCES_PATH) + "Tesseract/");
    m_ocrProcess.start(command);

    return true;
}

int SmartProgramBase::getLevenshteinDistance(const QString &a, const QString &b)
{
    QVector<int> v0(b.size() + 1);
    QVector<int> v1(b.size() + 1);

    for (int i = 0; i <= b.size(); i++)
    {
        v0[i] = i;
    }

    for (int i = 0; i < a.size(); i++)
    {
        v1[0] = i + 1;

        for (int j = 0; j < b.size(); j++)
        {
            int deletion = v0[j + 1] + 1;
            int insertion = v1[j] + 1;
            int substitution = v0[j];
            if (a[i] != b[j])
            {
                substitution += 1;
            }

            v1[j + 1] = qMin(deletion, qMin(insertion, substitution));
        }

        qSwap(v0, v1);
    }

    return v0[b.size()];
}

int SmartProgramBase::getLevenshteinDistanceSubString(const QString &longStr, const QString &shortStr)
{
    QVector<int> v0(shortStr.size() + 1);
    QVector<int> v1(shortStr.size() + 1);

    for (int i = 0; i <= shortStr.size(); i++)
    {
        v0[i] = i;
    }

    int min = shortStr.size();
    for (int i = 0; i < longStr.size(); i++)
    {
        v1[0] = 0;

        for (int j = 0; j < shortStr.size(); j++)
        {
            int deletion = v0[j + 1] + 1;
            int insertion = v1[j] + 1;
            int substitution = v0[j];
            if (longStr[i] != shortStr[j])
            {
                substitution += 1;
            }

            v1[j + 1] = qMin(deletion, qMin(insertion, substitution));
        }

        qSwap(v0, v1);
        min = qMin(min, v0[shortStr.size()]);
    }

    return min;
}

int SmartProgramBase::matchSubStrings(const QString &query, const QStringList &subStrings, int* o_dist)
{
    // Note: "query" and "subStrings" should be already normalized
    // This function finds the best match with in the entry,
    // though this doesn't matter much as any match in an entry always counts

    int minDist = INT_MAX;
    int minSubStringID = -1;
    for (int i = 0; i < subStrings.size(); i++)
    {
        QString const& subString = subStrings[i];

        // check for exact match
        if (query == subString)
        {
            minDist = 0;
            minSubStringID = i;
            break;
        }

        // Calculate Levenshtein Distance
        int dist = getLevenshteinDistance(query, subString);

        // Pretty naive way to filter, for modification less than database str/2
        if (dist < minDist && dist <= subString.size() / 2 )
        {
            minDist = dist;
            minSubStringID = i;
        }
    }

    if (minSubStringID >= 0 && o_dist)
    {
        *o_dist = minDist;
    }

    return minSubStringID;
}

QString SmartProgramBase::getOCRStringRaw()
{
    QString str;
    QFile output(QString(RESOURCES_PATH) + "Tesseract/output.txt");
    if (output.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        QTextStream in(&output);
        in.setCodec("UTF-8");
        str = in.readLine();
        output.close();
    }

    return str;
}

bool SmartProgramBase::getOCRNumber(int &number)
{
    QString queryRaw = getOCRStringRaw();

    // Nothing
    if (queryRaw.isEmpty())
    {
        qDebug() << "OCR did not find any number";
        return false;
    }

    QString numStr;
    bool hasDigit = false;
    for (QChar ch : queryRaw)
    {
        // in case it false detect as these...?
        if (ch == 'l' || ch == 'i' || ch == 't' || ch == 'F' || ch == 'f')
        {
            emit printLog(QString("Letter '") + ch + "' is converted to 1", LOG_WARNING);
            ch = '1';
        }

        if (ch.isDigit())
        {
            numStr += ch;
            hasDigit = true;
        }
    }

    if (!hasDigit)
    {
        emit printLog("OCR failed to read any number", LOG_ERROR);
        return false;
    }

    // This should be number but we check convertion just in case
    bool ok = false;
    number = numStr.toInt(&ok);

    if (ok)
    {
        qDebug() << "OCR returned number: " + numStr;
    }
    else
    {
        emit printLog("OCR failed to convert number", LOG_ERROR);
    }

    return ok;
}

QString SmartProgramBase::matchStringDatabase(const PokemonDatabase::OCREntries &entries)
{
    /* database structure:
     * {
     *      { entry1, { test, t35T, tosi } },
     *      { entry2, { Jeff } },
     *      { entry3, { ur mom } },
     *      ...
     * }
     */

    QString queryRaw = getOCRStringRaw();
    QString query = PokemonDatabase::normalizeString(queryRaw);

    // Nothing
    if (query.isEmpty())
    {
        qDebug() << "OCR returned empty string";
        return QString();
    }

    // Do comparison with each database string, find the best match entry
    int minDist = INT_MAX;
    QString minMatchedEntry;
    int minSubStringMatched = -1;
    for (auto iter = entries.begin(); iter != entries.end(); iter++)
    {
        int dist = 0;
        QStringList const& subStrings = iter.value();
        int subStringMatched = matchSubStrings(query, subStrings, &dist);
        if (subStringMatched >= 0 && subStringMatched < subStrings.size() && dist < minDist)
        {
            minDist = dist;
            minMatchedEntry = iter.key();
            minSubStringMatched = subStringMatched;
        }
    }

    if (!minMatchedEntry.isEmpty())
    {
        emit printLog("OCR text \"" + queryRaw + "\" has matched entry \"" + minMatchedEntry + "\" (LD = " + QString::number(minDist) + ")");
    }
    else
    {
        emit printLog("OCR text \"" + queryRaw + "\" found no matches");
    }

    return minMatchedEntry;
}

void SmartProgramBase::on_OCRErrorOccurred(QProcess::ProcessError error)
{
    setState_error("Unable to start text recognition, tesseract.exe might be missing.\nProcess exited with code: " + QString::number(error));
    m_runNextState = true;
}

void SmartProgramBase::on_OCRFinished()
{
    if (QFile::exists(QString(RESOURCES_PATH) + "Tesseract/output.txt"))
    {
        m_state = S_OCRReady;
    }
    else
    {
        setState_error("Expected tesseract output.txt not found");
    }

    m_runNextState = true;
}

void SmartProgramBase::sendRegularDiscordMessage()
{
    sendDiscordMessage("Program Status", false);
    QTimer::singleShot(1000 * 60 * 60, this, &SmartProgramBase::sendRegularDiscordMessage);
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

        // long running program
        if (m_hadDiscordMessage)
        {
            QImage frame;
            m_videoManager->getFrame(frame);
            sendDiscordMessage("Error Occured", false, LOG_ERROR, &frame);
        }

        break;
    }
    case S_Completed:
    {
        stop();
        emit printLog("-----------Finished-----------");
        emit completed();

        // long running program
        if (m_hadDiscordMessage)
        {
            sendDiscordMessage("Program Finished", false, LOG_SUCCESS);
        }
        break;
    }
    case S_CommandRunning:
    case S_CommandRunningCaptureRequested:
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
        if (m_settings->isLogDebugCommand())
        {
            emit printLog(logStr);
        }
        else
        {
            qDebug() << logStr;
        }

        emit runSequence(command);

        if (m_state != S_CommandRunningCaptureRequested)
        {
            break;
        }
        [[clang::fallthrough]];
    }
    case S_CaptureRequested:
    {
        m_videoManager->getFrame(m_capture);
        m_state = S_CaptureReady;
        m_runNextState = true;

        break;
    }
    case S_OCRRequested:
    {
        m_videoManager->getFrame(m_capture);
        if (!m_ocrRect.isNull() || m_ocrCustomImage)
        {
            startOCR(m_ocrRect, m_ocrHSVRange);
        }
        else
        {
            setState_error("Invalid OCR capture size");
            m_runNextState = true;
        }

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
    stat.first = qMax(stat.first, 0); // no negative, but something probably went wrong...?
    stats.setValue(stat.second, stat.first);

    updateStats();
}

void SmartProgramBase::updateStats()
{
    if (!m_statsLabel) return;

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
            if (m_settings->isStreamCounterEnabled())
            {
                QFile file(STREAM_COUNTER_PATH + key + ".txt");
                if(file.open(QIODevice::WriteOnly))
                {
                    QTextStream stream(&file);
                    if (!m_settings->isStreamCounterExcludePrefix())
                    {
                        stream << key + ": ";
                    }
                    stream << count;
                    file.close();
                }
            }
        }
    }
    m_statsLabel->setText(statsStr);
}

void SmartProgramBase::sendDiscordMessage(const QString &title, bool isMention, QColor color, const QImage *img, const QList<Discord::EmbedField> &fields)
{
    Discord::Embed embed = m_discordSettings->getEmbedTemplate(title);
    embed.setColor(color.rgb() & 0xFFFFFF);

    // add custom fields
    for (Discord::EmbedField const& field : fields)
    {
        embed.addField(field);
    }

    // append program stats
    SmartProgram sp = getProgramEnum();
    QString fieldMsg = getProgramGamePrefix(sp) + ": " + getProgramNameFromEnum(sp);

    qint64 mins = m_startDateTime.secsTo(QDateTime::currentDateTime()) / 60;
    qint64 hours = mins / 60;
    mins %= 60;
    fieldMsg += "\n Up Time: " + QString::number(hours) + " hours " + QString::number(mins) + " minutes";

    if (m_statsLabel->text() != "N/A")
    {
        fieldMsg += "\n" + m_statsLabel->text();
    }
    embed.addField(Discord::EmbedField("Smart Program Stats", fieldMsg, false));

    // send message
    m_discordSettings->sendMessage(embed, isMention, img);
    m_hadDiscordMessage = true;
}
