#include "audioconversionutils.h"

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
