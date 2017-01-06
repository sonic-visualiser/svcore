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

#ifndef TEST_AUDIO_ENCODINGS_H
#define TEST_AUDIO_ENCODINGS_H

// Quick tests for filename encodings and encoding of ID3 data. Not a
// test of audio codecs.

#include "../AudioFileReaderFactory.h"
#include "../AudioFileReader.h"

#include <cmath>

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

const char utf8_name_cdp_1[] = "Caf\303\251 de Paris";
const char utf8_name_cdp_2[] = "Caf\303\251 de \351\207\215\345\272\206";
const char utf8_name_tsprk[] = "T\303\253mple of Sp\303\266rks";
const char utf8_name_sprkt[] = "\343\202\271\343\203\235\343\203\274\343\202\257\343\201\256\345\257\272\351\231\242";

static const char *mapping[][2] = {
    { "id3v2-iso-8859-1", utf8_name_cdp_1 },
    { "id3v2-ucs-2", utf8_name_cdp_2 },
    { utf8_name_tsprk, utf8_name_tsprk },
    { utf8_name_sprkt, utf8_name_sprkt },
};
static const int mappingCount = 4;

class EncodingTest : public QObject
{
    Q_OBJECT

private:
    QString testDirBase;
    QString encodingDir;

public:
    EncodingTest(QString base) {
        if (base == "") {
            base = "svcore/data/fileio/test";
        }
        testDirBase = base;
        encodingDir = base + "/encodings";
    }

private:
    const char *strOf(QString s) {
        return strdup(s.toLocal8Bit().data());
    }

private slots:
    void init()
    {
        if (!QDir(encodingDir).exists()) {
            cerr << "ERROR: Audio encoding file directory \"" << encodingDir << "\" does not exist" << endl;
            QVERIFY2(QDir(encodingDir).exists(), "Audio encoding file directory not found");
        }
    }

    void read_data()
    {
        QTest::addColumn<QString>("audiofile");
        QStringList files = QDir(encodingDir).entryList(QDir::Files);
        foreach (QString filename, files) {
            QTest::newRow(strOf(filename)) << filename;
        }
    }

    void read()
    {
        QFETCH(QString, audiofile);

        AudioFileReaderFactory::Parameters params;
        AudioFileReader *reader =
            AudioFileReaderFactory::createReader
            (encodingDir + "/" + audiofile, params);

        QVERIFY(reader != nullptr);

        QStringList fileAndExt = audiofile.split(".");
        QString file = fileAndExt[0];
        QString extension = fileAndExt[1];

        if (extension == "mp3") {

            QString title = reader->getTitle();
            QVERIFY(title != QString());

            bool found = false;
            for (int m = 0; m < mappingCount; ++m) {
                if (file == QString::fromUtf8(mapping[m][0])) {
                    found = true;
                    QString expected = QString::fromUtf8(mapping[m][1]);
                    if (title != expected) {
                        cerr << "Title does not match expected: codepoints are" << endl;
                        cerr << "Title (" << title.length() << "ch): ";
                        for (int i = 0; i < title.length(); ++i) {
                            cerr << title[i].unicode() << " ";
                        }
                        cerr << endl;
                        cerr << "Expected (" << expected.length() << "ch): ";
                        for (int i = 0; i < expected.length(); ++i) {
                            cerr << expected[i].unicode() << " ";
                        }
                        cerr << endl;
                    }
                    QCOMPARE(title, expected);
                    break;
                }
            }

            if (!found) {
                cerr << "Couldn't find filename \""
                     << file << "\" in title mapping array" << endl;
                QSKIP("Couldn't find filename in title mapping array");
            }
        }
    }
};

#endif
