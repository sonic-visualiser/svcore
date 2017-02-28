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

#include "AudioFileReaderTest.h"
#include "AudioFileWriterTest.h"
#include "EncodingTest.h"
#include "MIDIFileReaderTest.h"

#include <QtTest>

#include <iostream>

int main(int argc, char *argv[])
{
    int good = 0, bad = 0;

    QString testDir;

#ifdef Q_OS_WIN
    // incredible to have to hardcode this, but I just can't figure out how to
    // get QMAKE_POST_LINK to add an arg to its command successfully on Windows
    testDir = "../sonic-visualiser/svcore/data/fileio/test";
#endif

    if (argc > 1) {
        cerr << "argc = " << argc << endl;
        testDir = argv[1];
    }

    if (testDir != "") {
        cerr << "Setting test directory base path to \"" << testDir << "\"" << endl;
    }

    QCoreApplication app(argc, argv);
    app.setOrganizationName("sonic-visualiser");
    app.setApplicationName("test-fileio");

    {
        AudioFileReaderTest t(testDir);
        if (QTest::qExec(&t, argc, argv) == 0) ++good;
        else ++bad;
    }

    {
        AudioFileWriterTest t(testDir);
        if (QTest::qExec(&t, argc, argv) == 0) ++good;
        else ++bad;
    }

    {
        EncodingTest t(testDir);
        if (QTest::qExec(&t, argc, argv) == 0) ++good;
        else ++bad;
    }

    {
        MIDIFileReaderTest t(testDir);
        if (QTest::qExec(&t, argc, argv) == 0) ++good;
        else ++bad;
    }

    if (bad > 0) {
	cerr << "\n********* " << bad << " test suite(s) failed!\n" << endl;
	return 1;
    } else {
	cerr << "All tests passed" << endl;
	return 0;
    }
}

