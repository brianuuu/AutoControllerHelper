#include "smartbrightnessmeanfinder.h"

SmartBrightnessMeanFinder::SmartBrightnessMeanFinder
(
    QVector<QSpinBox*> spinBoxes,
    QLabel* labelMean,
    QPushButton* imageMatchBtn,
    QLabel* labelImageMatch,
    QCheckBox* matchScale,
    QPushButton* ocrBtn,
    QLineEdit* ocrLE,
    QPushButton* ocrNumBtn,
    QPushButton* fixedImageBtn,
    QCheckBox* fixedImageCheck,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_spinBoxes(spinBoxes)
    , m_meanOutput(labelMean)
    , m_imageMatchBtn(imageMatchBtn)
    , m_imageMatchResult(labelImageMatch)
    , m_imageMatchScale(matchScale)
    , m_ocrBtn(ocrBtn)
    , m_ocrLE(ocrLE)
    , m_ocrNumBtn(ocrNumBtn)
    , m_fixedImageBtn(fixedImageBtn)
    , m_fixedImageCheck(fixedImageCheck)
{
    init();

    if (m_spinBoxes.size() != 10)
    {
        setState_error("Spinbox argument is expected to be 10");
    }
    else
    {
        for (QSpinBox* spinBox : m_spinBoxes)
        {
            if (!spinBox)
            {
                setState_error("Null spinbox reference provided");
                break;
            }
        }
    }
}

void SmartBrightnessMeanFinder::init()
{
    SmartProgramBase::init();

    connect(m_imageMatchBtn, &QPushButton::clicked, this, &SmartBrightnessMeanFinder::imageMatchAdd);
    m_imageMatchStarted = false;

    connect(m_ocrBtn, &QPushButton::clicked, this, &SmartBrightnessMeanFinder::orcRequested);
    m_ocrRequested = false;

    connect(m_ocrNumBtn, &QPushButton::clicked, this, &SmartBrightnessMeanFinder::orcNumRequested);
    m_ocrNumRequested = false;

    connect(m_fixedImageBtn, &QPushButton::clicked, this, &SmartBrightnessMeanFinder::fixedImageAdd);
    connect(m_fixedImageCheck, &QCheckBox::stateChanged, this, &SmartBrightnessMeanFinder::fixImageStateChanged);
}

void SmartBrightnessMeanFinder::reset()
{
    SmartProgramBase::reset();

    m_substage = SS_Init;
}

void SmartBrightnessMeanFinder::runNextState()
{
    State const state = getState();
    switch (m_substage)
    {
    case SS_Init:
    {
        m_substage = SS_Capture;
        setState_frameAnalyzeRequest();
        break;
    }
    case SS_Capture:
    {
        if (state == S_OCRReady)
        {
            if (m_ocrNumRequested)
            {
                int number = 0;
                if (getOCRNumber(number))
                {
                    emit printLog("OCR returned number: " + QString::number(number));
                }
            }
            else
            {
                if (m_ocrLE->text().isEmpty())
                {
                    emit printLog("OCR returned string: " + getOCRStringRaw());
                }
                else
                {
                    QStringList list = m_ocrLE->text().split(",");
                    for (QString& str : list)
                    {
                        str = PokemonDatabase::normalizeString(str);
                    }
                    PokemonDatabase::OCREntries entries;
                    entries["TestEntry"] = list;
                    matchStringDatabase(entries);
                }
            }

            m_ocrRequested = false;
            m_ocrNumRequested = false;

            m_substage = SS_Init;
            runNextStateDelay(100);
        }
        else if (state == S_CaptureReady)
        {
            QRect rect(m_spinBoxes[0]->value(), m_spinBoxes[1]->value(), m_spinBoxes[2]->value(), m_spinBoxes[3]->value());
            QColor minHSV, maxHSV;
            minHSV.setHsv(m_spinBoxes[4]->value(), m_spinBoxes[5]->value(), m_spinBoxes[6]->value());
            maxHSV.setHsv(m_spinBoxes[7]->value(), m_spinBoxes[8]->value(), m_spinBoxes[9]->value());
            HSVRange hsvRange(minHSV,maxHSV);

            double mean = getBrightnessMean(rect, hsvRange);
            m_meanOutput->setText("Brightness Mean = " + QString::number(mean));

            // Image matching
            if (m_imageMatchStarted)
            {
                QImage const image = m_imageMatchScale->isChecked() ? m_imageMatchImage.scaled(m_spinBoxes[2]->value(), m_spinBoxes[3]->value()) : m_imageMatchImage;
                if (image.width() > m_spinBoxes[2]->value() || image.height() > m_spinBoxes[3]->value())
                {
                    emit printLog("Capture dimension too small", LOG_WARNING);
                    m_imageMatchStarted = false;
                }
                else if (image.width() < m_spinBoxes[2]->value() / 2 || image.height() < m_spinBoxes[3]->value() / 2)
                {
                    emit printLog("Image is too small, less than half size of capture area", LOG_WARNING);
                    m_imageMatchStarted = false;
                }
                else
                {
                    double maxRatio = 0;
                    QPoint maxRatioOffset(0,0);
                    for (int y = 0; y <= rect.height() - image.height(); y++)
                    {
                        for (int x = 0; x <= rect.width() - image.width(); x++)
                        {
                            QRect cropRect(rect.left() + x, rect.top() + y, image.width(), image.height());
                            double ratio = getImageMatch(cropRect, hsvRange, image);
                            if (ratio > maxRatio)
                            {
                                maxRatio = ratio;
                                maxRatioOffset = QPoint(x,y);
                            }
                        }
                    }
                    m_imageMatchResult->setText("Similarity Ratio = " + QString::number(maxRatio) + ", Offset: {" + QString::number(maxRatioOffset.x()) + "," + QString::number(maxRatioOffset.y()) + "}");
                }
            }

            if (m_ocrRequested || m_ocrNumRequested)
            {
                // manually start OCR
                startOCR(rect, hsvRange);
                break;
            }

            m_substage = SS_Init;
            runNextStateDelay(100);
        }
        break;
    }
    }

    SmartProgramBase::runNextState();
}

void SmartBrightnessMeanFinder::imageMatchAdd()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Import Image"), "", "Monochrone BMP (*.bmp)");
    if (file.isEmpty()) return;

    QImage tempImage(file);
    if (!m_imageMatchScale->isChecked())
    {
        if (tempImage.width() > m_spinBoxes[2]->value() || tempImage.height() > m_spinBoxes[3]->value())
        {
            emit printLog("Image dimension too large", LOG_ERROR);
            return;
        }
    }

    m_imageMatchImage = tempImage.convertToFormat(QImage::Format_MonoLSB, Qt::MonoOnly);
    m_imageMatchStarted = true;

    /*for (int y = 1; y < m_imageMatchImage.height(); y++)
    {
        uint8_t *rowData = (uint8_t*)m_imageMatchImage.scanLine(y);
        for (int x = 0; x < m_imageMatchImage.width(); x++)
        {
            uint8_t byte = rowData[x / 8];
            qDebug() << (CHECK_BIT(byte, x % 8) ? 0 : 1);
        }
    }*/
}

void SmartBrightnessMeanFinder::fixedImageAdd()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Import Image"), "", "PNG file (*.png)");
    if (file.isEmpty()) return;

    QImage tempImage(file);
    if (tempImage.width() != 1280 || tempImage.height() != 720)
    {
        emit printLog("Image dimension must be 1280x720", LOG_ERROR);
        return;
    }

    m_videoManager->setFixedImage(tempImage.convertToFormat(QImage::Format_ARGB32));
}

void SmartBrightnessMeanFinder::fixImageStateChanged(int state)
{
    m_videoManager->setFixedImageUsed(state == Qt::Checked);
}


