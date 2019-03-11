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

#ifndef STRESS_EVENT_SERIES_H
#define STRESS_EVENT_SERIES_H

#include "../EventSeries.h"

#include <QObject>
#include <QtTest>

#include <iostream>

using namespace std;

class StressEventSeries : public QObject
{
    Q_OBJECT

private:
    void report(int n, QString sort, clock_t start, clock_t end) {
        QString message = QString("Time for %1 %2 events = ").arg(n).arg(sort);
        cerr << "                 " << message;
        for (int i = 0; i < 34 - message.size(); ++i) cerr << " ";
        cerr << double(end - start) * 1000.0 / double(CLOCKS_PER_SEC)
             << "ms" << endl;
    }        
    
    void short_n(int n) {
        clock_t start = clock();
        EventSeries s;
        for (int i = 0; i < n; ++i) {
            float value = float(rand()) / float(RAND_MAX);
            Event e(rand(), value, 1000, QString("event %1").arg(i));
            s.add(e);
        }
        QCOMPARE(s.count(), n);
        clock_t end = clock();
        report(n, "short", start, end);
    }

    void longish_n(int n) {
        clock_t start = clock();
        EventSeries s;
        for (int i = 0; i < n; ++i) {
            float value = float(rand()) / float(RAND_MAX);
            Event e(rand(), value, rand() / 1000, QString("event %1").arg(i));
            s.add(e);
        }
        QCOMPARE(s.count(), n);
        clock_t end = clock();
        report(n, "longish", start, end);
    }

private slots:
    void short_3() { short_n(1000); }
    void short_4() { short_n(10000); }
    void short_5() { short_n(100000); }
    void short_6() { short_n(1000000); }
    void longish_3() { longish_n(1000); }
    void longish_4() { longish_n(10000); }
    void longish_5() { longish_n(100000); }

    /*

(T540p, Core i5-4330M @ 2.80GHz, 16G)

cf5196881e3e:

                 Time for 1000 short events =      1.169ms
                 Time for 10000 short events =     20.566ms
                 Time for 100000 short events =    279.242ms
                 Time for 1000000 short events =   3925.06ms
                 Time for 1000 longish events =    1.938ms
                 Time for 10000 longish events =   72.209ms
                 Time for 100000 longish events =  6469.26ms

Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 12785ms

13.40user 0.37system 0:13.84elapsed 99%CPU (0avgtext+0avgdata 1052000maxresident)k
0inputs+40outputs (0major+260249minor)pagefaults 0swaps


dcd510bd89db:

                 Time for 1000 short events =      1.824ms
                 Time for 10000 short events =     19.203ms
                 Time for 100000 short events =    270.631ms
                 Time for 1000000 short events =   4425.2ms
                 Time for 1000 longish events =    2.395ms
                 Time for 10000 longish events =   83.623ms
                 Time for 100000 longish events =  5958.28ms

Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 13116ms

13.64user 0.26system 0:13.98elapsed 99%CPU (0avgtext+0avgdata 948104maxresident)k
0inputs+40outputs (0major+234387minor)pagefaults 0swaps

895186c43fce:

                 Time for 1000 short events =      1.706ms
                 Time for 10000 short events =     23.192ms
                 Time for 100000 short events =    310.605ms
                 Time for 1000000 short events =   4675.7ms
                 Time for 1000 longish events =    2.186ms
                 Time for 10000 longish events =   760.659ms
                 Time for 100000 longish events =  1335.57ms

Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 7804ms

7.97user 0.29system 0:08.31elapsed 99%CPU (0avgtext+0avgdata 706388maxresident)k
0inputs+40outputs (0major+182225minor)pagefaults 0swaps

1c21ddac220e (with simpler code):

                 Time for 1000 short events =      1.12ms
                 Time for 10000 short events =     14.997ms
                 Time for 100000 short events =    238.818ms
                 Time for 1000000 short events =   3765.09ms
                 Time for 1000 longish events =    1.657ms
                 Time for 10000 longish events =   1130.59ms
                 Time for 100000 longish events =  1840.98ms

Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 8081ms

7.88user 0.23system 0:08.19elapsed 99%CPU (0avgtext+0avgdata 781688maxresident)k
0inputs+40outputs (0major+200425minor)pagefaults 0swaps

    */
};

#endif
