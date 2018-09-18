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
        // well, this shows how horrible this api is
        ZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1)), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2)), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3)), ZoomLevel(ZoomLevel::FramesPerPixel, 3));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4)), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20)), ZoomLevel(ZoomLevel::FramesPerPixel, 20));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 23)), ZoomLevel(ZoomLevel::FramesPerPixel, 23));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1)), max);
    }
    
    void unconstrainedUp() {
        ZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 3));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 20));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 32), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundUp), max);
    }
    
    void unconstrainedDown() {
        ZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 3));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 20));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 32), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundDown), max);
    }

    void powerOfTwoNearest() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1)), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2)), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3)), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4)), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20)), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 23)), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 24)), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 25)), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1)), max);
    }
    
    void powerOfTwoUp() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 32), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 33), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 64));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundUp), max);
    }
    
    void powerOfTwoDown() {
        PowerOfTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 32), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 33), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundDown), max);
    }

    void powerOfSqrtTwoNearest() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1)), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2)), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3)), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4)), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 18)), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 19)), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 20)), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 23)), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 28)), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        // PowerOfSqrtTwoZoomConstraint makes an effort to ensure
        // bigger numbers get rounded to a multiple of something
        // simple (64 or 90 depending on whether they are power-of-two
        // or power-of-sqrt-two types)
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 800)), ZoomLevel(ZoomLevel::FramesPerPixel, 720));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1023)), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1024)), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1025)), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1)), max);
    }
    
    void powerOfSqrtTwoUp() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 18), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 22), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 23), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 32));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 800), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1023), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1024), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        // see comment above
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1025), ZoomConstraint::RoundUp), ZoomLevel(ZoomLevel::FramesPerPixel, 1440));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundUp), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundUp), max);
    }
    
    void powerOfSqrtTwoDown() {
        PowerOfSqrtTwoZoomConstraint c;
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 1));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 2), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 3), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 2));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 4), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 4));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 18), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 16));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 22), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 23), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 22));
        // see comment above
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 800), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 720));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1023), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 720));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1024), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, 1025), ZoomConstraint::RoundDown), ZoomLevel(ZoomLevel::FramesPerPixel, 1024));
        auto max = c.getMaxZoomLevel();
        QCOMPARE(c.getNearestZoomLevel(max, ZoomConstraint::RoundDown), max);
        QCOMPARE(c.getNearestZoomLevel(ZoomLevel(ZoomLevel::FramesPerPixel, max.level + 1), ZoomConstraint::RoundDown), max);
    }
};

#endif
    
