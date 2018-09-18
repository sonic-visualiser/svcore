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

#ifndef TEST_ZOOM_CONSTRAINTS_H
#define TEST_ZOOM_CONSTRAINTS_H

#include "../PowerOfTwoZoomConstraint.h"
#include "../PowerOfSqrtTwoZoomConstraint.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class TestZoomConstraints : public QObject
{
    Q_OBJECT

    void checkFpp(const ZoomConstraint &c,
                  ZoomConstraint::RoundingDirection dir,
                  int n,
                  int expected) {
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, n),
                                       dir),
                 ZoomLevel(ZoomLevel::FramesPerPixel, expected));
    }
    
private slots:
    void unconstrainedNearest() {
        ZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundNearest, 1, 1);
        checkFpp(c, ZoomConstraint::RoundNearest, 2, 2);
        checkFpp(c, ZoomConstraint::RoundNearest, 3, 3);
        checkFpp(c, ZoomConstraint::RoundNearest, 4, 4);
        checkFpp(c, ZoomConstraint::RoundNearest, 20, 20);
        checkFpp(c, ZoomConstraint::RoundNearest, 32, 32);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented()), max);
    }
    
    void unconstrainedUp() {
        ZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundUp, 1, 1);
        checkFpp(c, ZoomConstraint::RoundUp, 2, 2);
        checkFpp(c, ZoomConstraint::RoundUp, 3, 3);
        checkFpp(c, ZoomConstraint::RoundUp, 4, 4);
        checkFpp(c, ZoomConstraint::RoundUp, 20, 20);
        checkFpp(c, ZoomConstraint::RoundUp, 32, 32);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundUp), max);
    }
    
    void unconstrainedDown() {
        ZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundDown, 1, 1);
        checkFpp(c, ZoomConstraint::RoundDown, 2, 2);
        checkFpp(c, ZoomConstraint::RoundDown, 3, 3);
        checkFpp(c, ZoomConstraint::RoundDown, 4, 4);
        checkFpp(c, ZoomConstraint::RoundDown, 20, 20);
        checkFpp(c, ZoomConstraint::RoundDown, 32, 32);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundDown), max);
    }

    void powerOfTwoNearest() {
        PowerOfTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundNearest, 1, 1);
        checkFpp(c, ZoomConstraint::RoundNearest, 2, 2);
        checkFpp(c, ZoomConstraint::RoundNearest, 3, 2);
        checkFpp(c, ZoomConstraint::RoundNearest, 4, 4);
        checkFpp(c, ZoomConstraint::RoundNearest, 20, 16);
        checkFpp(c, ZoomConstraint::RoundNearest, 23, 16);
        checkFpp(c, ZoomConstraint::RoundNearest, 24, 16);
        checkFpp(c, ZoomConstraint::RoundNearest, 25, 32);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented()), max);
    }
    
    void powerOfTwoUp() {
        PowerOfTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundUp, 1, 1);
        checkFpp(c, ZoomConstraint::RoundUp, 2, 2);
        checkFpp(c, ZoomConstraint::RoundUp, 3, 4);
        checkFpp(c, ZoomConstraint::RoundUp, 4, 4);
        checkFpp(c, ZoomConstraint::RoundUp, 20, 32);
        checkFpp(c, ZoomConstraint::RoundUp, 32, 32);
        checkFpp(c, ZoomConstraint::RoundUp, 33, 64);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundUp), max);
    }
    
    void powerOfTwoDown() {
        PowerOfTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundDown, 1, 1);
        checkFpp(c, ZoomConstraint::RoundDown, 2, 2);
        checkFpp(c, ZoomConstraint::RoundDown, 3, 2);
        checkFpp(c, ZoomConstraint::RoundDown, 4, 4);
        checkFpp(c, ZoomConstraint::RoundDown, 20, 16);
        checkFpp(c, ZoomConstraint::RoundDown, 32, 32);
        checkFpp(c, ZoomConstraint::RoundDown, 33, 32);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundDown), max);
    }

    void powerOfSqrtTwoNearest() {
        PowerOfSqrtTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundNearest, 1, 1);
        checkFpp(c, ZoomConstraint::RoundNearest, 2, 2);
        checkFpp(c, ZoomConstraint::RoundNearest, 3, 2);
        checkFpp(c, ZoomConstraint::RoundNearest, 4, 4);
        checkFpp(c, ZoomConstraint::RoundNearest, 18, 16);
        checkFpp(c, ZoomConstraint::RoundNearest, 19, 16);
        checkFpp(c, ZoomConstraint::RoundNearest, 20, 22);
        checkFpp(c, ZoomConstraint::RoundNearest, 23, 22);
        checkFpp(c, ZoomConstraint::RoundNearest, 28, 32);
        // PowerOfSqrtTwoZoomConstraint makes an effort to ensure
        // bigger numbers get rounded to a multiple of something
        // simple (64 or 90 depending on whether they are power-of-two
        // or power-of-sqrt-two types)
        checkFpp(c, ZoomConstraint::RoundNearest, 800, 720);
        checkFpp(c, ZoomConstraint::RoundNearest, 1023, 1024);
        checkFpp(c, ZoomConstraint::RoundNearest, 1024, 1024);
        checkFpp(c, ZoomConstraint::RoundNearest, 1024, 1024);
        checkFpp(c, ZoomConstraint::RoundNearest, 1025, 1024);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented()), max);
    }
    
    void powerOfSqrtTwoUp() {
        PowerOfSqrtTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundUp, 1, 1);
        checkFpp(c, ZoomConstraint::RoundUp, 2, 2);
        checkFpp(c, ZoomConstraint::RoundUp, 3, 4);
        checkFpp(c, ZoomConstraint::RoundUp, 4, 4);
        checkFpp(c, ZoomConstraint::RoundUp, 18, 22);
        checkFpp(c, ZoomConstraint::RoundUp, 22, 22);
        checkFpp(c, ZoomConstraint::RoundUp, 23, 32);
        checkFpp(c, ZoomConstraint::RoundUp, 800, 1024);
        checkFpp(c, ZoomConstraint::RoundUp, 1023, 1024);
        checkFpp(c, ZoomConstraint::RoundUp, 1024, 1024);
        // see comment above
        checkFpp(c, ZoomConstraint::RoundUp, 1025, 1440);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundUp), max);
    }
    
    void powerOfSqrtTwoDown() {
        PowerOfSqrtTwoZoomConstraint c;
        checkFpp(c, ZoomConstraint::RoundDown, 1, 1);
        checkFpp(c, ZoomConstraint::RoundDown, 2, 2);
        checkFpp(c, ZoomConstraint::RoundDown, 3, 2);
        checkFpp(c, ZoomConstraint::RoundDown, 4, 4);
        checkFpp(c, ZoomConstraint::RoundDown, 18, 16);
        checkFpp(c, ZoomConstraint::RoundDown, 22, 22);
        checkFpp(c, ZoomConstraint::RoundDown, 23, 22);
        // see comment above
        checkFpp(c, ZoomConstraint::RoundDown, 800, 720);
        checkFpp(c, ZoomConstraint::RoundDown, 1023, 720);
        checkFpp(c, ZoomConstraint::RoundDown, 1024, 1024);
        checkFpp(c, ZoomConstraint::RoundDown, 1025, 1024);
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max,
                                       ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(max.incremented(),
                                       ZoomConstraint::RoundDown), max);
    }
};

#endif
    
