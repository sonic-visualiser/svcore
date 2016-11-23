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

#ifndef TEST_COLUMN_OP_H
#define TEST_COLUMN_OP_H

#include "../ColumnOp.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class TestColumnOp : public QObject
{
    Q_OBJECT

    typedef ColumnOp C;
    typedef ColumnOp::Column Column;
    typedef vector<double> BinMapping;

#ifdef REPORT
    template <typename T>
    void report(vector<T> v) {
        cerr << "Vector is: [ ";
        for (int i = 0; i < int(v.size()); ++i) {
            if (i > 0) cerr << ", ";
            cerr << v[i];
        }
        cerr << " ]\n";
    }
#else
    template <typename T>
    void report(vector<T> ) { }
#endif
                                     
private slots:
    void applyGain() {
        QCOMPARE(C::applyGain({}, 1.0), Column());
        Column c { 1, 2, 3, -4, 5, 6 };
        Column actual(C::applyGain(c, 1.5));
        Column expected { 1.5, 3, 4.5, -6, 7.5, 9 };
        QCOMPARE(actual, expected);
        actual = C::applyGain(c, 1.0);
        QCOMPARE(actual, c);
        actual = C::applyGain(c, 0.0);
        expected = { 0, 0, 0, 0, 0, 0 };
        QCOMPARE(actual, expected);
    }

    void fftScale() {
        QCOMPARE(C::fftScale({}, 2.0), Column());
        Column c { 1, 2, 3, -4, 5 };
        Column actual(C::fftScale(c, 8));
        Column expected { 0.25, 0.5, 0.75, -1, 1.25 };
        QCOMPARE(actual, expected);
    }

    void isPeak_null() {
        QVERIFY(!C::isPeak({}, 0));
        QVERIFY(!C::isPeak({}, 1));
        QVERIFY(!C::isPeak({}, -1));
    }

    void isPeak_obvious() {
        Column c { 0.4, 0.5, 0.3 };
        QVERIFY(!C::isPeak(c, 0));
        QVERIFY(C::isPeak(c, 1));
        QVERIFY(!C::isPeak(c, 2));
    }

    void isPeak_edges() {
        Column c { 0.5, 0.4, 0.3 };
        QVERIFY(C::isPeak(c, 0));
        QVERIFY(!C::isPeak(c, 1));
        QVERIFY(!C::isPeak(c, 2));
        QVERIFY(!C::isPeak(c, 3));
        QVERIFY(!C::isPeak(c, -1));
        c = { 1.4, 1.5 };
        QVERIFY(!C::isPeak(c, 0));
        QVERIFY(C::isPeak(c, 1));
    }

    void isPeak_flat() {
        Column c { 0.0, 0.0, 0.0 };
        QVERIFY(C::isPeak(c, 0));
        QVERIFY(!C::isPeak(c, 1));
        QVERIFY(!C::isPeak(c, 2));
    }

    void isPeak_mixedSign() {
        Column c { 0.4, -0.5, -0.3, -0.6, 0.1, -0.3 };
        QVERIFY(C::isPeak(c, 0));
        QVERIFY(!C::isPeak(c, 1));
        QVERIFY(C::isPeak(c, 2));
        QVERIFY(!C::isPeak(c, 3));
        QVERIFY(C::isPeak(c, 4));
        QVERIFY(!C::isPeak(c, 5));
    }

    void isPeak_duplicate() {
        Column c({ 0.5, 0.5, 0.4, 0.4 });
        QVERIFY(C::isPeak(c, 0));
        QVERIFY(!C::isPeak(c, 1));
        QVERIFY(!C::isPeak(c, 2));
        QVERIFY(!C::isPeak(c, 3));
        c = { 0.4, 0.4, 0.5, 0.5 };
        QVERIFY(C::isPeak(c, 0)); // counterintuitive but necessary
        QVERIFY(!C::isPeak(c, 1));
        QVERIFY(C::isPeak(c, 2));
        QVERIFY(!C::isPeak(c, 3));
    }

    void peakPick() {
        QCOMPARE(C::peakPick({}), Column());
        Column c({ 0.5, 0.5, 0.4, 0.4 });
        QCOMPARE(C::peakPick(c), Column({ 0.5, 0.0, 0.0, 0.0 }));
        c = Column({ 0.4, -0.5, -0.3, -0.6, 0.1, -0.3 });
        QCOMPARE(C::peakPick(c), Column({ 0.4, 0.0, -0.3, 0.0, 0.1, 0.0 }));
    }

    void normalize_null() {
        QCOMPARE(C::normalize({}, ColumnNormalization::None), Column());
        QCOMPARE(C::normalize({}, ColumnNormalization::Sum1), Column());
        QCOMPARE(C::normalize({}, ColumnNormalization::Max1), Column());
        QCOMPARE(C::normalize({}, ColumnNormalization::Hybrid), Column());
    }

    void normalize_none() {
        Column c { 1, 2, 3, 4 };
        QCOMPARE(C::normalize(c, ColumnNormalization::None), c);
    }

    void normalize_none_mixedSign() {
        Column c { 1, 2, -3, -4 };
        QCOMPARE(C::normalize(c, ColumnNormalization::None), c);
    }

    void normalize_sum1() {
        Column c { 1, 2, 4, 3 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Sum1),
                 Column({ 0.1, 0.2, 0.4, 0.3 }));
    }

    void normalize_sum1_mixedSign() {
        Column c { 1, 2, -4, -3 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Sum1),
                 Column({ 0.1, 0.2, -0.4, -0.3 }));
    }

    void normalize_max1() {
        Column c { 4, 3, 2, 1 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Max1),
                 Column({ 1.0, 0.75, 0.5, 0.25 }));
    }

    void normalize_max1_mixedSign() {
        Column c { -4, -3, 2, 1 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Max1),
                 Column({ -1.0, -0.75, 0.5, 0.25 }));
    }

    void normalize_hybrid() {
        // with max == 99, log10(max+1) == 2 so scale factor will be 2/99
        Column c { 22, 44, 99, 66 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Hybrid),
                 Column({ 44.0/99.0, 88.0/99.0, 2.0, 132.0/99.0 }));
    }

    void normalize_hybrid_mixedSign() {
        // with max == 99, log10(max+1) == 2 so scale factor will be 2/99
        Column c { 22, 44, -99, -66 };
        QCOMPARE(C::normalize(c, ColumnNormalization::Hybrid),
                 Column({ 44.0/99.0, 88.0/99.0, -2.0, -132.0/99.0 }));
    }
    
    void distribute_simple() {
        Column in { 1, 2, 3 };
        BinMapping binfory { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5 };
        Column expected { 1, 1, 2, 2, 3, 3 };
        Column actual(C::distribute(in, 6, binfory, 0, false));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
    void distribute_simple_interpolated() {
        Column in { 1, 2, 3 };
        BinMapping binfory { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5 };
        // There is a 0.5-bin offset from the distribution you might
        // expect, because this corresponds visually to the way that
        // bin values are duplicated upwards in simple_distribution.
        // It means that switching between interpolated and
        // non-interpolated views retains the visual position of each
        // bin peak as somewhere in the middle of the scale area for
        // that bin.
        Column expected { 1, 1, 1.5, 2, 2.5, 3 };
        Column actual(C::distribute(in, 6, binfory, 0, true));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
    void distribute_nonlinear() {
        Column in { 1, 2, 3 };
        BinMapping binfory { 0.0, 0.2, 0.5, 1.0, 2.0, 2.5 };
        Column expected { 1, 1, 1, 2, 3, 3 };
        Column actual(C::distribute(in, 6, binfory, 0, false));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
    void distribute_nonlinear_interpolated() {
        // See distribute_simple_interpolated
        Column in { 1, 2, 3 };
        BinMapping binfory { 0.0, 0.2, 0.5, 1.0, 2.0, 2.5 };
        Column expected { 1, 1, 1, 1.5, 2.5, 3 };
        Column actual(C::distribute(in, 6, binfory, 0, true));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
    void distribute_shrinking() {
        Column in { 4, 1, 2, 3, 5, 6 };
        BinMapping binfory { 0.0, 2.0, 4.0 };
        Column expected { 4, 3, 6 };
        Column actual(C::distribute(in, 3, binfory, 0, false));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
    void distribute_shrinking_interpolated() {
        // should be same as distribute_shrinking, we don't
        // interpolate when resizing down
        Column in { 4, 1, 2, 3, 5, 6 };
        BinMapping binfory { 0.0, 2.0, 4.0 };
        Column expected { 4, 3, 6 };
        Column actual(C::distribute(in, 3, binfory, 0, true));
        report(actual);
        QCOMPARE(actual, expected);
    }
    
        
};
    
#endif
