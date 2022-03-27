#include "audiomanager.h"

AudioManager::AudioManager(QWidget *parent) : QWidget(parent)
{
    this->setFixedSize(640,100);
    this->hide();
    m_displayImage = QImage(this->size(), QImage::Format_RGB32);
    m_displayImage.fill(Qt::black);

    // Set up global audio format
    m_audioFormat.setSampleRate(48000);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_audioFormat))
    {
        QMessageBox::warning(this, "Warning", "Raw audio format not supported by backend, cannot play audio.", QMessageBox::Ok);
        m_audioOutput = Q_NULLPTR;
    }
    else
    {
        m_audioOutput = new QAudioOutput(m_audioFormat, this);
    }

    m_audioDevice = Q_NULLPTR;
    connect(this, &AudioManager::drawSignal, this, &AudioManager::drawSlot);

    // Display
    m_displayMode = ADM_None;
    m_displaySamples = 1024;

    // Raw wave data
    m_rawWaveDataSize = 0;

    // Spectrogram data
    m_fftBufferData.resize(FFT_SAMPLE_COUNT * 8);
    m_fftAnalysisStart = 0;
    m_fftNewDataStart = 0;
    m_fftDataIn = fftwf_alloc_complex(FFT_SAMPLE_COUNT);
    m_fftDataOut = fftwf_alloc_complex(FFT_SAMPLE_COUNT);
    m_spectrogramData.resize(FFT_SAMPLE_COUNT / 2);
    connect(this, &AudioManager::newFFTBufferDataSignal, this, &AudioManager::newFFTBufferDataSlot);

    m_freqLow = 100;
    m_freqHigh = 10500;

    // Sound detection
    m_cachedSpikes.reserve(MAX_DETECTION_WINDOW);
    m_detectedWindowSize = 0;
}

AudioManager::~AudioManager()
{
    fftwf_free(m_fftDataIn);
    fftwf_free(m_fftDataOut);

    for (AudioFileHolder* holder : m_audioFileHolders)
    {
        delete holder;
    }
}

void AudioManager::start()
{
    m_audioDevice = m_audioOutput->start();
    m_detectedWindowSize = 0;
    QWidget::update();
}

void AudioManager::stop()
{
    m_playbackMutex.lock();
    {
        m_audioOutput->stop();
        m_audioDevice = Q_NULLPTR;
    }
    m_playbackMutex.unlock();

    resetRawWaveData();
    resetFFTBufferData();
    stopDetection();
    QWidget::update();
}

//---------------------------------------------
// Slots
//---------------------------------------------
void AudioManager::setVolume(int volume)
{
    qDebug() << "Volume set to" << volume;
    volume = qBound(0, volume, 100);

    m_playbackMutex.lock();
    {
        if (volume == 100)
        {
            m_volumeScaleDB = 1.0;
        }
        else if (volume == 0)
        {
            m_volumeScaleDB = 0.0;
        }
        else
        {
            constexpr double exp = 20.0;
            m_volumeScaleDB = (qPow(exp, double(volume) * 0.01) - 1.0) / (exp - 1.0);
        }
    }
    m_playbackMutex.unlock();
}

void AudioManager::displayModeChanged(int index)
{
    qDebug() << "Display Mode Updated to" << index;
    m_displayMutex.lock();
    {
        m_displayMode = AudioDisplayMode(index);

        // Set display height
        if (index > ADM_None && index < ADM_COUNT)
        {
            this->show();
        }
        else
        {
            this->hide();
        }

        // Handle resetting data
        switch (m_displayMode)
        {
        case ADM_RawWave:
        {
            resetRawWaveData_NonTS();
            break;
        }
        case ADM_FreqBars:
        case ADM_Spectrogram:
        {
            resetFFTBufferData_NonTS();
            break;
        }
        default: break;
        }
    }
    m_displayMutex.unlock();

    // call paintEvent
    QWidget::update();
}

void AudioManager::displaySampleChanged(int count)
{
    qDebug() << "Display Sample size changed to" << count;
    m_displayMutex.lock();
    {
        m_displaySamples = count;
        resetRawWaveData_NonTS();
        resetFFTBufferData_NonTS();
    }
    m_displayMutex.unlock();

    QWidget::update();
}

void AudioManager::drawSlot()
{
    QWidget::update();
}

void AudioManager::freqLowChanged(int value)
{
    m_displayMutex.lock();
    {
        m_freqLow = value;
        m_displayImage.fill(Qt::black);
    }
    m_displayMutex.unlock();
}

void AudioManager::freqHighChanged(int value)
{
    m_displayMutex.lock();
    {
        m_freqHigh = value;
        m_displayImage.fill(Qt::black);
    }
    m_displayMutex.unlock();
}

void AudioManager::newFFTBufferDataSlot()
{
    // TODO: if we have more data ready, do multiple FFT a frame instead of waiting another signal?
    bool doFFT = false;
    m_fftDataMutex.lock();
    {
        // Check if we have enough data
        int unprocessedDataSize = m_fftNewDataStart - m_fftAnalysisStart;
        if (m_fftNewDataStart < m_fftAnalysisStart)
        {
            unprocessedDataSize += m_fftBufferData.size();
        }

        if (unprocessedDataSize >= FFT_SAMPLE_COUNT)
        {
            doFFT = true;

            // Grab input FFT data, apply Hanning window to reduce leakage
            QVector<float> const& hanningFunction = AudioConversionUtils::getHanningFunction();
            int pos = m_fftAnalysisStart;
            for (int i = 0; i < FFT_SAMPLE_COUNT; i++)
            {
                m_fftDataIn[i][REAL] = m_fftBufferData[pos] * hanningFunction[i];
                m_fftDataIn[i][IMAG] = 0.0f;

                pos++;
                if (pos >= m_fftBufferData.size())
                {
                    pos = 0;
                }
            }

            // Shift to the next window
            m_fftAnalysisStart = (m_fftAnalysisStart + FFT_WINDOW_STEP) % m_fftBufferData.size();
        }
        else
        {
            qDebug() << "Waiting for more FFT input data";
        }
    }
    m_fftDataMutex.unlock();

    // Do FFT analysis, TODO: move this to other thread?
    if (doFFT)
    {
        AudioConversionUtils::fft(FFT_SAMPLE_COUNT, m_fftDataIn, m_fftDataOut);
        AudioConversionUtils::fftOutToSpectrogram(FFT_SAMPLE_COUNT, m_fftDataOut, m_spectrogramData);

        doDetection();
        QWidget::update();
    }
}

//---------------------------------------------
// Recieve data
//---------------------------------------------
void AudioManager::pushAudioData(const void *samples, unsigned int count, int64_t pts)
{
    if (!isStarted()) return;

    size_t sampleSize = count * m_audioFormat.bytesPerFrame();

    // Only support this atm...
    if (m_audioFormat.sampleSize() != 16 || m_audioFormat.sampleType() != QAudioFormat::SignedInt) return;

    // Playback
    m_playbackMutex.lock();
    if (m_volumeScaleDB == 1.0)
    {
        // Don't convert anything
        m_audioDevice->write((const char*)samples, sampleSize);
    }
    else if (m_volumeScaleDB > 0.0)
    {
        // Scale the audio, unfortunately we have to assume the data type here...
        int16_t* sampleScaled = new int16_t[sampleSize / sizeof(int16_t)];
        memcpy(sampleScaled, samples, sampleSize);
        for (size_t i = 0; i < sampleSize / sizeof(int16_t); i++)
        {
            sampleScaled[i] = static_cast<int16_t>(double(sampleScaled[i]) * m_volumeScaleDB);
        }
        m_audioDevice->write((const char*)sampleScaled, sampleSize);
        delete[] sampleScaled;
    }
    m_playbackMutex.unlock();

    // Convert raw samples to float
    QVector<float> newData;
    AudioConversionUtils::convertSamplesToFloat(m_audioFormat, (const char*)samples, sampleSize, newData);

    // Processing
    switch (m_displayMode)
    {
    case ADM_RawWave:
    {
        writeRawWaveData(newData);
        break;
    }
    case ADM_FreqBars:
    case ADM_Spectrogram:
    {
        writeFFTBufferData(newData);
        break;
    }
    default: break;
    }
}

//---------------------------------------------
// Detection
//---------------------------------------------
int AudioManager::addDetection(const QString &fileName, float minScore, int lowFreqFilter)
{
    AudioFileHolder* holder = nullptr;
    if (!m_audioFileHolders.contains(fileName))
    {
        QString errorStr;
        holder = new AudioFileHolder(this);
        if (!holder->loadWaveFile(fileName, m_audioFormat, minScore, lowFreqFilter, errorStr))
        {
            emit printLog(errorStr, LOG_ERROR);
            delete holder;
            return 0;
        }

        if (holder->getWindowCount() > MAX_DETECTION_WINDOW)
        {
            emit printLog("Detection sound has " + QString::number(holder->getWindowCount()) + " windows exceeded " + QString::number(MAX_DETECTION_WINDOW) + " limit");
            delete holder;
            return 0;
        }

        m_audioFileHolders[fileName] = holder;
        holder->setID(m_audioFileHolders.size());
    }
    else
    {
        holder = m_audioFileHolders[fileName];
    }

    // If we are adding that means a smart program required detection
    int min, max;
    holder->getFrequencyRange(min, max);
    emit soundDetectionRequired(min, max);

    // Return the ID of the sound
    emit printLog(holder->getFileName() + " cached (" + QString::number(holder->getWindowCount()) + " windows, ID: " + QString::number(holder->getID()) + ")");
    return holder->getID();
}

void AudioManager::startDetection(int id)
{
    for (AudioFileHolder* holder : m_audioFileHolders)
    {
        if (holder->getID() == id)
        {
            holder->getWindowSkipCounter() = 0;
            m_detectingSounds.insert(holder);
            qDebug() << "Started detecting" << holder->getFileName();
            return;
        }
    }

    emit printLog("Invalid sound detection ID", LOG_ERROR);
}

void AudioManager::doDetection()
{
    if (m_detectingSounds.isEmpty()) return;

    // Get spikes for the current spectrogram
    QVector<float> convData;
    AudioConversionUtils::spikeConvolution(0, m_spectrogramData.size(), m_spectrogramData, convData);

    SpikeIDScore spikes;
    PeakFinder::findPeaks(convData, spikes, 0, false);

    if (m_cachedSpikes.size() == MAX_DETECTION_WINDOW)
    {
        m_cachedSpikes.pop_front();
    }
    m_cachedSpikes.push_back(spikes);

    for (AudioFileHolder* holder : m_detectingSounds)
    {
        // Need to wait for enough windows to compare
        int windowCount = holder->getWindowCount();
        if (m_cachedSpikes.size() < windowCount)
        {
            continue;
        }

        // Previously detected
        int& windowSkipCounter = holder->getWindowSkipCounter();
        if (windowSkipCounter > 0)
        {
            windowSkipCounter--;
            continue;
        }

        // Get the score by finding if current windows contains the template's spikes
        // If so add the score by the magnitude of the spike and average to no. in the window
        float score = 0.0f;
        QVector<SpikeIDScore> const& spikesCollection = holder->getSpikesCollection();
        for (int i = 0; i < spikesCollection.size(); i++)
        {
            float tempScore = 0.0f;
            SpikeIDScore const& curCachedSpikes = m_cachedSpikes[m_cachedSpikes.size() - spikesCollection.size() + i];
            SpikeIDScore const& curSpikesCollection = spikesCollection[i];
            for (auto iter = curSpikesCollection.begin(); iter != curSpikesCollection.end(); iter++)
            {
                if (curCachedSpikes.contains(iter.key()))
                {
                    tempScore += iter.value();
                }
            }
            tempScore /= curSpikesCollection.size();
            score += tempScore;
        }
        score /= spikesCollection.size();
        holder->setScore(score);

        if (score > holder->getMinScore())
        {
            emit printLog(holder->getFileName() + " detected with score " + QString::number(score) + " > " + QString::number(holder->getMinScore()), LOG_SUCCESS);
            emit soundDetected(holder->getID());
            windowSkipCounter = spikesCollection.size();
            m_detectedWindowSize = spikesCollection.size();
        }
    }
}

void AudioManager::stopDetection(int id)
{
    // If specified ID, just remove that one
    if (id > 0)
    {
        for (AudioFileHolder* holder : m_detectingSounds)
        {
            if (holder->getID() == id)
            {
                holder->getWindowSkipCounter() = 0;
                m_detectingSounds.remove(holder);
                qDebug() << "Stopped detecting" << holder->getFileName();
                return;
            }
        }
        emit printLog("Invalid sound detection ID", LOG_ERROR);
        return;
    }

    // Clear all
    for (AudioFileHolder* holder : m_detectingSounds)
    {
        holder->getWindowSkipCounter() = 0;
    }
    m_detectingSounds.clear();
    m_cachedSpikes.clear();
    m_detectedWindowSize = 0;
}

//---------------------------------------------
// Raw Wave
//---------------------------------------------
void AudioManager::resetRawWaveData()
{
    m_displayMutex.lock();
    {
        resetRawWaveData_NonTS();
    }
    m_displayMutex.unlock();
}

void AudioManager::resetRawWaveData_NonTS()
{
    m_rawWaveDataSize = 0;
    for (float& f : m_rawWaveData)
    {
        f = 0.0f;
    }
}

void AudioManager::writeRawWaveData(QVector<float> const& newData)
{
    m_displayMutex.lock();
    {
        // only re-allocate if we don't have enough size
        // this way we can re-use the same memory
        m_rawWaveDataSize = newData.size() / 2;
        if (m_rawWaveData.size() < m_rawWaveDataSize)
        {
            m_rawWaveData.resize(m_rawWaveDataSize);
        }

        // Average LR channels
        for (int i = 0; i < m_rawWaveDataSize; i++)
        {
            m_rawWaveData[i] = (newData[2*i] + newData[2*i+1]) * 0.5f;
        }
    }
    m_displayMutex.unlock();
    emit drawSignal();
}

//---------------------------------------------
// FFT Spectrogram
//---------------------------------------------
void AudioManager::resetFFTBufferData()
{
    m_fftDataMutex.lock();
    {
        resetFFTBufferData_NonTS();
    }
    m_fftDataMutex.unlock();
}

void AudioManager::resetFFTBufferData_NonTS()
{
    m_fftNewDataStart = 0;
    m_fftAnalysisStart = 0;
    for (float& f : m_fftBufferData)
    {
        f = 0.0f;
    }
    for (float& f : m_spectrogramData)
    {
        f = 0.0f;
    }
    for (int i = 0; i < FFT_SAMPLE_COUNT; i++)
    {
        m_fftDataIn[i][REAL] = 0.0f;
        m_fftDataIn[i][IMAG] = 0.0f;
        m_fftDataOut[i][REAL] = 0.0f;
        m_fftDataOut[i][IMAG] = 0.0f;
    }

    m_displayImage.fill(Qt::black);
}

void AudioManager::writeFFTBufferData(QVector<float> const& newData)
{
    m_fftDataMutex.lock();
    {
        // NOTE: We can only take a maximum of FFT_WINDOW_STEP no. of samples
        int const frameCount = newData.size() / 2;
        if (frameCount > FFT_WINDOW_STEP)
        {
            emit printLog("ERROR: Input data sample size " + QString::number(frameCount) + " is larger than " + QString::number(FFT_WINDOW_STEP), LOG_ERROR);
        }

        // Push new data to buffer
        for (int i = 0; i < newData.size() / 2 && i < FFT_WINDOW_STEP; i++)
        {
            m_fftBufferData[m_fftNewDataStart] = (newData[2*i] + newData[2*i+1]) * 0.5f;

            // Warp back to beginning of the buffer
            m_fftNewDataStart++;
            if (m_fftNewDataStart >= m_fftBufferData.size())
            {
                m_fftNewDataStart = 0;
            }
        }
    }
    m_fftDataMutex.unlock();

    // Ask AudioManager to do FFT on new set of data
    emit newFFTBufferDataSignal();
}

//---------------------------------------------
// Drawing
//---------------------------------------------
void AudioManager::paintEvent(QPaintEvent* event)
{
    if (this->height() == 0) return;

    m_displayMutex.lock();
    {
        // Draw on image
        paintEvent_NonTS();
    }
    m_displayMutex.unlock();
}

void AudioManager::paintEvent_NonTS()
{
    int const width = this->width();
    int const height = this->height();
    float const heightHalf = height * 0.5f;

    // Paint image
    switch (m_displayMode)
    {
    case ADM_RawWave:
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        painter.setPen(QColor(Qt::cyan));

        // If no samples, draw a null sound line
        if (m_rawWaveDataSize == 0)
        {
            // Draw null sound line
            QPainter imagePainter(&m_displayImage);
            imagePainter.fillRect(this->rect(), Qt::black);
            imagePainter.setPen(QColor(Qt::cyan));
            imagePainter.drawLine(0, int(heightHalf), this->width(), int(heightHalf));
            painter.drawImage(this->rect(), m_displayImage);
            return;
        }

        QPoint lastPointPos(0, int(heightHalf));
        if (m_displaySamples <= width)
        {
            // fewer samples than width, we need to scale it up
            float const pointWidth = float(width) / float(m_displaySamples);
            for (int i = 0; i < m_rawWaveDataSize; i++)
            {
                float const p = m_rawWaveData[i] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(0, int(p));
                }
                else
                {
                    int const xPos = int(pointWidth) * i;
                    QPoint newPointPos = QPoint(xPos, int(p));
                    painter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }
        }
        else
        {
            // More samples then width, will need to ignore some
            float const sampleRatio = float(m_displaySamples) / float(width);
            int const drawWidth = int(float(m_rawWaveDataSize) / sampleRatio);

            // Shift previously drawn wave data
            m_displayImage = m_displayImage.copy(drawWidth, 0, width, height);
            QPainter imagePainter(&m_displayImage);
            imagePainter.setPen(QColor(Qt::cyan));

            // Draw the new data at the right side end of the image
            int const xPosStart = width - drawWidth;
            for (int i = 0; i < drawWidth; i++)
            {
                int sampleIndex = int(sampleRatio * float(i));
                if (sampleIndex >= m_rawWaveDataSize) break;

                float const p = m_rawWaveData[sampleIndex] * heightHalf + heightHalf;
                if (i == 0)
                {
                    lastPointPos = QPoint(xPosStart, int(p));
                }
                else
                {
                    QPoint newPointPos = QPoint(xPosStart + i, int(p));
                    imagePainter.drawLine(lastPointPos, newPointPos);
                    lastPointPos = newPointPos;
                }
            }

            // Finally draw the image on widget
            painter.drawImage(this->rect(), m_displayImage);
        }
        break;
    }
    case ADM_FreqBars:
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        painter.setPen(QColor(Qt::cyan));
        painter.drawLine(0, height, width, height);

        float const freqRes = float(m_audioFormat.sampleRate()) / FFT_SAMPLE_COUNT;
        int const indexStart = int(float(m_freqLow) / freqRes);
        int const indexEnd = int(float(m_freqHigh) / freqRes) + 1;
        int const drawWidth = indexEnd - indexStart;

        if (drawWidth >= width)
        {
            // More samples then width, will need to ignore some
            float const sampleRatio = float(drawWidth) / float(width);
            for (int i = 0; i < width; i++)
            {
                int sampleIndex = indexStart + int(sampleRatio * i);
                if (sampleIndex >= m_spectrogramData.size()) break;

                float const logMag = m_spectrogramData[sampleIndex];
                if (logMag > 0.0f)
                {
                    painter.drawLine(i, height, i, int((1.0f - logMag) * height));
                }
            }
        }
        else
        {
            // fewer samples than width, we need to scale it up
            float nextXPos = 0.0f;
            float const barWidth = float(width) / float(drawWidth);
            for (int i = 0; i < drawWidth; i++)
            {
                int sampleIndex = indexStart + i;
                if (sampleIndex >= m_spectrogramData.size()) break;

                float const logMag = m_spectrogramData[sampleIndex];
                if (logMag > 0.0f)
                {
                    painter.drawRect(int(nextXPos), int((1.0f - logMag) * height), int(barWidth), height);
                }
                nextXPos += barWidth;
            }
        }
        break;
    }
    case ADM_Spectrogram:
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);

        // Shift previously drawn spectrogram data
        m_displayImage = m_displayImage.copy(1, 0, width, height);
        QPainter imagePainter(&m_displayImage);

        float const freqRes = float(m_audioFormat.sampleRate()) / FFT_SAMPLE_COUNT;
        int const indexStart = int(float(m_freqLow) / freqRes);
        int const indexEnd = int(float(m_freqHigh) / freqRes);

        float const sampleRatio = float(indexEnd - indexStart + 1) / float(height);
        for (int i = 0; i < height; i++)
        {
            int sampleIndex = indexStart + int(sampleRatio * i);
            if (sampleIndex >= m_spectrogramData.size()) break;

            imagePainter.setPen(getMagnitudeColor(m_spectrogramData[sampleIndex]));
            imagePainter.drawPoint(width - 1, i);
        }

        // A sound is detected!
        if (m_detectedWindowSize > 0)
        {
            QPen pen;
            pen.setWidth(4);
            pen.setColor(Qt::green);
            imagePainter.setPen(pen);
            imagePainter.drawRect(width - m_detectedWindowSize - 1, 0, m_detectedWindowSize, height);
            m_detectedWindowSize = 0;
        }

        // Finally draw the image on widget
        painter.drawImage(this->rect(), m_displayImage);

        // Debug draw the score of detecting sounds
        if (!m_detectingSounds.isEmpty())
        {
            QFont font = painter.font();
            font.setPointSize(12);
            painter.setFont(font);
            int textPos = height - 4;
            for (AudioFileHolder* holder : m_detectingSounds)
            {
                QString text = holder->getFileName() + ": " + QString::number(holder->getScore());
                painter.setPen(Qt::black);
                painter.drawText(QPoint(4, textPos), text);
                painter.setPen(Qt::red);
                painter.drawText(QPoint(3, textPos-1), text);
                textPos += 15;
            }
        }
        break;
    }
    default: break;
    }
}

QColor AudioManager::getMagnitudeColor(float v)
{
    if (v <= 0.0f)
    {
        return QColor(0, 0, 127);
    }
    else if (v < 0.125f)
    {
        return QColor(0, 0, int((0.5f + 4.0f * v) * 255.f));
    }
    else if (v < 0.375f)
    {
        return QColor(0, int((v - 0.125f) * 1020.0f), 255);
    }
    else if (v < 0.625f)
    {
        int c = int((v - 0.375f) * 1020.0f);
        return QColor(c, 255, 255 - c);
    }
    else if (v < 0.875f)
    {
        return QColor(255, 255 - int((v - 0.625f) * 1020.0f), 0);
    }
    else if (v <= 1.0f)
    {
        return QColor(255 - int((v - 0.875f) * 1020.0f), 0, 0);
    }
    else
    {
        return QColor(127, 0, 0);
    }
}
