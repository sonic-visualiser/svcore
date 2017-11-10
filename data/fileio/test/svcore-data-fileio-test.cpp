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

#include "../../../test/TestHelper.h"
#include "AudioFileReaderTest.h"
#include "AudioFileWriterTest.h"
#include "EncodingTest.h"
#include "MIDIFileReaderTest.h"
#include "CSVStreamWriterTest.h"

int main(int argc, char *argv[])
{
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

    return Test::startTestRunner(
        {
            Test::createFactory<AudioFileReaderTest>(testDir),
            Test::createFactory<AudioFileWriterTest>(testDir),
            Test::createFactory<EncodingTest>(testDir),
            Test::createFactory<MIDIFileReaderTest>(testDir),
            Test::createFactory<CSVStreamWriterTest>()
        },
        argc,
        argv,
        "test-fileio"
    );
}
