/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _PLUGIN_RDF_DESCRIPTION_H_
#define _PLUGIN_RDF_DESCRIPTION_H_

#include <QString>
#include <map>

class FileSource;

class PluginRDFDescription
{
public:
    PluginRDFDescription() : m_haveDescription(false) { }
    PluginRDFDescription(QString pluginId);
    ~PluginRDFDescription();

    enum OutputType
    {
        OutputTypeUnknown,
        OutputFeatures,
        OutputEvents,
        OutputFeaturesAndEvents
    };

    enum OutputDisposition
    {
        OutputDispositionUnknown,
        OutputSparse,
        OutputDense,
        OutputTrackLevel
    };

    bool haveDescription() const;
    OutputType getOutputType(QString outputId) const;
    OutputDisposition getOutputDisposition(QString outputId) const;
    QString getOutputFeatureTypeURI(QString outputId) const;
    QString getOutputEventTypeURI(QString outputId) const;
    QString getOutputUnit(QString outputId) const;

protected:    
    typedef std::map<QString, OutputType> OutputTypeMap;
    typedef std::map<QString, OutputDisposition> OutputDispositionMap;
    typedef std::map<QString, QString> OutputStringMap;

    QString m_pluginId;
    bool m_haveDescription;
    OutputTypeMap m_outputTypes;
    OutputDispositionMap m_outputDispositions;
    OutputStringMap m_outputFeatureTypeURIMap;
    OutputStringMap m_outputEventTypeURIMap;
    OutputStringMap m_outputUnitMap;
    bool indexURL(QString url);
};

#endif

