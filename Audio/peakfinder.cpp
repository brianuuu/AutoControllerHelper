// PeakFinder.cpp
#include "peakfinder.h"

void diff(std::vector<float> in, std::vector<float>& out)
{
    out = std::vector<float>(in.size()-1);
    for (size_t i = 1; i < in.size(); ++i)
    {
        out[i - 1] = in[i] - in[i - 1];
    }
}

void vectorElementsProduct(std::vector<float> a, std::vector<float> b, std::vector<float>& out)
{
    out = std::vector<float>(a.size());
    for (size_t i = 0; i < a.size(); ++i)
    {
        out[i] = a[i] * b[i];
    }
}

void findIndicesLessThan(std::vector<float> in, float threshold, std::vector<int>& indices)
{
    for (size_t i = 0; i < in.size(); ++i)
    {
        if(in[i] < threshold)
        {
            indices.push_back(int(i + 1));
        }
    }
}

void selectElementsFromIndices(std::vector<float> in, std::vector<int> indices, std::vector<float>& out)
{
    for (size_t i = 0; i < indices.size(); ++i)
    {
        out.push_back(in[ size_t(indices[i]) ]);
    }
}

void selectElementsFromIndices(std::vector<int> in, std::vector<int> indices, std::vector<int>& out)
{
    for (size_t i = 0; i < indices.size(); ++i)
    {
        out.push_back(in[ size_t(indices[i]) ]);
    }
}

void signVector(std::vector<float> in, std::vector<int>& out)
{
    out = std::vector<int>(in.size());
    for (size_t i=0; i < in.size(); ++i)
    {
        if (in[i] > 0)
        {
            out[i] = 1;
        }
        else if (in[i] < 0)
        {
            out[i] = -1;
        }
        else
        {
            out[i] = 0;
        }
    }
}

void scalarProduct(float scalar, std::vector<float> in, std::vector<float>& out)
{
    out = std::vector<float>(in.size());
    for(size_t i = 0; i < in.size(); ++i)
    {
        out[i] = scalar * in[i];
    }
}

void PeakFinder::findPeaks(QVector<float> const& in, QVector<int>& peakInds, bool includeEndpoints, float extrema)
{
    std::vector<float> x0 = in.toStdVector();
    std::vector<int> peakIndsStd;

    int minIdx = distance(x0.begin(), min_element(x0.begin(), x0.end()));
    int maxIdx = distance(x0.begin(), max_element(x0.begin(), x0.end()));

    float sel = (x0[size_t(maxIdx)] - x0[size_t(minIdx)]) / 4.0f;
    int len0 = int(x0.size());

    scalarProduct(extrema, x0, x0);

    std::vector<float> dx;
    diff(x0, dx);
    replace(dx.begin(), dx.end(), 0.0f, -PeakFinder::EPS);
    std::vector<float> dx0(dx.begin(), dx.end()-1);
    std::vector<float> dx0_1(dx.begin()+1, dx.end());
    std::vector<float> dx0_2;

    vectorElementsProduct(dx0, dx0_1, dx0_2);

    std::vector<int> ind;
    findIndicesLessThan(dx0_2, 0, ind); // Find where the derivative changes sign
    std::vector<float> x;
    float leftMin = 0.0;
    int minMagIdx = 0;
    float minMag = 0.0;

    if(includeEndpoints)
    {
        //x = [x0(1);x0(ind);x0(end)];
        selectElementsFromIndices(x0, ind, x);
        x.insert(x.begin(), x0[0]);
        x.insert(x.end(), x0[x0.size()-1]);
        //ind = [1;ind;len0];
        ind.insert(ind.begin(), 1);
        ind.insert(ind.end(), len0);
        minMagIdx = distance(x.begin(), std::min_element(x.begin(), x.end()));
        minMag = x[size_t(minMagIdx)];
        //std::cout<<"Hola"<<std::endl;
        leftMin = minMag;
    }
    else
    {
        selectElementsFromIndices(x0, ind, x);
        if(x.size()>2)
        {
            minMagIdx = distance(x.begin(), std::min_element(x.begin(), x.end()));
            minMag = x[size_t(minMagIdx)];
            leftMin = x[0]<x0[0]?x[0]:x0[0];
        }
    }

    int len = int(x.size());
    if (len > 2)
    {
        float tempMag = minMag;
        bool foundPeak = false;
        int ii;

        if (includeEndpoints)
        {
            // Deal with first point a little differently since tacked it on
            // Calculate the sign of the derivative since we tacked the first
            //  point on it does not neccessarily alternate like the rest.
            std::vector<float> xSub0(x.begin(), x.begin()+3);//tener cuidado subvector
            std::vector<float> xDiff;//tener cuidado subvector
            diff(xSub0, xDiff);

            std::vector<int> signDx;
            signVector(xDiff, signDx);

            if (signDx[0] <= 0) // The first point is larger or equal to the second
            {
                if (signDx[0] == signDx[1]) // Want alternating signs
                {
                    x.erase(x.begin()+1);
                    ind.erase(ind.begin()+1);
                    len = len-1;
                }
            }
            else // First point is smaller than the second
            {
                if (signDx[0] == signDx[1]) // Want alternating signs
                {
                    x.erase(x.begin());
                    ind.erase(ind.begin());
                    len = len-1;
                }
            }
        }

        //Skip the first point if it is smaller so we always start on maxima
        if ( x[0] >= x[1] )
        {
            ii = 0;
        }
        else
        {
            ii = 1;
        }

        //Preallocate max number of maxima
        float maxPeaks = ceilf(float(len) / 2.0f);
        std::vector<int> peakLoc(size_t(maxPeaks), 0);
        std::vector<float> peakMag(size_t(maxPeaks), 0.0f);
        int cInd = 1;
        int tempLoc = 0;

        while(ii < len)
        {
            ii = ii + 1;//This is a peak
            //Reset peak finding if we had a peak and the next peak is bigger
            //than the last or the left min was small enough to reset.
            if(foundPeak)
            {
                tempMag = minMag;
                foundPeak = false;
            }

            //Found new peak that was lager than temp mag and selectivity larger
            //than the minimum to its left.

            if( x[size_t(ii - 1)] > tempMag && x[size_t(ii - 1)] > leftMin + sel )
            {
                tempLoc = ii-1;
                tempMag = x[size_t(ii - 1)];
            }

            //Make sure we don't iterate past the length of our vector
            if(ii == len)
                break; //We assign the last point differently out of the loop

            ii = ii+1; // Move onto the valley

            //Come down at least sel from peak
            if (!foundPeak && tempMag > sel + x[size_t(ii - 1)])
            {
                foundPeak = true; //We have found a peak
                leftMin = x[size_t(ii - 1)];
                peakLoc[size_t(cInd - 1)] = tempLoc; // Add peak to index
                peakMag[size_t(cInd - 1)] = tempMag;
                cInd = cInd + 1;
            }
            else if(x[size_t(ii - 1)] < leftMin) // New left minima
                leftMin = x[size_t(ii - 1)];

        }

        // Check end point
        if (includeEndpoints)
        {
            if ( x[x.size() - 1] > tempMag && x[x.size() - 1] > leftMin + sel )
            {
                peakLoc[size_t(cInd - 1)] = len-1;
                peakMag[size_t(cInd - 1)] = x[x.size() - 1];
                cInd = cInd + 1;
            }
            else if ( !foundPeak && tempMag > minMag )// Check if we still need to add the last point
            {
                peakLoc[size_t(cInd - 1)] = tempLoc;
                peakMag[size_t(cInd - 1)] = tempMag;
                cInd = cInd + 1;
            }
        }
        else if (!foundPeak)
        {
            float minAux = x0[x0.size() - 1]<x[x.size() - 1]?x0[x0.size() - 1]:x[x.size() - 1];
            if ( x[x.size() - 1] > tempMag && x[x.size() - 1] > leftMin + sel )
            {
                peakLoc[size_t(cInd - 1)] = len - 1;
                peakMag[size_t(cInd - 1)] = x[x.size() - 1];
                cInd = cInd + 1;
            }
            else if( (!tempMag) > minAux + sel )// Check if we still need to add the last point
            {
                peakLoc[size_t(cInd - 1)] = tempLoc;
                peakMag[size_t(cInd - 1)] = tempMag;
                cInd = cInd + 1;
            }
        }

        //Create output
        if( cInd > 0 )
        {
            std::vector<int> peakLocTmp(peakLoc.begin(), peakLoc.begin()+cInd-1);
            selectElementsFromIndices(ind, peakLocTmp, peakIndsStd);
        }

    }
    //else
    //{
        //input signal length <= 2
    //}

    peakInds = QVector<int>::fromStdVector(peakIndsStd);
}
