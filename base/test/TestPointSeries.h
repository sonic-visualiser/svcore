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

    void identicalPointsSpan() {

        PointSeries s;
        Point p(10, QString());
        s.add(p);
        s.add(p);

        PointVector span;
        span.push_back(p);
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());

        s.remove(p);
        span.clear();
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());
    }
    
    void identicalPointsWithDurationSpan() {

        PointSeries s;
        Point p(10, 1.0, 20, QString());
        s.add(p);
        s.add(p);
        PointVector span;
        span.push_back(p);
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), span);
        QCOMPARE(s.getPointsSpanning(29), span);
        QCOMPARE(s.getPointsSpanning(30), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());

        s.remove(p);
        span.clear();
        span.push_back(p);
        QCOMPARE(s.getPointsSpanning(10), span);
        QCOMPARE(s.getPointsSpanning(11), span);
        QCOMPARE(s.getPointsSpanning(29), span);
        QCOMPARE(s.getPointsSpanning(30), PointVector());
        QCOMPARE(s.getPointsSpanning(9), PointVector());
    }

    void multiplePointsSpan() {

        PointSeries s;
        Point a(10, QString("a"));
        Point b(11, QString("b"));
        Point c(40, QString("c"));
        s.add(c);
        s.add(a);
        s.add(b);
        s.remove(a);
        s.add(a);
        s.add(c);
        s.remove(c);
        QCOMPARE(s.count(), 3);
        PointVector span;
        span.push_back(a);
        QCOMPARE(s.getPointsSpanning(10), span);
        span.clear();
        span.push_back(c);
        QCOMPARE(s.getPointsSpanning(40), span);
        QCOMPARE(s.getPointsSpanning(9), PointVector());
    }

    void disjointPointsWithDurationSpan() {

        PointSeries s;
        Point a(10, 1.0f, 20, QString("a"));
        Point b(100, 1.2f, 30, QString("b"));
        s.add(a);
        s.add(b);
        QCOMPARE(s.getPointsSpanning(0), PointVector());
        QCOMPARE(s.getPointsSpanning(10), PointVector({ a }));
        QCOMPARE(s.getPointsSpanning(15), PointVector({ a }));
        QCOMPARE(s.getPointsSpanning(30), PointVector());
        QCOMPARE(s.getPointsSpanning(99), PointVector());
        QCOMPARE(s.getPointsSpanning(100), PointVector({ b }));
        QCOMPARE(s.getPointsSpanning(120), PointVector({ b }));
        QCOMPARE(s.getPointsSpanning(130), PointVector());
    }
    
    void overlappingPointsWithAndWithoutDurationSpan() {

        PointSeries s;
        Point p(20, QString("p"));
        Point a(10, 1.0, 20, QString("a"));
        s.add(p);
        s.add(a);
        PointVector span;
        span.push_back(a);
        QCOMPARE(s.getPointsSpanning(15), span);
        QCOMPARE(s.getPointsSpanning(25), span);
        span.clear();
        span.push_back(p);
        span.push_back(a);
        QCOMPARE(s.getPointsSpanning(20), span);
    }

    void overlappingPointsWithDurationSpan() {

        PointSeries s;
        Point a(20, 1.0, 10, QString("a"));
        Point b(10, 1.0, 20, QString("b"));
        Point c(10, 1.0, 40, QString("c"));
        s.add(a);
        s.add(b);
        s.add(c);
        QCOMPARE(s.getPointsSpanning(10), PointVector({ b, c }));
        QCOMPARE(s.getPointsSpanning(20), PointVector({ b, c, a }));
        QCOMPARE(s.getPointsSpanning(25), PointVector({ b, c, a }));
        QCOMPARE(s.getPointsSpanning(30), PointVector({ c }));
        QCOMPARE(s.getPointsSpanning(40), PointVector({ c }));
        QCOMPARE(s.getPointsSpanning(50), PointVector());
    }

    void pointPatternSpan() {

        PointSeries s;
        Point a(0, 1.0, 18, QString("a"));
        Point b(3, 2.0, 6, QString("b"));
        Point c(5, 3.0, 2, QString("c"));
        Point d(6, 4.0, 10, QString("d"));
        Point e(14, 5.0, 3, QString("e"));
        s.add(b);
        s.add(c);
        s.add(d);
        s.add(a);
        s.add(e);
        QCOMPARE(s.getPointsSpanning(8), PointVector({ a, b, d }));
    }
};

#endif
