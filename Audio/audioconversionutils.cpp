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
