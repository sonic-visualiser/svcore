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

#include "TextTest.h"

#include "base/Debug.h"
#include "base/StringBits.h"

#include <QFile>
#include <QXmlStreamReader>

namespace sv {

bool
TextTest::isApparentTextDocument(FileSource source)
{
    // Return true if the document can be opened and contains some
    // sort of text, either UTF-8 (so it could be Turtle) or another
    // encoding that is recognised as XML

    if (!source.isAvailable()) {
        SVDEBUG << "NOTE: TextTest::isApparentTextDocument: Failed to retrieve document from " << source.getLocation() << endl;
        return false;
    }

    QFile file(source.getLocalFilename());
    if (!file.open(QFile::ReadOnly)) {
        SVDEBUG << "NOTE: TextTest::isApparentTextDocument: Failed to open local file from " << source.getLocalFilename() << endl;
        return false;
    }

    QByteArray bytes = file.read(200);

    if (StringBits::isValidUtf8(std::string(bytes.data()), true)) {
        SVDEBUG << "NOTE: TextTest::isApparentTextDocument: Document appears to be UTF-8" << endl;
        return true; // good enough to be worth trying to parse
    }

    QXmlStreamReader xmlReader(bytes);
    bool isApparentXml = true;
    if (xmlReader.hasError()) {
        isApparentXml = false;
    } else if (!xmlReader.atEnd()) {
        if (xmlReader.readNext() == QXmlStreamReader::Invalid) {
            isApparentXml = false;
        }
    }

    if (isApparentXml) {
        SVDEBUG << "NOTE: TextTest::isApparentTextDocument: Document appears to be XML" << endl;
        return true;
    }

    SVDEBUG << "NOTE: TextTest::isApparentTextDocument: Document is not UTF-8 and is not XML, rejecting" << endl;
    return false;
}

} // end namespace sv

