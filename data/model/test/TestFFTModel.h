/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TEST_FFT_MODEL_H
#define TEST_FFT_MODEL_H

#include "../FFTModel.h"

#include "MockWaveModel.h"

#include "Compares.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>
#include <complex>

using namespace std;

class TestFFTModel : public QObject
{
    Q_OBJECT

private:
    void test(DenseTimeValueModel *model,
              WindowType window, int windowSize, int windowIncrement, int fftSize,
              int columnNo, vector<vector<complex<float>>> expectedValues,
              int expectedWidth) {
        for (int ch = 0; in_range_for(expectedValues, ch); ++ch) {
            for (int polar = 0; polar <= 1; ++polar) {
                FFTModel fftm(model, ch, window, windowSize, windowIncrement,
                              fftSize, bool(polar));
                QCOMPARE(fftm.getWidth(), expectedWidth);
                int hs1 = fftSize/2 + 1;
                QCOMPARE(fftm.getHeight(), hs1);
                vector<float> reals(hs1 + 1, 0.f);
                vector<float> imags(hs1 + 1, 0.f);
                reals[hs1] = 999.f; // overrun guards
                imags[hs1] = 999.f;
                fftm.getValuesAt(columnNo, &reals[0], &imags[0]);
                for (int i = 0; i < hs1; ++i) {
                    float eRe = expectedValues[ch][i].real();
                    float eIm = expectedValues[ch][i].imag();
                    if (reals[i] != eRe || imags[i] != eIm) {
                        cerr << "ERROR: output is not as expected for column "
                             << i << " in channel " << ch << " (polar store = "
                             << polar << ")" << endl;
                        cerr << "expected : ";
                        for (int j = 0; j < hs1; ++j) {
                            cerr << expectedValues[ch][j] << " ";
                        }
                        cerr << "\nactual   : ";
                        for (int j = 0; j < hs1; ++j) {
                            cerr << complex<float>(reals[j], imags[j]) << " ";
                        }
                        cerr << endl;
                    }
                    QCOMPARE(reals[i], eRe);
                    QCOMPARE(imags[i], eIm);
                }
                QCOMPARE(reals[hs1], 999.f);
                QCOMPARE(imags[hs1], 999.f);
            }
        }
    }

private slots:

    // NB. FFTModel columns are centred on the sample frame, and in
    // particular this means column 0 is centred at sample 0 (i.e. it
    // contains only half the window-size worth of real samples, the
    // others are 0-valued from before the origin).  Generally in
    // these tests we are padding our signal with half a window of
    // zeros, in order that the result for column 0 is all zeros
    // (rather than something with a step in it that is harder to
    // reason about the FFT of) and the results for subsequent columns
    // are those of our expected signal.
    
    void dc_simple_rect() {
	MockWaveModel mwm({ DC }, 16, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 1,
             { { { 4.f, 0.f }, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 2,
             { { { 4.f, 0.f }, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }

    void dc_simple_hann() {
        // The Hann window function is a simple sinusoid with period
        // equal to twice the window size, and it halves the DC energy
	MockWaveModel mwm({ DC }, 16, 4);
        test(&mwm, HanningWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, HanningWindow, 8, 8, 8, 1,
             { { { 4.f, 0.f }, { 2.f, 0.f }, {}, {}, {} } }, 4);
        test(&mwm, HanningWindow, 8, 8, 8, 2,
             { { { 4.f, 0.f }, { 2.f, 0.f }, {}, {}, {} } }, 4);
        test(&mwm, HanningWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }
    
    void sine_simple_rect() {
	MockWaveModel mwm({ Sine }, 16, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 1,
             { { {}, { 0.f, 2.f }, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 2,
             { { {}, { 0.f, 2.f }, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }
    
    void cosine_simple_rect() {
	MockWaveModel mwm({ Cosine }, 16, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 1,
             { { {}, { 2.f, 0.f }, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 2,
             { { {}, { 2.f, 0.f }, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }
    
    void nyquist_simple_rect() {
	MockWaveModel mwm({ Nyquist }, 16, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 1,
             { { {}, {}, {}, {}, { 2.f, 0.f } } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 2,
             { { {}, {}, {}, {}, { 2.f, 0.f } } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }
    
    void dirac_simple_rect() {
	MockWaveModel mwm({ Dirac }, 16, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 0,
             { { {}, {}, {}, {}, {} } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 1,
             { { { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f } } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 2,
             { { { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f } } }, 4);
        test(&mwm, RectangularWindow, 8, 8, 8, 3,
             { { {}, {}, {}, {}, {} } }, 4);
    }

};

#endif
