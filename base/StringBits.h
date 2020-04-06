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

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2010 Chris Cannam.
*/

#ifndef SV_STRING_BITS_H
#define SV_STRING_BITS_H

#include <QString>
#include <QStringList>
#include <QChar>

class StringBits
{
public:
    /**
     * Convert a string to a double using basic "C"-locale syntax,
     * i.e. always using '.' as a decimal point.  We use this as a
     * fallback when parsing files from an unknown source, if
     * locale-specific conversion fails.  Does not support e notation.
     * If ok is non-NULL, *ok will be set to true if conversion
     * succeeds or false otherwise.
     */
    static double stringToDoubleLocaleFree(QString s, bool *ok = 0);

    /**
     * Split a string at the given separator character, allowing
     * quoted sections that contain the separator.  If the separator
     * is ' ', any (amount of) whitespace will be considered as a
     * single separator.  If the separator is another whitespace
     * character such as '\t', it will be used literally.
     */
    static QStringList splitQuoted(QString s, QChar separator);

    /**
     * Split a string at the given separator character.  If quoted is
     * true, do so by calling splitQuoted (above).  If quoted is
     * false, use QString::split; if separator is ' ', use
     * SkipEmptyParts behaviour, otherwise use KeepEmptyParts (this is
     * analogous to the behaviour of splitQuoted).
     */
    static QStringList split(QString s, QChar separator, bool quoted);

    /**
     * Join a vector of strings into a single string, with the
     * delimiter as the joining string. If a string contains the
     * delimiter already, quote it with double-quotes, replacing any
     * existing double-quotes within it by a pair of double-quotes, as
     * specified in RFC 4180 Common Format and MIME Type for
     * Comma-Separated Values (CSV) Files.
     */
    static QString joinDelimited(QVector<QString> row, QString delimiter);
};

#endif
