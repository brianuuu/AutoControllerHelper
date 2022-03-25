#include "audioconversionutils.h"

AudioConversionUtils& AudioConversionUtils::instance()
{
    static AudioConversionUtils utils;
    return utils;
}

AudioConversionUtils::AudioConversionUtils()
{
    m_hanningFunction.resize(FFT_SAMPLE_COUNT);
    for (int i = 0; i < FFT_SAMPLE_COUNT / 2; i++)
    {
        m_hanningFunction[i] = 0.5f - 0.5f * std::cos((2.0f * float(M_PI) * i) / (FFT_SAMPLE_COUNT - 1));
        m_hanningFunction[FFT_SAMPLE_COUNT - 1 - i] = m_hanningFunction[i];
    }

    m_spikeConvFunction.resize(18);
    for (int i = 0; i < 9; i++)
    {
        m_spikeConvFunction[i] = -4.0f + 8.f * i / 8.0f;
        m_spikeConvFunction[17 - i];
    }
}

void AudioConversionUtils::debugAudioFormat(const QAudioFormat &audioFormat)
{
    qDebug() << "Debug Audio Format:";

    QString str = "Sample type: ";
    switch(audioFormat.sampleType())
    {
    case QAudioFormat::SampleType::Float:       str += "Float";         break;
    case QAudioFormat::SampleType::SignedInt:   str += "SignedInt";     break;
    case QAudioFormat::SampleType::UnSignedInt: str += "UnSignedInt";   break;
    default:                                    str += "Error";         break;
    }

    qDebug() << "Bytes per Sample:" << audioFormat.bytesPerFrame() / audioFormat.channelCount();
    qDebug() << "Channel Count:" << audioFormat.channelCount();
    qDebug() << "Sample Rate:" << audioFormat.sampleRate();
    qDebug() << "Codec:" << audioFormat.codec();
}

const QVector<float> &AudioConversionUtils::getHanningFunction()
{
    return instance().m_hanningFunction;
}

const QVector<float> &AudioConversionUtils::getSpikeConvFunction()
{
    return instance().m_spikeConvFunction;
}

//-----------------------------------------
// Normalize (type to float)
//-----------------------------------------
bool AudioConversionUtils::convertSamplesToFloat(const QAudioFormat &format, const char *data, size_t dataSize, QVector<float> &out)
{
    switch(format.sampleType())
    {
    case QAudioFormat::SignedInt:
    {
        switch(format.sampleSize())
        {
        case 8: normalizeType<int8_t>(format, data, dataSize, out); return true;
        case 16: normalizeType<int16_t>(format, data, dataSize, out); return true;
        case 32: normalizeType<int32_t>(format, data, dataSize, out); return true;
        default: return false;
        }
    }
    case QAudioFormat::UnSignedInt:
    {
        switch(format.sampleSize())
        {
        case 8: normalizeType<uint8_t>(format, data, dataSize, out); return true;
        case 16: normalizeType<uint16_t>(format, data, dataSize, out); return true;
        case 32: normalizeType<uint32_t>(format, data, dataSize, out); return true;
        default: return false;
        }
    }
    case QAudioFormat::Float:
    {
        if (format.sampleSize() == sizeof(float) * 8)
        {
            out.resize(static_cast<int>(dataSize / sizeof(float)));
            memcpy(out.data(), data, dataSize);
            return true;
        }
        return false;
    }
    default: return false;
    }
}

template<typename Type>
void AudioConversionUtils::normalizeType(const QAudioFormat &format, const char *data, size_t dataSize, QVector<float> &out)
{
    // Allocate memory here
    out.resize(static_cast<int>(dataSize / sizeof(Type)));
    normalizeAudio<Type>(reinterpret_cast<const Type*>(data), out.data(), dataSize / sizeof(Type), format.byteOrder() == QAudioFormat::LittleEndian);
}

template<typename Type>
void AudioConversionUtils::normalizeAudio(const Type *in, float *out, size_t outSize, bool isLittleEndian)
{
    const float rcp = (std::is_unsigned<Type>::value ? 2.0f : 1.0f) / std::numeric_limits<Type>::max();
    const float sub = std::is_unsigned<Type>::value ? 1.0f : 0.0f;
    for (size_t c = 0; c < outSize; c++)
    {
        out[c] = ((float)(isLittleEndian ? in[c] : byteSwap(in[c])) * rcp - sub);
    }
}

//-----------------------------------------
// Fast Fourier Transform
//-----------------------------------------
void AudioConversionUtils::fft(int sampleSize, fftwf_complex *in, fftwf_complex *out)
{
    // create a DFT plan
    fftwf_plan plan = fftwf_plan_dft_1d(sampleSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // execute the plan
    fftwf_execute(plan);

    // do some cleaning
    fftwf_destroy_plan(plan);
    fftwf_cleanup();
}

void AudioConversionUtils::ifft(int sampleSize, fftwf_complex *in, fftwf_complex *out)
{
    // create an IDFT plan
    fftwf_plan plan = fftwf_plan_dft_1d(sampleSize, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    // execute the plan
    fftwf_execute(plan);

    // do some cleaning
    fftwf_destroy_plan(plan);
    fftwf_cleanup();

    // scale the output to obtain the exact inverse
    for (int i = 0; i < sampleSize; ++i)
    {
        out[i][REAL] /= sampleSize;
        out[i][IMAG] /= sampleSize;
    }
}

void AudioConversionUtils::debugComplex(fftwf_complex *c, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (c[i][IMAG] < 0)
        {
            qDebug() << QString::number(c[i][REAL]) + " - " + QString::number(qAbs(c[i][IMAG])) + "i";
        }
        else
        {
            qDebug() << QString::number(c[i][REAL]) + " + " + QString::number(c[i][IMAG]) + "i";
        }
    }
}

void AudioConversionUtils::fftOutToSpectrogram(int sampleSize, const fftwf_complex *in, QVector<float> &out)
{
    if (out.size() != sampleSize / 2)
    {
        out.resize(sampleSize / 2);
    }

    for (int i = 0; i < sampleSize / 2; i++)
    {
        float const& real = in[i][REAL];
        float const& imag = in[i][IMAG];
        float const mag = std::sqrt(real * real + imag * imag) / (sampleSize / 2);

        // Get magnitude in log scale, this will be [0,-inf]
        out[i] = 0.0f;
        if (mag > 0.0f)
        {
            constexpr float minMag = -10.0f;
            constexpr float maxMag = -3.0f;
            float const logMag = std::log(mag);
            if (logMag > minMag)
            {
                out[i] = 1.0f - ((maxMag - logMag) / (maxMag - minMag));
            }
        }
    }
}

void AudioConversionUtils::spikeConvolution(int indexStart, int indexEnd, const QVector<float> &in, QVector<float> &out, float threshold)
{
    QVector<float> const& spikeConvFunc = instance().getSpikeConvFunction();
    int outSize = (indexEnd - indexStart) - spikeConvFunc.size() + 1;
    if (out.size() != outSize)
    {
        out.resize(outSize);
    }

    for(int i = 0; i < outSize; i++)
    {
        out[i] = 0.0f;
        for(int j = 0; j < spikeConvFunc.size(); j++)
        {
            out[i] += in[indexStart + i + j] * spikeConvFunc[j];
        }

        if (out[i] <= threshold)
        {
            out[i] = 0.0f;
        }
    }
}
