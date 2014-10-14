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

#include "CSVFeatureWriter.h"

#include <iostream>

#include <QRegExp>
#include <QTextStream>

using namespace std;
using namespace Vamp;

CSVFeatureWriter::CSVFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFileTotal |
                      SupportStdOut,
                      "csv"),
    m_separator(","),
    m_sampleTiming(false),
    m_endTimes(false)
{
}

CSVFeatureWriter::~CSVFeatureWriter()
{
}

string
CSVFeatureWriter::getDescription() const
{
    return "Write features in comma-separated (CSV) format. If transforms are being written to a single file or to stdout, the first column in the output will contain the input audio filename, or an empty string if the feature hails from the same audio file as its predecessor. If transforms are being written to multiple files, the audio filename column will be omitted. Subsequent columns will contain the feature timestamp, then any or all of duration, values, and label.";
}

CSVFeatureWriter::ParameterList
CSVFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();
    Parameter p;
    
    p.name = "separator";
    p.description = "Column separator for output.  Default is \",\" (comma).";
    p.hasArg = true;
    pl.push_back(p);
    
    p.name = "sample-timing";
    p.description = "Show timings as sample frame counts instead of in seconds.";
    p.hasArg = false;
    pl.push_back(p);
    
    p.name = "end-times";
    p.description = "Show start and end time instead of start and duration, for features with duration.";
    p.hasArg = false;
    pl.push_back(p);

    return pl;
}

void
CSVFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);

    SVDEBUG << "CSVFeatureWriter::setParameters" << endl;
    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        cerr << i->first << " -> " << i->second << endl;
        if (i->first == "separator") {
            m_separator = i->second.c_str();
        } else if (i->first == "sample-timing") {
            m_sampleTiming = true;
        } else if (i->first == "end-times") {
            m_endTimes = true;
        }
    }
}

void
CSVFeatureWriter::write(QString trackId,
                        const Transform &transform,
                        const Plugin::OutputDescriptor& ,
                        const Plugin::FeatureList& features,
                        std::string summaryType)
{
    // Select appropriate output file for our track/transform
    // combination

    QTextStream *sptr = getOutputStream(trackId, transform.getIdentifier());
    if (!sptr) {
        throw FailedToOpenOutputStream(trackId, transform.getIdentifier());
    }

    QTextStream &stream = *sptr;

    for (unsigned int i = 0; i < features.size(); ++i) {

        if (m_stdout || m_singleFileName != "") {
            if (trackId != m_prevPrintedTrackId) {
                stream << "\"" << trackId << "\"" << m_separator;
                m_prevPrintedTrackId = trackId;
            } else {
                stream << m_separator;
            }
        }

        if (m_sampleTiming) {

            stream << Vamp::RealTime::realTime2Frame
                (features[i].timestamp, transform.getSampleRate());

            if (features[i].hasDuration) {
                stream << m_separator;
                if (m_endTimes) {
                    stream << Vamp::RealTime::realTime2Frame
                        (features[i].timestamp + features[i].duration,
                         transform.getSampleRate());
                } else {
                    stream << Vamp::RealTime::realTime2Frame
                        (features[i].duration, transform.getSampleRate());
                }
            }

        } else {

            QString timestamp = features[i].timestamp.toString().c_str();
            timestamp.replace(QRegExp("^ +"), "");
            stream << timestamp;

            if (features[i].hasDuration) {
                if (m_endTimes) {
                    QString endtime =
                        (features[i].timestamp + features[i].duration)
                        .toString().c_str();
                    endtime.replace(QRegExp("^ +"), "");
                    stream << m_separator << endtime;
                } else {
                    QString duration = features[i].duration.toString().c_str();
                    duration.replace(QRegExp("^ +"), "");
                    stream << m_separator << duration;
                }
            }            
        }

        if (summaryType != "") {
            stream << m_separator << summaryType.c_str();
        }

        for (unsigned int j = 0; j < features[i].values.size(); ++j) {
            stream << m_separator << features[i].values[j];
        }

        if (features[i].label != "") {
            stream << m_separator << "\"" << features[i].label.c_str() << "\"";
        }

        stream << "\n";
    }
}


