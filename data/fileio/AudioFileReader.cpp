/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioFileReader.h"

void
AudioFileReader::setError(std::string text, std::string arg)
{
    if (arg == "") m_error = text;
    else m_error = text + ": " + arg;
}

void
AudioFileReader::setError(std::string text, int arg)
{
    char *buf = new char(text.length() + 100);
    sprintf(buf, "%s: code %d", text.c_str(), arg);
    m_error = buf;
    delete[] buf;
}

