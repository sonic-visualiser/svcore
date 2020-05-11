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

#ifndef SV_TEXT_TEST_H
#define SV_TEXT_TEST_H

#include "data/fileio/FileSource.h"

class TextTest
{
public:
    /**
     * Return true if the source appears to point to a text format of
     * some kind (could be CSV, XML, RDF/Turtle etc).
     *
     * We apply two tests and report success if either succeeds:
     *
     * 1. The first few hundred bytes (where present) of the document
     *    are valid UTF-8
     *
     * 2. The document starts with the text "<?xml" when opened using
     *    QXmlInputSource (which guesses its text encoding)
     *
     * So we only accept non-UTF-8 encodings where they also happen to
     * be XML documents.
     */
    static bool isApparentTextDocument(FileSource);
};

#endif
