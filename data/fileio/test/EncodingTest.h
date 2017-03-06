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
#include "../WavFileWriter.h"

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

// Mapping between filename and expected title metadata field
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
    QString outDir;

public:
    EncodingTest(QString base) {
        if (base == "") {
            base = "svcore/data/fileio/test";
        }
        testDirBase = base;
        encodingDir = base + "/encodings";
        outDir = base + "/outfiles";
    }

private:
    const char *strOf(QString s) {
        return strdup(s.toLocal8Bit().data());
    }

    void addAudioFiles() {
        QTest::addColumn<QString>("audiofile");
        QStringList files = QDir(encodingDir).entryList(QDir::Files);
        foreach (QString filename, files) {
            QTest::newRow(strOf(filename)) << filename;
        }
    }

private slots:
    void init()
    {
        if (!QDir(encodingDir).exists()) {
            cerr << "ERROR: Audio encoding file directory \"" << encodingDir << "\" does not exist" << endl;
            QVERIFY2(QDir(encodingDir).exists(), "Audio encoding file directory not found");
        }
        if (!QDir(outDir).exists() && !QDir().mkpath(outDir)) {
            cerr << "ERROR: Audio out directory \"" << outDir << "\" does not exist and could not be created" << endl;
            QVERIFY2(QDir(outDir).exists(), "Audio out directory not found and could not be created");
        }
    }

    void readAudio_data() {
        addAudioFiles();
    }

    void readAudio() {

        // Ensure that we can open all the files
        
        QFETCH(QString, audiofile);

        AudioFileReaderFactory::Parameters params;
        AudioFileReader *reader =
            AudioFileReaderFactory::createReader
            (encodingDir + "/" + audiofile, params);

        QVERIFY(reader != nullptr);

        delete reader;
    }

    void readMetadata_data() {
        addAudioFiles();
    }
    
    void readMetadata() {
        
        // All files other than WAVs should have title metadata; check
        // that the title matches whatever is in our mapping structure
        // defined at the top
        
        QFETCH(QString, audiofile);

        AudioFileReaderFactory::Parameters params;
        AudioFileReader *reader =
            AudioFileReaderFactory::createReader
            (encodingDir + "/" + audiofile, params);

        QVERIFY(reader != nullptr);

        QStringList fileAndExt = audiofile.split(".");
        QString file = fileAndExt[0];
        QString extension = fileAndExt[1];

        if (extension == "wav") {

            // Nothing
            
            delete reader;

        } else {

#if (!defined (HAVE_OGGZ) || !defined(HAVE_FISHSOUND))
            if (extension == "ogg") {
                QSKIP("Lack native Ogg Vorbis reader, so won't be getting metadata");
            }
#endif
            
            auto blah = reader->getInterleavedFrames(0, 10);
            
            QString title = reader->getTitle();
            QVERIFY(title != QString());

            delete reader;

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
                // Note that this can happen legitimately on Windows,
                // where (for annoying VCS-related reasons) the test
                // files may have a different filename encoding from
                // the expected UTF-16. We check this properly in
                // readWriteAudio below, by saving out the file to a
                // name matching the metadata
                cerr << "Couldn't find filename \""
                     << file << "\" in title mapping array" << endl;
                QSKIP("Couldn't find filename in title mapping array");
            }
        }
    }

    void readWriteAudio_data() {
        addAudioFiles();
    }

    void readWriteAudio()
    {
        // For those files that have title metadata (i.e. all of them
        // except the WAVs), read the title metadata and write a wav
        // file (of arbitrary content) whose name matches that.  Then
        // check that we can re-read it. This is intended to exercise
        // systems on which the original test filename is miscoded (as
        // can happen on Windows).
        
        QFETCH(QString, audiofile);

        QStringList fileAndExt = audiofile.split(".");
        QString file = fileAndExt[0];
        QString extension = fileAndExt[1];

        if (extension == "wav") {
            return;
        }

#if (!defined (HAVE_OGGZ) || !defined(HAVE_FISHSOUND))
        if (extension == "ogg") {
            QSKIP("Lack native Ogg Vorbis reader, so won't be getting metadata");
        }
#endif

        AudioFileReaderFactory::Parameters params;
        AudioFileReader *reader =
            AudioFileReaderFactory::createReader
            (encodingDir + "/" + audiofile, params);
        QVERIFY(reader != nullptr);

        QString title = reader->getTitle();
        QVERIFY(title != QString());

        for (int useTemporary = 0; useTemporary <= 1; ++useTemporary) {
        
            QString outfile = outDir + "/" + file + ".wav";
            WavFileWriter writer(outfile,
                                 reader->getSampleRate(),
                                 1,
                                 useTemporary ?
                                 WavFileWriter::WriteToTemporary :
                                 WavFileWriter::WriteToTarget);

            QVERIFY(writer.isOK());

            floatvec_t data { 0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0 };
            const float *samples = data.data();
            bool ok = writer.writeSamples(&samples, 8);
            QVERIFY(ok);

            ok = writer.close();
            QVERIFY(ok);

            AudioFileReader *rereader =
                AudioFileReaderFactory::createReader(outfile, params);
            QVERIFY(rereader != nullptr);

            floatvec_t readFrames = rereader->getInterleavedFrames(0, 8);
            QCOMPARE(readFrames, data);

            delete rereader;
        }

        delete reader;
    }
};

#endif
