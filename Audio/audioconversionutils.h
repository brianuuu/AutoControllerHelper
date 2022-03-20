#ifndef AUDIOCONVERSIONUTILS_H
#define AUDIOCONVERSIONUTILS_H

#include <QAudioFormat>
#include <QWidget>

class AudioConversionUtils
{
public:
    // Main conversion function
    static bool convertSamplesToFloat(const QAudioFormat& format, const char* data, size_t dataSize, QVector<float>& out);

private:
    // Byte swap template functions
    inline static uint8_t byteSwap(uint8_t x) { return x; }
    inline static int8_t byteSwap(int8_t x) { return x; }
    inline static uint16_t byteSwap(uint16_t x) { return (uint32_t)_byteswap_ulong(x) >> 16; }
    inline static int16_t byteSwap(int16_t x) { return (int16_t)((uint32_t)_byteswap_ulong(x) >> 16); }
    inline static uint32_t byteSwap(uint32_t x) { return _byteswap_ulong(x); }
    inline static int16_t byteSwap(int32_t x) { return (int16_t)_byteswap_ulong(x); }

    // PCM data conversion to float
    template <typename Type>
    static void normalizeType(const QAudioFormat& format, const char* data, size_t dataSize, QVector<float>& out);
    template <typename Type>
    static void normalizeAudio(const Type* in, float* out, size_t outSize, bool isLittleEndian);
};

#endif // AUDIOCONVERSIONUTILS_H
