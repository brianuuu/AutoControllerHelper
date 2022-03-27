#include "audiofileholder.h"

AudioFileHolder::AudioFileHolder(QObject *parent) : QObject(parent)
{
    m_wavFile = nullptr;
    m_freqStart = 20;
    m_freqEnd = 20000;

    m_score = 0.0f;
    m_minScore = 10000.0f;
    m_windowSkipCounter = 0;
}

AudioFileHolder::~AudioFileHolder()
{
    if (m_wavFile)
    {
        m_wavFile->close();
        delete m_wavFile;
        m_wavFile = nullptr;
    }
}

bool AudioFileHolder::loadWaveFile(const QString &filename, const QAudioFormat &audioFormat, float minScore, int lowFreqFilter, QString &errorStr)
{
    if (m_wavFile)
    {
        errorStr = "Already used by " + m_fileName;
        return false;
    }

    m_fileName = filename + ".wav";
    m_wavFile = new WavFile(this);
    if (!m_wavFile->open(RESOURCES_PATH + m_fileName))
    {
       errorStr = "Unable to open " + m_fileName;
       return false;
    }

    qDebug() << "Loading" << m_fileName;
    QAudioFormat const& wavAudioFormat = m_wavFile->audioFormat();
    AudioConversionUtils::debugAudioFormat(wavAudioFormat);
    if (audioFormat.sampleRate() != wavAudioFormat.sampleRate())
    {
        errorStr = "WavFile sample rate " + QString::number(wavAudioFormat.sampleRate()) + " does not match " + QString::number(audioFormat.sampleRate());
        return false;
    }

    // Read raw bytes
    qint64 const byteCount = m_wavFile->size() - m_wavFile->pos();
    QVector<char> rawBuffer;
    rawBuffer.resize(int(byteCount));
    qint64 const bytesRead = m_wavFile->read(rawBuffer.data(), byteCount);
    if (bytesRead != byteCount)
    {
        errorStr = "Unable to read all " + QString::number(byteCount) + "bytes from wav file: " + QString::number(bytesRead) + " read";
        return false;
    }
    else
    {
        qDebug() << byteCount << "raw bytes read from wav file";
    }

    /*QFile file(QString(RESOURCES_PATH) + "/output.csv");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);*/

    // Convert raw samples to float
    QVector<float> floatData;
    AudioConversionUtils::convertSamplesToFloat(wavAudioFormat, rawBuffer.data(), size_t(rawBuffer.size()), floatData);
    switch (wavAudioFormat.channelCount())
    {
    case 2:
    {
        for (int i = 0; i < floatData.size() / 2; i++)
        {
            floatData[i] = (floatData[2*i] + floatData[2*i+1]) * 0.5f;
        }
        floatData.resize(floatData.size() / 2);
        break;
    }
    case 1: break;
    default:
    {
        errorStr = "Invalid channel count: " + QString::number(wavAudioFormat.channelCount());
        return false;
    }
    }
    qDebug() << "Converted to" << floatData.size() << "float audio data";

    QVector<float> spectrogramData;
    QVector<float> convData;
    QVector<float> const& hanningFunction = AudioConversionUtils::getHanningFunction();

    fftwf_complex* fftDataIn = fftwf_alloc_complex(FFT_SAMPLE_COUNT);
    fftwf_complex* fftDataOut = fftwf_alloc_complex(FFT_SAMPLE_COUNT);

    m_freqStart = lowFreqFilter;
    m_freqEnd = 20000.0f;
    float const freqRes = float(wavAudioFormat.sampleRate()) / FFT_SAMPLE_COUNT;
    int const indexStart = int(float(m_freqStart) / freqRes);
    int const indexEnd = int(m_freqEnd / freqRes) + 1;

    int const sampleCount = floatData.size();
    int const windowCount = (sampleCount < FFT_SAMPLE_COUNT) ? 1 : (sampleCount - FFT_SAMPLE_COUNT) / FFT_WINDOW_STEP + 1;

    int dataStart = 0;
    for(int i = 0; i < windowCount; i++)
    {
        for (int j = 0; j < FFT_SAMPLE_COUNT; j++)
        {
            fftDataIn[j][REAL] = (j < sampleCount) ? floatData[dataStart + j] * hanningFunction[j] : 0.0f;
            fftDataIn[j][IMAG] = 0.0f;
        }

        AudioConversionUtils::fft(FFT_SAMPLE_COUNT, fftDataIn, fftDataOut);
        AudioConversionUtils::fftOutToSpectrogram(FFT_SAMPLE_COUNT, fftDataOut, spectrogramData);
        AudioConversionUtils::spikeConvolution(indexStart, indexEnd, spectrogramData, convData);

        dataStart += FFT_WINDOW_STEP;

        SpikeIDScore spikes;
        PeakFinder::findPeaks(convData, spikes, indexStart, false);
        m_spikesCollection.push_back(spikes);

        // Output log
        /*for (int j = 0; j < spikes.size(); j++)
        {
            if (j > 0) out << ",";
            out << spikes[j];
        }
        out << "\n";*/
    }

    fftwf_free(fftDataIn);
    fftwf_free(fftDataOut);

    qDebug() << "Finished adding" << m_fileName << "with" << windowCount << "windows";

    m_minScore = minScore;
    return true;
}

void AudioFileHolder::getFrequencyRange(int &start, int &end)
{
    start = m_freqStart;
    end = m_freqEnd;
}

int AudioFileHolder::getWindowCount() const
{
    return m_spikesCollection.size();
}

const QVector<SpikeIDScore> &AudioFileHolder::getSpikesCollection()
{
    return m_spikesCollection;
}
