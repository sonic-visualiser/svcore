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

#ifndef TEST_SPARSE_MODELS_H
#define TEST_SPARSE_MODELS_H

#include "../SparseOneDimensionalModel.h"
#include "../NoteModel.h"

#include <QObject>
#include <QtTest>

#include <iostream>

using namespace std;

class TestSparseModels : public QObject
{
    Q_OBJECT

private slots:
    void s1d_empty() {
        SparseOneDimensionalModel m(100, 10, false);
        QCOMPARE(m.isEmpty(), true);
        QCOMPARE(m.getPointCount(), 0);
        QCOMPARE(m.getPoints().begin(), m.getPoints().end());
        QCOMPARE(m.getStartFrame(), 0);
        QCOMPARE(m.getEndFrame(), 0);
        QCOMPARE(m.getSampleRate(), 100);
        QCOMPARE(m.getResolution(), 10);
        QCOMPARE(m.isSparse(), true);

        SparseOneDimensionalModel::Point p(10);
        m.addPoint(p);
        m.clear();
        QCOMPARE(m.isEmpty(), true);
        QCOMPARE(m.getPointCount(), 0);
        QCOMPARE(m.getPoints().begin(), m.getPoints().end());
        QCOMPARE(m.getStartFrame(), 0);
        QCOMPARE(m.getEndFrame(), 0);

        m.addPoint(p);
        m.deletePoint(p);
        QCOMPARE(m.isEmpty(), true);
        QCOMPARE(m.getPointCount(), 0);
        QCOMPARE(m.getPoints().begin(), m.getPoints().end());
        QCOMPARE(m.getStartFrame(), 0);
        QCOMPARE(m.getEndFrame(), 0);
    }

    void s1d_extents() {
        SparseOneDimensionalModel m(100, 10, false);
        SparseOneDimensionalModel::Point p1(20);
        m.addPoint(p1);
        QCOMPARE(m.isEmpty(), false);
        QCOMPARE(m.getPointCount(), 1);
        SparseOneDimensionalModel::Point p2(50);
        m.addPoint(p2);
        QCOMPARE(m.isEmpty(), false);
        QCOMPARE(m.getPointCount(), 2);
        QCOMPARE(m.getPoints().size(), 2);
        QCOMPARE(*m.getPoints().begin(), p1);
        QCOMPARE(*m.getPoints().rbegin(), p2);
        QCOMPARE(m.getStartFrame(), 20);
        QCOMPARE(m.getEndFrame(), 60);
        QCOMPARE(m.containsPoint(p1), true);
        m.deletePoint(p1);
        QCOMPARE(m.getPointCount(), 1);
        QCOMPARE(m.getPoints().size(), 1);
        QCOMPARE(*m.getPoints().begin(), p2);
        QCOMPARE(m.getStartFrame(), 50);
        QCOMPARE(m.getEndFrame(), 60);
        QCOMPARE(m.containsPoint(p1), false);
    }
             
    void s1d_sample() {
        SparseOneDimensionalModel m(100, 10, false);
        SparseOneDimensionalModel::Point p1(20), p2(20), p3(50);
        m.addPoint(p1);
        m.addPoint(p2);
        m.addPoint(p3);
        QCOMPARE(m.getPoints().size(), 3);
        QCOMPARE(*m.getPoints().begin(), p1);
        QCOMPARE(*m.getPoints().rbegin(), p3);

        auto pp = m.getPoints(20, 30);
        QCOMPARE(pp.size(), 2);
        QCOMPARE(*pp.begin(), p1);
        QCOMPARE(*pp.rbegin(), p2);

        pp = m.getPoints(40, 50);
        QCOMPARE(pp.size(), 0);

        pp = m.getPoints(50, 50);
        QCOMPARE(pp.size(), 1);
        QCOMPARE(*pp.begin(), p3);
    }

    void s1d_xml() {
        SparseOneDimensionalModel m(100, 10, false);
        m.setObjectName("This \"&\" that");
        SparseOneDimensionalModel::Point p1(20), p2(20), p3(50);
        p2.label = "Label &'\">";
        m.addPoint(p1);
        m.addPoint(p2);
        m.addPoint(p3);
        QString xml;
        QTextStream str(&xml, QIODevice::WriteOnly);
        m.toXml(str);
        str.flush();
        QString expected =
            "<model id='1' name='This &quot;&amp;&quot; that' sampleRate='100' start='20' end='60' type='sparse' dimensions='1' resolution='10' notifyOnAdd='false' dataset='0' />\n"
            "<dataset id='0' dimensions='1'>\n"
            "  <point frame='20' label='' />\n"
            "  <point frame='20' label='Label &amp;&apos;&quot;&gt;' />\n"
            "  <point frame='50' label='' />\n"
            "</dataset>\n";
        expected.replace("\'", "\"");
        if (xml != expected) {
            cerr << "Obtained xml:\n" << xml
                 << "\nExpected:\n" << expected << endl;
        }
        QCOMPARE(xml, expected);
    }

    void note_extents() {
        NoteModel m(100, 10, false);
        NoteModel::Point p1(20, 123.4, 40, 0.8, "note 1");
        m.addPoint(p1);
        QCOMPARE(m.isEmpty(), false);
        QCOMPARE(m.getPointCount(), 1);
        NoteModel::Point p2(50, 124.3, 30, 0.9, "note 2");
        m.addPoint(p2);
        QCOMPARE(m.isEmpty(), false);
        QCOMPARE(m.getPointCount(), 2);
        QCOMPARE(m.getPoints().size(), 2);
        QCOMPARE(*m.getPoints().begin(), p1);
        QCOMPARE(*m.getPoints().rbegin(), p2);
        QCOMPARE(m.getStartFrame(), 20);
        QCOMPARE(m.getEndFrame(), 80);
        QCOMPARE(m.containsPoint(p1), true);
        QCOMPARE(m.getValueMinimum(), 123.4);
        QCOMPARE(m.getValueMaximum(), 124.3);
        m.deletePoint(p1);
        QCOMPARE(m.getPointCount(), 1);
        QCOMPARE(m.getPoints().size(), 1);
        QCOMPARE(*m.getPoints().begin(), p2);
        QCOMPARE(m.getStartFrame(), 50);
        QCOMPARE(m.getEndFrame(), 80);
        QCOMPARE(m.containsPoint(p1), false);
    }
             
    void note_sample() {
        NoteModel m(100, 10, false);
        NoteModel::Point p1(20, 123.4, 20, 0.8, "note 1");
        NoteModel::Point p2(20, 124.3, 10, 0.9, "note 2");
        NoteModel::Point p3(50, 126.3, 30, 0.9, "note 3");
        m.addPoint(p1);
        m.addPoint(p2);
        m.addPoint(p3);

        QCOMPARE(m.getPoints().size(), 3);
        QCOMPARE(*m.getPoints().begin(), p1);
        QCOMPARE(*m.getPoints().rbegin(), p3);

        auto pp = m.getPoints(20, 30);
        QCOMPARE(pp.size(), 2);
        QCOMPARE(*pp.begin(), p1);
        QCOMPARE(*pp.rbegin(), p2);

        pp = m.getPoints(30, 50);
        QCOMPARE(pp.size(), 1);
        QCOMPARE(*pp.begin(), p1);

        pp = m.getPoints(40, 50);
        QCOMPARE(pp.size(), 0);

        pp = m.getPoints(50, 50);
        QCOMPARE(pp.size(), 1);
        QCOMPARE(*pp.begin(), p3);
    }

    void note_xml() {
        NoteModel m(100, 10, false);
        NoteModel::Point p1(20, 123.4, 20, 0.8, "note 1");
        NoteModel::Point p2(20, 124.3, 10, 0.9, "note 2");
        NoteModel::Point p3(50, 126.3, 30, 0.9, "note 3");
        m.setScaleUnits("Hz");
        m.addPoint(p1);
        m.addPoint(p2);
        m.addPoint(p3);
        QString xml;
        QTextStream str(&xml, QIODevice::WriteOnly);
        m.toXml(str);
        str.flush();
        QString expected =
            "<model id='3' name='' sampleRate='100' start='20' end='60' type='sparse' dimensions='3' resolution='10' notifyOnAdd='false' dataset='2'  subtype='note' valueQuantization='0' minimum='123.4' maximum='126.3' units='Hz'/>\n"
            "<dataset id='2' dimensions='3'>\n"
            "  <point frame='20' value='123.4' duration='20' level='0.8' label='note 1' />\n"
            "  <point frame='20' value='124.3' duration='10' level='0.9' label='note 2' />\n"
            "  <point frame='50' value='126.3' duration='30' level='0.9' label='note 3' />\n"
            "</dataset>\n";
        expected.replace("\'", "\"");
        if (xml != expected) {
            cerr << "Obtained xml:\n" << xml
                 << "\nExpected:\n" << expected << endl;
        }
        QCOMPARE(xml, expected);
    }
    
        
};

#endif
