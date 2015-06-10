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

using namespace std;

class TestFFTModel : public QObject
{
    Q_OBJECT

private slots:

    void example() {
	MockWaveModel mwm({ DC }, 16);
	FFTModel fftm(&mwm, 0, RectangularWindow, 8, 8, 8, false);
	float reals[6], imags[6];
	reals[5] = 999.f; // overrun guards
	imags[5] = 999.f;
	fftm.getValuesAt(0, reals, imags);
	cerr << "reals: " << reals[0] << "," << reals[1] << "," << reals[2] << "," << reals[3] << "," << reals[4] <<  endl;
	cerr << "imags: " << imags[0] << "," << imags[1] << "," << imags[2] << "," << imags[3] << "," << imags[4] <<  endl;
	QCOMPARE(reals[0], 4.f); // rectangular window scales by 0.5
	QCOMPARE(reals[1], 0.f);
	QCOMPARE(reals[2], 0.f);
	QCOMPARE(reals[3], 0.f);
	QCOMPARE(reals[4], 0.f);
	QCOMPARE(reals[5], 999.f);
	QCOMPARE(imags[5], 999.f);
	imags[5] = 0.f;
	COMPARE_ALL_TO_F(imags, 0.f);
    }
    
};

#endif
