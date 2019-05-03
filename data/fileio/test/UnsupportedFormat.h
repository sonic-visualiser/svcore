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

#ifndef SV_UNSUPPORTED_FORMAT_H
#define SV_UNSUPPORTED_FORMAT_H

static bool isLegitimatelyUnsupported(QString format) {

#ifdef Q_OS_WIN
    return (format == "apple_lossless");
#else
#ifdef Q_OS_MAC
    return (format == "wma");
#else
    return (format == "aac" ||
            format == "apple_lossless" ||
            format == "m4a" ||
            format == "wma");
#endif
#endif
}

#endif
