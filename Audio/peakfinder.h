// PeakFinder.h
#ifndef PEAKFINDER_H
#define PEAKFINDER_H

#include <QDebug>
#include <QWidget>
#include <QtMath>

#include "audioconversionutils.h"

class PeakFinder
{
    static constexpr float EPS = 2.2204e-16f;

    /*
        Inputs
        x0: input signal
        extrema: 1 if maxima are desired, -1 if minima are desired
        includeEndpoints - If true the endpoints will be included as possible extrema otherwise they will not be included
        Output
        peakInds: Indices of peaks in x0
    */

public:
    static void findPeaks(QVector<float> const& in, SpikeIDScore& peakInds, int indexStart, bool includeEndpoints = true, float extrema = 1);
};

#endif
