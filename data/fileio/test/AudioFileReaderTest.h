/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2013 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TEST_AUDIO_FILE_READER_H
#define TEST_AUDIO_FILE_READER_H

#include "../AudioFileReaderFactory.h"
#include "../AudioFileReader.h"

#include "AudioTestData.h"

#include <cmath>

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;

static QString audioDir = "testfiles";

class AudioFileReaderTest : public QObject
{
    Q_OBJECT

    const char *strOf(QString s) {
        return strdup(s.toLocal8Bit().data());
    }

private slots:
    void init()
    {
        if (!QDir(audioDir).exists()) {
            cerr << "ERROR: Audio test file directory \"" << audioDir << "\" does not exist" << endl;
            QVERIFY2(QDir(audioDir).exists(), "Audio test file directory not found");
        }
    }

    void read_data()
    {
        QTest::addColumn<QString>("audiofile");
        QStringList files = QDir(audioDir).entryList(QDir::Files);
        foreach (QString filename, files) {
            QTest::newRow(strOf(filename)) << filename;
        }
    }

    void read()
    {
        QFETCH(QString, audiofile);

        int readRate = 48000;

	AudioFileReader *reader =
	    AudioFileReaderFactory::createReader
	    (audioDir + "/" + audiofile, readRate);

        QStringList fileAndExt = audiofile.split(".");
        QStringList bits = fileAndExt[0].split("-");
        QString extension = fileAndExt[1];
        int nominalRate = bits[0].toInt();
        int nominalChannels = bits[1].toInt();
        int nominalDepth = 16;
        if (bits.length() > 2) nominalDepth = bits[2].toInt();

	if (!reader) {
	    QSKIP("Unsupported file, skipping", SkipSingle);
	}

        QCOMPARE((int)reader->getChannelCount(), nominalChannels);
        QCOMPARE((int)reader->getNativeRate(), nominalRate);
        QCOMPARE((int)reader->getSampleRate(), readRate);

	int channels = reader->getChannelCount();
	AudioTestData tdata(readRate, channels);
	
	float *reference = tdata.getInterleavedData();
	int refsize = tdata.getFrameCount() * channels;
	
	vector<float> test;
	
	// The reader should give us exactly the expected number of
	// frames, except for mp3 files -- so we ask for one more,
	// just to check we don't get it!
	reader->getInterleavedFrames
	    (0, tdata.getFrameCount() + 1, test);
	int read = test.size() / channels;

        if (extension == "mp3") {
            // mp3s round up
            QVERIFY(read >= tdata.getFrameCount());
        } else {
            QCOMPARE(read, tdata.getFrameCount());
        }

        // Our limits are pretty relaxed -- we're not testing decoder
        // or resampler quality here, just whether the results are
        // plainly wrong (e.g. at wrong samplerate or with an offset)

	float limit = 0.01;
        if (nominalDepth < 16) {
            limit = 0.02;
        }
        if (extension == "ogg" || extension == "mp3" || extension == "aac") {
            limit = 0.04;
        }

        int edgeSize = 100; 
        float edgeLimit = limit * 10; // in first or final edgeSize frames

	for (int c = 0; c < channels; ++c) {
	    float maxdiff = 0.f;
	    int maxAt = 0;
	    float totdiff = 0.f;
	    for (int i = 0; i < read; ++i) {
		float diff = fabsf(test[i * channels + c] -
				   reference[i * channels + c]);
		totdiff += diff;
                // in edge areas, record this only if it exceeds edgeLimit
                if (i < edgeSize || i + edgeSize >= read) {
                    if (diff > edgeLimit) {
                        maxdiff = diff;
                        maxAt = i;
                    }
                } else {
                    if (diff > maxdiff) {
                        maxdiff = diff;
                        maxAt = i;
                    }
		}
	    }
	    float meandiff = totdiff / read;
//	    cerr << "meandiff on channel " << c << ": " << meandiff << endl;
//	    cerr << "maxdiff on channel " << c << ": " << maxdiff << " at " << maxAt << endl;
	    if (maxdiff >= limit) {
		cerr << "ERROR: for audiofile " << audiofile << ": maxdiff = " << maxdiff << " at frame " << maxAt << " of " << read << " on channel " << c << " (mean diff = " << meandiff << ")" << endl;
		QVERIFY(maxdiff < limit);
	    }
	}
    }
};

#endif
