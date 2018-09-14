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

#ifndef TEST_CSV_FORMAT_H
#define TEST_CSV_FORMAT_H

// Tests for the code that guesses the most likely format for parsing a CSV file

#include "../CSVFormat.h"

#include "base/Debug.h"

#include <cmath>

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class CSVFormatTest : public QObject
{
    Q_OBJECT

private:
    QDir csvDir;

public:
    CSVFormatTest(QString base) {
        if (base == "") {
            base = "svcore/data/fileio/test";
        }
        csvDir = QDir(base + "/csv");
    }

private slots:
    void init() {
        if (!csvDir.exists()) {
            SVCERR << "ERROR: CSV test file directory \"" << csvDir.absolutePath() << "\" does not exist" << endl;
            QVERIFY2(csvDir.exists(), "CSV test file directory not found");
        }
    }

    void separatorComma() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("separator-comma.csv")));
        QCOMPARE(f.getSeparator(), QChar(','));
        QCOMPARE(f.getColumnCount(), 3);
    }
    
    void separatorTab() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("separator-tab.csv")));
        QCOMPARE(f.getSeparator(), QChar('\t'));
        QCOMPARE(f.getColumnCount(), 3);
    }
    
    void separatorPipe() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("separator-pipe.csv")));
        QCOMPARE(f.getSeparator(), QChar('|'));
        // differs from the others
        QCOMPARE(f.getColumnCount(), 4);
    }
    
    void separatorSpace() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("separator-space.csv")));
        QCOMPARE(f.getSeparator(), QChar(' '));
        // NB fields are separated by 1 or more spaces, not necessarily exactly 1
        QCOMPARE(f.getColumnCount(), 3);
    }
    
    void separatorColon() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("separator-colon.csv")));
        QCOMPARE(f.getSeparator(), QChar(':'));
        QCOMPARE(f.getColumnCount(), 3);
    }
    
    void comment() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("comment.csv")));
        QCOMPARE(f.getSeparator(), QChar(','));
        QCOMPARE(f.getColumnCount(), 4);
    }

    void qualities() {
        CSVFormat f;
        QVERIFY(f.guessFormatFor(csvDir.filePath("column-qualities.csv")));
        QCOMPARE(f.getSeparator(), QChar(','));
        QCOMPARE(f.getColumnCount(), 7);
        QList<CSVFormat::ColumnQualities> q = f.getColumnQualities();
        QList<CSVFormat::ColumnQualities> expected;
        expected << 0;
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric |
                                               CSVFormat::ColumnIntegral |
                                               CSVFormat::ColumnIncreasing);
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric |
                                               CSVFormat::ColumnIntegral |
                                               CSVFormat::ColumnIncreasing |
                                               CSVFormat::ColumnLarge);
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric);
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric |
                                               CSVFormat::ColumnIncreasing);
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric |
                                               CSVFormat::ColumnSmall |
                                               CSVFormat::ColumnSigned);
        expected << CSVFormat::ColumnQualities(CSVFormat::ColumnNumeric |
                                               CSVFormat::ColumnIntegral |
                                               CSVFormat::ColumnIncreasing |
                                               CSVFormat::ColumnNearEmpty);
        QCOMPARE(q, expected);
    }
};

#endif
