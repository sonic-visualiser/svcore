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

static QString encodingDir = "svcore/data/fileio/test/encodings";

static const char *mapping[][2] = {
    { u8"id3v2-iso-8859-1", u8"Café de Paris" },
    { u8"id3v2-ucs-2", u8"Café de 重庆" },
    { u8"Tëmple of Spörks", u8"Tëmple of Spörks" },
    { u8"スポークの寺院", u8"スポークの寺院" }
};
static const int mappingCount = 4;

class EncodingTest : public QObject
{
    Q_OBJECT

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
                    QCOMPARE(title, QString::fromUtf8(mapping[m][1]));
                    break;
                }
            }

            if (!found) {
                cerr << "Failed to find filename \""
                     << file << "\" in title mapping array" << endl;
                QVERIFY(found);
            }
        }
    }
};

#endif
