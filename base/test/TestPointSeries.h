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

#ifndef TEST_POINT_SERIES_H
#define TEST_POINT_SERIES_H

#include "../PointSeries.h"

#include <QObject>
#include <QtTest>

#include <iostream>

using namespace std;

class TestPointSeries : public QObject
{
    Q_OBJECT

private slots:
    void empty() {

        PointSeries s;
        QCOMPARE(s.isEmpty(), true);
        QCOMPARE(s.count(), 0);

        Point p(10, QString());
        QCOMPARE(s.contains(p), false);
        QCOMPARE(s.getPointsSpanning(400), PointVector());
    }

    void singlePoint() {

        PointSeries s;
        Point p(10, QString());
        s.add(p);
        QCOMPARE(s.isEmpty(), false);
        QCOMPARE(s.count(), 1);
        QCOMPARE(s.contains(p), true);

        s.remove(p);
        QCOMPARE(s.isEmpty(), true);
        QCOMPARE(s.contains(p), false);
    }

    void singlePointSpan() {

        PointSeries s;
        Point p(10, QString());
        s.add(p);
        PointVector span;
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());
    }

    void singlePointWithDurationSpan() {

        PointSeries s;
        Point p(10, 1.0, 20, QString());
        s.add(p);
        PointVector span;
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), span);
        QCOMPARE(s.getPointsSpanning(29), span);
        QCOMPARE(s.getPointsSpanning(30), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());
    }
        
};

#endif
