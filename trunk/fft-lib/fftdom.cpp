/*================================================================================

    fftdom.cpp  -  Copyright (C) 1997 by Don Cross <dcross@intersrv.com>
                   http://www.intersrv.com/~dcross/fft.html

    This C++ source file contains a function for finding the dominant 
    frequency in an output buffer calculated by the FFT algorithm.

================================================================================*/

#include <assert.h>

// This function returns the dominant frequency in Hz.

double FindDominantFrequency (
    int numSamples,           // total number of frequency samples in buffer
    int windowSize,           // width of search window (experiment with this; try 4)
    double *real,             // real coefficient buffer
    double *imag,             // imaginary coefficient buffer
    double *ampl,             // work buffer for holding r*r+i*i values
    double samplingRate )     // sampling rate expressed in samples/second [Hz]
{
    const int n = numSamples/2;   // number of positive frequencies.

    // Pre-calculate amplitude values in the buffer provided by the caller.
    // This makes the code a lot more efficient.

    for ( int i=0; i<n; i++ )
    {
        ampl[i] = real[i]*real[i] + imag[i]*imag[i];
    }

    // Zero out amplitudes for frequencies below 20 Hz.
    int low = int ( numSamples * 20.0 / samplingRate );
    assert ( low>=0 && low<numSamples );
    for ( i=low; i>=0; i-- )
    {
        ampl[i] = 0.0;
    }

    // In the following loop, i1 is the left side of the window
    // and i2 is the right side.

    double maxWeight = -1.0;
    double centerFreq = -1.0;

    for ( int i2 = windowSize; i2 < n; i2++ )
    {
        const int i1 = i2 - windowSize;

        double moment = 0.0;
        double weight = 0.0;

        for ( int i=i1; i<=i2; i++ )
        {
            weight += ampl[i];
            moment += double(i) * ampl[i];
        }

        if ( weight > maxWeight )
        {
            maxWeight = weight;
            centerFreq = moment / weight;
        }
    }

    return samplingRate * centerFreq / double(numSamples);
}

/*--- end of fil
e fftdom.cpp ---*/
