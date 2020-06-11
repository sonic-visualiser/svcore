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

#ifndef TEST_CSV_READER_H
#define TEST_CSV_READER_H

#include "../CSVFileReader.h"

#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/SparseTimeValueModel.h"
#include "data/model/RegionModel.h"
#include "data/model/EditableDenseThreeDimensionalModel.h"

#include "base/Debug.h"

#include <cmath>

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

class CSVReaderTest : public QObject
{
    Q_OBJECT

private:
    QDir csvDir;
    sv_samplerate_t mainRate;

public:
    CSVReaderTest(QString base) {
        if (base == "") {
            base = "svcore/data/fileio/test";
        }
        csvDir = QDir(base + "/csv");
        mainRate = 44100;
    }

private:
    void loadFrom(QString filename, Model *&model) {
        QString path(csvDir.filePath(filename));
        CSVFormat f;
        f.guessFormatFor(path);
        CSVFileReader reader(path, f, mainRate);
        model = reader.load();
        QVERIFY(model);
        QVERIFY(reader.isOK());
        QCOMPARE(reader.getError(), QString());
    }

private slots:
    void init() {
        if (!csvDir.exists()) {
            SVCERR << "ERROR: CSV test file directory \"" << csvDir.absolutePath() << "\" does not exist" << endl;
            QVERIFY2(csvDir.exists(), "CSV test file directory not found");
        }
    }

    void modelType1DSamples() {
        Model *model = nullptr;
        loadFrom("model-type-1d-samples.csv", model);
        auto actual = qobject_cast<SparseOneDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        //!!! + the actual contents
        delete model;
    }

    void modelType1DSeconds() {
        Model *model = nullptr;
        loadFrom("model-type-1d-seconds.csv", model);
        auto actual = qobject_cast<SparseOneDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }

    void modelType2DDurationSamples() {
        Model *model = nullptr;
        loadFrom("model-type-2d-duration-samples.csv", model);
        auto actual = qobject_cast<RegionModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DDurationSeconds() {
        Model *model = nullptr;
        loadFrom("model-type-2d-duration-seconds.csv", model);
        auto actual = qobject_cast<RegionModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void badNegativeDuration() {
        Model *model = nullptr;
        loadFrom("bad-negative-duration.csv", model);
        auto actual = qobject_cast<RegionModel *>(model);
        QVERIFY(actual);
        //!!! + check duration has been corrected
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DEndTimeSamples() {
        Model *model = nullptr;
        loadFrom("model-type-2d-endtime-samples.csv", model);
        auto actual = qobject_cast<RegionModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DEndTimeSeconds() {
        Model *model = nullptr;
        loadFrom("model-type-2d-endtime-seconds.csv", model);
        auto actual = qobject_cast<RegionModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DImplicit() {
        Model *model = nullptr;
        loadFrom("model-type-2d-implicit.csv", model);
        auto actual = qobject_cast<SparseTimeValueModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DSamples() {
        Model *model = nullptr;
        loadFrom("model-type-2d-samples.csv", model);
        auto actual = qobject_cast<SparseTimeValueModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType2DSeconds() {
        Model *model = nullptr;
        loadFrom("model-type-2d-seconds.csv", model);
        auto actual = qobject_cast<SparseTimeValueModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void modelType3DImplicit() {
        Model *model = nullptr;
        loadFrom("model-type-3d-implicit.csv", model);
        auto actual = qobject_cast<EditableDenseThreeDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getWidth(), 6);
        QCOMPARE(actual->getHeight(), 6);
        delete model;
    }
    
    void modelType3DSamples() {
        Model *model = nullptr;
        loadFrom("model-type-3d-samples.csv", model);
        auto actual = qobject_cast<EditableDenseThreeDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getWidth(), 6);
        QCOMPARE(actual->getHeight(), 6);
        delete model;
    }
    
    void modelType3DSeconds() {
        Model *model = nullptr;
        loadFrom("model-type-3d-seconds.csv", model);
        auto actual = qobject_cast<EditableDenseThreeDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getWidth(), 6);
        QCOMPARE(actual->getHeight(), 6);
        delete model;
    }
    
    void withBlankLines1D() {
        Model *model = nullptr;
        loadFrom("with-blank-lines-1d.csv", model);
        auto actual = qobject_cast<SparseOneDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void withBlankLines2D() {
        Model *model = nullptr;
        loadFrom("with-blank-lines-2d.csv", model);
        auto actual = qobject_cast<SparseTimeValueModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
    
    void withBlankLines3D() {
        Model *model = nullptr;
        loadFrom("with-blank-lines-3d.csv", model);
        auto actual = qobject_cast<EditableDenseThreeDimensionalModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getWidth(), 6);
        QCOMPARE(actual->getHeight(), 6);
        delete model;
    }

    void quoting() {
        Model *model = nullptr;
        loadFrom("quoting.csv", model);
        auto actual = qobject_cast<SparseTimeValueModel *>(model);
        QVERIFY(actual);
        QCOMPARE(actual->getAllEvents().size(), 5);
        delete model;
    }
};

#endif
