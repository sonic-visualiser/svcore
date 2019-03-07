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

#ifndef TEST_EVENT_SERIES_H
#define TEST_EVENT_SERIES_H

#include "../EventSeries.h"

#include <QObject>
#include <QtTest>

#include <iostream>

using namespace std;

class TestEventSeries : public QObject
{
    Q_OBJECT

private slots:
    void empty() {

        EventSeries s;
        QCOMPARE(s.isEmpty(), true);
        QCOMPARE(s.count(), 0);

        Event p(10, QString());
        QCOMPARE(s.contains(p), false);
        QCOMPARE(s.getEventsSpanning(400), EventVector());
    }

    void singleEvent() {

        EventSeries s;
        Event p(10, QString());
        s.add(p);
        QCOMPARE(s.isEmpty(), false);
        QCOMPARE(s.count(), 1);
        QCOMPARE(s.contains(p), true);

        s.remove(p);
        QCOMPARE(s.isEmpty(), true);
        QCOMPARE(s.contains(p), false);
    }

    void singleEventSpan() {

        EventSeries s;
        Event p(10, QString());
        s.add(p);
        EventVector span;
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());
    }

    void singleEventWithDurationSpan() {

        EventSeries s;
        Event p(10, 1.0, 20, QString());
        s.add(p);
        EventVector span;
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), span);
        QCOMPARE(s.getEventsSpanning(29), span);
        QCOMPARE(s.getEventsSpanning(30), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());
    }

    void identicalEventsSpan() {

        EventSeries s;
        Event p(10, QString());
        s.add(p);
        s.add(p);

        EventVector span;
        span.push_back(p);
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());

        s.remove(p);
        span.clear();
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());
    }
    
    void identicalEventsWithDurationSpan() {

        EventSeries s;
        Event p(10, 1.0, 20, QString());
        s.add(p);
        s.add(p);
        EventVector span;
        span.push_back(p);
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), span);
        QCOMPARE(s.getEventsSpanning(29), span);
        QCOMPARE(s.getEventsSpanning(30), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());

        s.remove(p);
        span.clear();
        span.push_back(p);
        QCOMPARE(s.getEventsSpanning(10), span);
        QCOMPARE(s.getEventsSpanning(11), span);
        QCOMPARE(s.getEventsSpanning(29), span);
        QCOMPARE(s.getEventsSpanning(30), EventVector());
        QCOMPARE(s.getEventsSpanning(9), EventVector());
    }

    void multipleEventsSpan() {

        EventSeries s;
        Event a(10, QString("a"));
        Event b(11, QString("b"));
        Event c(40, QString("c"));
        s.add(c);
        s.add(a);
        s.add(b);
        s.remove(a);
        s.add(a);
        s.add(c);
        s.remove(c);
        QCOMPARE(s.count(), 3);
        EventVector span;
        span.push_back(a);
        QCOMPARE(s.getEventsSpanning(10), span);
        span.clear();
        span.push_back(c);
        QCOMPARE(s.getEventsSpanning(40), span);
        QCOMPARE(s.getEventsSpanning(9), EventVector());
    }

    void disjointEventsWithDurationSpan() {

        EventSeries s;
        Event a(10, 1.0f, 20, QString("a"));
        Event b(100, 1.2f, 30, QString("b"));
        s.add(a);
        s.add(b);
        QCOMPARE(s.getEventsSpanning(0), EventVector());
        QCOMPARE(s.getEventsSpanning(10), EventVector({ a }));
        QCOMPARE(s.getEventsSpanning(15), EventVector({ a }));
        QCOMPARE(s.getEventsSpanning(30), EventVector());
        QCOMPARE(s.getEventsSpanning(99), EventVector());
        QCOMPARE(s.getEventsSpanning(100), EventVector({ b }));
        QCOMPARE(s.getEventsSpanning(120), EventVector({ b }));
        QCOMPARE(s.getEventsSpanning(130), EventVector());
    }
    
    void overlappingEventsWithAndWithoutDurationSpan() {

        EventSeries s;
        Event p(20, QString("p"));
        Event a(10, 1.0, 20, QString("a"));
        s.add(p);
        s.add(a);
        EventVector span;
        span.push_back(a);
        QCOMPARE(s.getEventsSpanning(15), span);
        QCOMPARE(s.getEventsSpanning(25), span);
        span.clear();
        span.push_back(p);
        span.push_back(a);
        QCOMPARE(s.getEventsSpanning(20), span);
    }

    void overlappingEventsWithDurationSpan() {

        EventSeries s;
        Event a(20, 1.0, 10, QString("a"));
        Event b(10, 1.0, 20, QString("b"));
        Event c(10, 1.0, 40, QString("c"));
        s.add(a);
        s.add(b);
        s.add(c);
        QCOMPARE(s.getEventsSpanning(10), EventVector({ b, c }));
        QCOMPARE(s.getEventsSpanning(20), EventVector({ b, c, a }));
        QCOMPARE(s.getEventsSpanning(25), EventVector({ b, c, a }));
        QCOMPARE(s.getEventsSpanning(30), EventVector({ c }));
        QCOMPARE(s.getEventsSpanning(40), EventVector({ c }));
        QCOMPARE(s.getEventsSpanning(50), EventVector());
    }

    void eventPatternSpan() {

        EventSeries s;
        Event a(0, 1.0, 18, QString("a"));
        Event b(3, 2.0, 6, QString("b"));
        Event c(5, 3.0, 2, QString("c"));
        Event d(6, 4.0, 10, QString("d"));
        Event e(14, 5.0, 3, QString("e"));
        s.add(b);
        s.add(c);
        s.add(d);
        s.add(a);
        s.add(e);
        QCOMPARE(s.getEventsSpanning(8), EventVector({ a, b, d }));
    }
};

#endif
