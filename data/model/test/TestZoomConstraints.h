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

private slots:
    void unconstrainedNearest() {
        ZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1), 1);
        QCOMPARE(c.getNearestBlockSize(2), 2);
        QCOMPARE(c.getNearestBlockSize(3), 3);
        QCOMPARE(c.getNearestBlockSize(4), 4);
        QCOMPARE(c.getNearestBlockSize(20), 20);
        QCOMPARE(c.getNearestBlockSize(23), 23);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max), max);
        QCOMPARE(c.getNearestBlockSize(max+1), max);
    }
    
    void unconstrainedUp() {
        ZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundUp), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundUp), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundUp), 3);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundUp), 4);
        QCOMPARE(c.getNearestBlockSize(20, ZoomConstraint::RoundUp), 20);
        QCOMPARE(c.getNearestBlockSize(32, ZoomConstraint::RoundUp), 32);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundUp), max);
    }
    
    void unconstrainedDown() {
        ZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundDown), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundDown), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundDown), 3);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundDown), 4);
        QCOMPARE(c.getNearestBlockSize(20, ZoomConstraint::RoundDown), 20);
        QCOMPARE(c.getNearestBlockSize(32, ZoomConstraint::RoundDown), 32);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundDown), max);
    }

    void powerOfTwoNearest() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1), 1);
        QCOMPARE(c.getNearestBlockSize(2), 2);
        QCOMPARE(c.getNearestBlockSize(3), 2);
        QCOMPARE(c.getNearestBlockSize(4), 4);
        QCOMPARE(c.getNearestBlockSize(20), 16);
        QCOMPARE(c.getNearestBlockSize(23), 16);
        QCOMPARE(c.getNearestBlockSize(24), 16);
        QCOMPARE(c.getNearestBlockSize(25), 32);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max), max);
        QCOMPARE(c.getNearestBlockSize(max+1), max);
    }
    
    void powerOfTwoUp() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundUp), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundUp), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundUp), 4);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundUp), 4);
        QCOMPARE(c.getNearestBlockSize(20, ZoomConstraint::RoundUp), 32);
        QCOMPARE(c.getNearestBlockSize(32, ZoomConstraint::RoundUp), 32);
        QCOMPARE(c.getNearestBlockSize(33, ZoomConstraint::RoundUp), 64);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundUp), max);
    }
    
    void powerOfTwoDown() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundDown), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundDown), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundDown), 2);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundDown), 4);
        QCOMPARE(c.getNearestBlockSize(20, ZoomConstraint::RoundDown), 16);
        QCOMPARE(c.getNearestBlockSize(32, ZoomConstraint::RoundDown), 32);
        QCOMPARE(c.getNearestBlockSize(33, ZoomConstraint::RoundDown), 32);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundDown), max);
    }

    void powerOfSqrtTwoNearest() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1), 1);
        QCOMPARE(c.getNearestBlockSize(2), 2);
        QCOMPARE(c.getNearestBlockSize(3), 2);
        QCOMPARE(c.getNearestBlockSize(4), 4);
        QCOMPARE(c.getNearestBlockSize(18), 16);
        QCOMPARE(c.getNearestBlockSize(19), 16);
        QCOMPARE(c.getNearestBlockSize(20), 22);
        QCOMPARE(c.getNearestBlockSize(23), 22);
        QCOMPARE(c.getNearestBlockSize(28), 32);
        // PowerOfSqrtTwoZoomConstraint makes an effort to ensure
        // bigger numbers get rounded to a multiple of something
        // simple (64 or 90 depending on whether they are power-of-two
        // or power-of-sqrt-two types)
        QCOMPARE(c.getNearestBlockSize(800), 720);
        QCOMPARE(c.getNearestBlockSize(1023), 1024);
        QCOMPARE(c.getNearestBlockSize(1024), 1024);
        QCOMPARE(c.getNearestBlockSize(1025), 1024);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max), max);
        QCOMPARE(c.getNearestBlockSize(max+1), max);
    }
    
    void powerOfSqrtTwoUp() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundUp), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundUp), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundUp), 4);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundUp), 4);
        QCOMPARE(c.getNearestBlockSize(18, ZoomConstraint::RoundUp), 22);
        QCOMPARE(c.getNearestBlockSize(22, ZoomConstraint::RoundUp), 22);
        QCOMPARE(c.getNearestBlockSize(23, ZoomConstraint::RoundUp), 32);
        QCOMPARE(c.getNearestBlockSize(800, ZoomConstraint::RoundUp), 1024);
        QCOMPARE(c.getNearestBlockSize(1023, ZoomConstraint::RoundUp), 1024);
        QCOMPARE(c.getNearestBlockSize(1024, ZoomConstraint::RoundUp), 1024);
        // see comment above
        QCOMPARE(c.getNearestBlockSize(1025, ZoomConstraint::RoundUp), 1440);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundUp), max);
    }
    
    void powerOfSqrtTwoDown() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestBlockSize(1, ZoomConstraint::RoundDown), 1);
        QCOMPARE(c.getNearestBlockSize(2, ZoomConstraint::RoundDown), 2);
        QCOMPARE(c.getNearestBlockSize(3, ZoomConstraint::RoundDown), 2);
        QCOMPARE(c.getNearestBlockSize(4, ZoomConstraint::RoundDown), 4);
        QCOMPARE(c.getNearestBlockSize(18, ZoomConstraint::RoundDown), 16);
        QCOMPARE(c.getNearestBlockSize(22, ZoomConstraint::RoundDown), 22);
        QCOMPARE(c.getNearestBlockSize(23, ZoomConstraint::RoundDown), 22);
        // see comment above
        QCOMPARE(c.getNearestBlockSize(800, ZoomConstraint::RoundDown), 720);
        QCOMPARE(c.getNearestBlockSize(1023, ZoomConstraint::RoundDown), 720);
        QCOMPARE(c.getNearestBlockSize(1024, ZoomConstraint::RoundDown), 1024);
        QCOMPARE(c.getNearestBlockSize(1025, ZoomConstraint::RoundDown), 1024);
        int max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestBlockSize(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestBlockSize(max+1, ZoomConstraint::RoundDown), max);
    }
};

#endif
    
