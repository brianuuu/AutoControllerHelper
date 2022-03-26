#ifndef AUDIOCONVERSIONUTILS_H
#define AUDIOCONVERSIONUTILS_H

#include <QAudioFormat>
#include <QDebug>
#include <QWidget>
#include <QtMath>

#include <fftw3.h>
#define REAL 0
#define IMAG 1

#define FFT_SAMPLE_COUNT    4096
#define FFT_WINDOW_STEP     1024

typedef QMap<int, float> SpikeIDScore;

class AudioConversionUtils
{
private:
    static AudioConversionUtils& instance();
    AudioConversionUtils();

public:
    // Utils
    static void debugAudioFormat(QAudioFormat const& audioFormat);
    static QVector<float> const& getHanningFunction();
    static QVector<float> const& getSpikeConvFunction();

    // Main conversion function
    static bool convertSamplesToFloat(const QAudioFormat& format, const char* data, size_t dataSize, QVector<float>& out);

    // Fast Fourier Transform
    static void fft(int sampleSize, fftwf_complex *in, fftwf_complex *out);
    static void ifft(int sampleSize, fftwf_complex *in, fftwf_complex *out);
    static void debugComplex(fftwf_complex *c, int size);
    static void fftOutToSpectrogram(int sampleSize, fftwf_complex const* in, QVector<float>& out);
    static void spikeConvolution(int indexStart, int indexEnd, QVector<float> const& in, QVector<float>& out, float threshold = 1.0f);

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

private:
    QVector<float> m_hanningFunction;
    QVector<float> m_spikeConvFunction;
};

#endif // AUDIOCONVERSIONUTILS_H
