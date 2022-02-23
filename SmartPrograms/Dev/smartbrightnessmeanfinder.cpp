#include "smartbrightnessmeanfinder.h"

SmartBrightnessMeanFinder::SmartBrightnessMeanFinder
(
    QVector<QSpinBox*> spinBoxes,
    QLabel* labelMean,
    QPushButton* imageMatchBtn,
    QLabel* labelImageMatch,
    QPushButton* ocrBtn,
    QLineEdit* ocrLE,
    QPushButton* ocrNumBtn,
    SmartProgramParameter parameter
)
    : SmartProgramBase(parameter)
    , m_spinBoxes(spinBoxes)
    , m_meanOutput(labelMean)
    , m_imageMatchBtn(imageMatchBtn)
    , m_imageMatchResult(labelImageMatch)
    , m_ocrBtn(ocrBtn)
    , m_ocrLE(ocrLE)
    , m_ocrNumBtn(ocrNumBtn)
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
                getOCRNumber(number);
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
                    matchStringDatabase( { PokemonDatabase::OCREntry("TestEntry", list) } );
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
                if (m_imageMatchImage.width() > m_spinBoxes[2]->value() || m_imageMatchImage.height() > m_spinBoxes[3]->value())
                {
                    emit printLog("Capture dimension too small", LOG_WARNING);
                    m_imageMatchStarted = false;
                }
                else
                {
                    double maxRatio = 0;
                    QPoint maxRatioOffset(0,0);
                    for (int y = 0; y <= rect.height() - m_imageMatchImage.height(); y++)
                    {
                        for (int x = 0; x <= rect.width() - m_imageMatchImage.width(); x++)
                        {
                            QRect cropRect(rect.left() + x, rect.top() + y, m_imageMatchImage.width(), m_imageMatchImage.height());
                            double ratio = getImageMatch(cropRect, hsvRange, m_imageMatchImage);
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
    if (tempImage.width() > m_spinBoxes[2]->value() || tempImage.height() > m_spinBoxes[3]->value())
    {
        emit printLog("Image dimension too large", LOG_ERROR);
        return;
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
