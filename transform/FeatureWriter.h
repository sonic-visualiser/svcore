/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.

    Sonic Annotator
    A utility for batch feature extraction from audio files.

    Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London.
    Copyright 2007-2008 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FEATURE_WRITER_H_
#define _FEATURE_WRITER_H_

#include <string>
#include <map>
#include <vector>

#include <QString>

#include "Transform.h"

#include <vamp-hostsdk/Plugin.h>

using std::string;
using std::map;
using std::vector;

class FeatureWriter
{
public:
    virtual ~FeatureWriter() { }

    struct Parameter { // parameter of the writer, not the plugin
        string name;
        string description;
        bool hasArg;
    };
    typedef vector<Parameter> ParameterList;
    virtual ParameterList getSupportedParameters() const {
        return ParameterList();
    }

    virtual void setParameters(map<string, string> &params) {
        return;
    }

    struct TrackMetadata {
        QString title;
        QString maker;
    };
    virtual void setTrackMetadata(QString trackid, TrackMetadata metadata) { }

    // may throw FailedToOpenFile or other exceptions

    virtual void write(QString trackid,
                       const Transform &transform,
                       const Vamp::Plugin::OutputDescriptor &output,
                       const Vamp::Plugin::FeatureList &features,
                       std::string summaryType = "") = 0;

    virtual void flush() { } // whatever the last stream was

    virtual void finish() = 0;
};

#endif
