#include "smartprogrambase.h"

#include "autocontrollerwindow.h"

SmartProgramBase::SmartProgramBase(SmartProgramParameter parameter)
    : QWidget(parameter.parent)
    , m_parameters(parameter)
    , m_ocrHSVRange(0,0,0,0,0,0)
{
    init();
}

bool SmartProgramBase::run()
{
    if (m_state == S_NotStarted)
    {
        if (getProgramExportLog(getProgramEnum()) && m_parameters.settings->isLogAutosave())
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
    //if (m_state == S_CommandRunning)
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

    // OCR
    m_ocrRect = QRect();
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
    Q_ASSERT(m_state == S_CaptureReady);

    QImage cropped = m_capture.copy(rectPos);
    if (m_parameters.preview)
    {
        m_parameters.preview->clear();
        m_parameters.preview->setSceneRect(cropped.rect());
        m_parameters.preview->addPixmap(QPixmap::fromImage(cropped));
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

            if (m_parameters.previewMasked)
            {
                matched ? SET_BIT(rowMaskedData[x / 8], x % 8) : CLEAR_BIT(rowMaskedData[x / 8], x % 8);
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
    Q_ASSERT(m_state == S_CaptureReady);

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

QImage SmartProgramBase::getMonochromeImage(QRect rectPos, SmartProgramBase::HSVRange hsvRange, bool whiteIsOne)
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

double SmartProgramBase::getImageMatch(QRect rectPos, SmartProgramBase::HSVRange hsvRange, const QImage &testImage)
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

bool SmartProgramBase::checkImageMatchTarget(QRect rectPos, SmartProgramBase::HSVRange hsvRange, const QImage &testImage, double target, QPoint* offset)
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
    if (m_parameters.settings->isLogDebugColor())
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

bool SmartProgramBase::inializeCommands(int size)
{
    m_commands.clear();
    if (!m_parameters.smartProgramCommands)
    {
        m_errorMsg = "Commands .xml file missing!";
        m_state = S_Error;
        return false;
    }

    // No command
    if (size <= 0)
    {
        return true;
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
        qDebug() << "Invalid command index";
        m_state = S_Error;
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
        qDebug() << "Invalid custom command";
        m_state = S_Error;
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

void SmartProgramBase::runStateLoop()
{
    if (m_runNextState)
    {
        m_runNextState = false;
        runNextState();
    }
}

bool SmartProgramBase::startOCR(QRect rectPos, SmartProgramBase::HSVRange hsvRange, bool isNumber)
{
    // Get filtered image, flip black/white since black text detects better
    QImage masked = getMonochromeImage(rectPos, hsvRange, false);
    masked.save(QString(RESOURCES_PATH) + "Tesseract/capture.png", "PNG");

    // Check if .traineddata exist
    GameLanguage gameLanguage = isNumber ? GL_English : m_parameters.settings->getGameLanguage();
    if (!m_parameters.settings->ensureTrainedDataExist())
    {
        QString languageName = SmartProgramSetting::getGameLanguageName(gameLanguage);
        setState_error("Language trained data for '" + languageName + "' for Tesseract is missing, please goto 'Tesseract' folder and follow the instructions in README.md");

        m_runNextState = true;
        return false;
    }

    QString command = QString(RESOURCES_PATH) + "Tesseract/tesseract.exe ";
    command += "./capture.png ./output --tessdata-dir . ";
    command += "-l " + SmartProgramSetting::getGameLanguagePrefix(gameLanguage);
    command += " --psm 7 --oem 2 -c tessedit_create_txt=1";
    m_ocrProcess.setWorkingDirectory(QString(RESOURCES_PATH) + "Tesseract/");
    m_ocrProcess.start(command);

    return true;
}

QString SmartProgramBase::stringRemoveNonAlphaNumeric(const QString &str)
{
    QString temp;
    for (QChar c : str)
    {
        if (c.isLetterOrNumber()
         || c == QChar(0x3099)  // Japanese dakuten
         || c == QChar(0x309A)) // Japanese handakuten
        {
            temp += c;
            continue;
        }
    }

    return temp;
}

QString SmartProgramBase::normalizeString(const QString &str)
{
    QString temp = str.normalized(QString::NormalizationForm_KD);
    temp = stringRemoveNonAlphaNumeric(temp);
    return temp.toLower();
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
    // Note: "query" should be already normalized
    // This function finds the best match with in the entry,
    // though this doesn't matter much as any match in an entry always counts

    int minDist = INT_MAX;
    int minSubStringID = -1;
    for (int i = 0; i < subStrings.size(); i++)
    {
        // TODO: pre-normalize all substrings in database
        QString const subString = normalizeString(subStrings[i]);

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
        emit printLog("OCR returned empty string, expected number", LOG_ERROR);
        return false;
    }

    QString numStr;
    bool hasDigit = false;
    for (QChar ch : queryRaw)
    {
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
        emit printLog("OCR returned number: " + numStr);
    }

    return ok;
}

int SmartProgramBase::matchStringDatabase(const QVector<OCREntry> &database)
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

    // Nothing
    if (queryRaw.isEmpty())
    {
        emit printLog("OCR returned empty string");
        return -1;
    }

    // Do comparison with each database string, find the best match entry
    QString query = normalizeString(queryRaw);

    int minDist = INT_MAX;
    int minMatchedEntry = -1;
    int minSubStringMatched = -1;
    for (int i = 0; i < database.size(); i++)
    {
        int dist = 0;
        QStringList const& subStrings = database[i].second;
        int subStringMatched = matchSubStrings(query, subStrings, &dist);
        if (subStringMatched >= 0 && subStringMatched < subStrings.size() && dist < minDist)
        {
            minDist = dist;
            minMatchedEntry = i;
            minSubStringMatched = subStringMatched;
        }
    }

    if (minMatchedEntry >= 0)
    {
        OCREntry const& entry = database[minMatchedEntry];
        emit printLog("OCR text '" + queryRaw + "' has matched entry " + entry.first + ": '" + entry.second[minSubStringMatched] + "' from database (LD = " + QString::number(minDist) + ")");
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
        if (m_parameters.settings->isLogDebugCommand())
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
    case S_OCRRequested:
    {
        m_parameters.vlcWrapper->getFrame(m_capture);
        if (!m_ocrRect.isNull())
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
                    if (!m_parameters.settings->isStreamCounterExcludePrefix())
                    {
                        stream << key + ": ";
                    }
                    stream << count;
                    file.close();
                }
            }
        }
    }
    m_parameters.statsLabel->setText(statsStr);
}
