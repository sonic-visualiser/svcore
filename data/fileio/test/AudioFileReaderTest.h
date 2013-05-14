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
	    QSKIP("Unsupported file, skipping");
	}

        QCOMPARE((int)reader->getChannelCount(), nominalChannels);
        QCOMPARE((int)reader->getNativeRate(), nominalRate);
        QCOMPARE((int)reader->getSampleRate(), readRate);

	int channels = reader->getChannelCount();
	AudioTestData tdata(readRate, channels);
	
	float *reference = tdata.getInterleavedData();
        int refFrames = tdata.getFrameCount();
	int refsize = refFrames * channels;
	
	vector<float> test;
	
	// The reader should give us exactly the expected number of
	// frames, except for mp3/aac files. We ask for quite a lot
	// more, though, so we can (a) check that we only get the
	// expected number back (if this is not mp3/aac) or (b) take
	// into account silence at beginning and end (if it is).
	reader->getInterleavedFrames(0, refFrames + 5000, test);
	int read = test.size() / channels;

        if (extension == "mp3" || extension == "aac" || extension == "m4a") {
            // mp3s and aacs can have silence at start and end
            QVERIFY(read >= refFrames);
        } else {
            QCOMPARE(read, refFrames);
        }

        // Our limits are pretty relaxed -- we're not testing decoder
        // or resampler quality here, just whether the results are
        // plainly wrong (e.g. at wrong samplerate or with an offset)

	float limit = 0.01;
        float edgeLimit = limit * 10; // in first or final edgeSize frames
        int edgeSize = 100; 

        if (nominalDepth < 16) {
            limit = 0.02;
        }
        if (extension == "ogg" || extension == "mp3" ||
            extension == "aac" || extension == "m4a") {
            limit = 0.2;
            edgeLimit = limit * 3;
        }

        // And we ignore completely the last few frames when upsampling
        int discard = 1 + readRate / nominalRate;

        int offset = 0;

        if (extension == "aac" || extension == "m4a") {
            // our m4a file appears to have a fixed offset of 1024 (at
            // file sample rate)
            offset = (1024 / float(nominalRate)) * readRate;
        }

        if (extension == "mp3") {
            // while mp3s appear to vary
            for (int i = 0; i < read; ++i) {
                bool any = false;
                float thresh = 0.01;
                for (int c = 0; c < channels; ++c) {
                    if (fabsf(test[i * channels + c]) > thresh) {
                        any = true;
                        break;
                    }
                }
                if (any) {
                    offset = i;
                    break;
                }
            }
//            std::cerr << "offset = " << offset << std::endl;
        }

	for (int c = 0; c < channels; ++c) {
	    float maxdiff = 0.f;
	    int maxAt = 0;
	    float totdiff = 0.f;
	    for (int i = 0; i < read - offset - discard && i < refFrames; ++i) {
		float diff = fabsf(test[(i + offset) * channels + c] -
				   reference[i * channels + c]);
		totdiff += diff;
                // in edge areas, record this only if it exceeds edgeLimit
                if (i < edgeSize || i + edgeSize >= read - offset) {
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
            if (meandiff >= limit) {
		cerr << "ERROR: for audiofile " << audiofile << ": mean diff = " << meandiff << " for channel " << c << endl;
                QVERIFY(meandiff < limit);
            }
	    if (maxdiff >= limit) {
		cerr << "ERROR: for audiofile " << audiofile << ": max diff = " << maxdiff << " at frame " << maxAt << " of " << read << " on channel " << c << " (mean diff = " << meandiff << ")" << endl;
		QVERIFY(maxdiff < limit);
	    }
	}
    }
};

#endif
