/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
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

#include <fstream>

#include "vamp-hostsdk/PluginHostAdapter.h"
#include "vamp-hostsdk/PluginLoader.h"

#include "RDFFeatureWriter.h"
#include "RDFTransformFactory.h"

#include <QTextStream>
#include <QUrl>
#include <QRegExp>

using namespace std;
using Vamp::Plugin;
using Vamp::PluginBase;

RDFFeatureWriter::RDFFeatureWriter() :
    FileFeatureWriter(SupportOneFilePerTrackTransform |
                      SupportOneFilePerTrack |
                      SupportOneFileTotal,
                      "n3"),
    m_plain(false),
    m_count(0)
{
}

RDFFeatureWriter::~RDFFeatureWriter()
{
}

RDFFeatureWriter::ParameterList
RDFFeatureWriter::getSupportedParameters() const
{
    ParameterList pl = FileFeatureWriter::getSupportedParameters();
    Parameter p;

    p.name = "plain";
    p.description = "Use \"plain\" RDF even if transform metadata is available.";
    p.hasArg = false;
    pl.push_back(p);

    p.name = "signal-uri";
    p.description = "Link the output RDF to the given signal URI.";
    p.hasArg = true;
    pl.push_back(p);
    
    return pl;
}

void
RDFFeatureWriter::setParameters(map<string, string> &params)
{
    FileFeatureWriter::setParameters(params);

    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        if (i->first == "plain") {
            m_plain = true;
        }
        if (i->first == "signal-uri") {
            m_suri = i->second.c_str();
        }
    }
}

void
RDFFeatureWriter::setTrackMetadata(QString trackId,
                                   TrackMetadata metadata)
{
    std::cerr << "RDFFeatureWriter::setTrackMetadata: \""
              << trackId.toStdString() << "\" -> \"" << metadata.title.toStdString() << "\",\"" << metadata.maker.toStdString() << "\"" << std::endl;
    m_metadata[trackId] = metadata;
}

void
RDFFeatureWriter::write(QString trackId,
                        const Transform &transform,
                        const Plugin::OutputDescriptor& output,
                        const Plugin::FeatureList& features,
                        std::string summaryType)
{
    QString pluginId = transform.getPluginIdentifier();

    if (m_rdfDescriptions.find(pluginId) == m_rdfDescriptions.end()) {

        m_rdfDescriptions[pluginId] = PluginRDFDescription(pluginId);

        if (m_rdfDescriptions[pluginId].haveDescription()) {
            cerr << "NOTE: Have RDF description for plugin ID \""
                 << pluginId.toStdString() << "\"" << endl;
        } else {
            cerr << "NOTE: Do not have RDF description for plugin ID \""
                 << pluginId.toStdString() << "\"" << endl;
        }
    }

    // Need to select appropriate output file for our track/transform
    // combination

    QTextStream *stream = getOutputStream(trackId, transform.getIdentifier());
    if (!stream) return; //!!! this is probably better handled with an exception

    if (m_startedStreamTransforms.find(stream) ==
        m_startedStreamTransforms.end()) {
        cerr << "This stream is new, writing prefixes" << endl;
        writePrefixes(stream);
        if (m_singleFileName == "" && !m_stdout) {
            writeSignalDescription(stream, trackId);
        }
    }

    if (m_startedStreamTransforms[stream].find(transform) ==
        m_startedStreamTransforms[stream].end()) {
        m_startedStreamTransforms[stream].insert(transform);
        writeLocalFeatureTypes
            (stream, transform, output, m_rdfDescriptions[pluginId]);
    }

    if (m_singleFileName != "" || m_stdout) {
        if (m_startedTrackIds.find(trackId) == m_startedTrackIds.end()) {
            writeSignalDescription(stream, trackId);
            m_startedTrackIds.insert(trackId);
        }
    }

    QString timelineURI = m_trackTimelineURIs[trackId];
    
    if (timelineURI == "") {
        cerr << "RDFFeatureWriter: INTERNAL ERROR: writing features without having established a timeline URI!" << endl;
        exit(1);
    }

    if (summaryType != "") {

        writeSparseRDF(stream, transform, output, features,
                       m_rdfDescriptions[pluginId], timelineURI);

    } else if (m_rdfDescriptions[pluginId].haveDescription() &&
               m_rdfDescriptions[pluginId].getOutputDisposition
               (output.identifier.c_str()) == 
               PluginRDFDescription::OutputDense) {

        QString signalURI = m_trackSignalURIs[trackId];

        if (signalURI == "") {
            cerr << "RDFFeatureWriter: INTERNAL ERROR: writing dense features without having established a signal URI!" << endl;
            exit(1);
        }

        writeDenseRDF(stream, transform, output, features,
                      m_rdfDescriptions[pluginId], signalURI, timelineURI);

    } else {

        writeSparseRDF(stream, transform, output, features,
                       m_rdfDescriptions[pluginId], timelineURI);
    }
}

void
RDFFeatureWriter::writePrefixes(QTextStream *sptr)
{
    QTextStream &stream = *sptr;

    stream << "@prefix dc: <http://purl.org/dc/elements/1.1/> .\n"
           << "@prefix mo: <http://purl.org/ontology/mo/> .\n"
           << "@prefix af: <http://purl.org/ontology/af/> .\n"
           << "@prefix foaf: <http://xmlns.com/foaf/0.1/> . \n"
           << "@prefix event: <http://purl.org/NET/c4dm/event.owl#> .\n"
           << "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
           << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
           << "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
           << "@prefix tl: <http://purl.org/NET/c4dm/timeline.owl#> .\n"
           << "@prefix vamp: <http://purl.org/ontology/vamp/> .\n"
           << "@prefix : <#> .\n\n";
}

void
RDFFeatureWriter::writeSignalDescription(QTextStream *sptr,
                                         QString trackId)
{
    QTextStream &stream = *sptr;

    /*
     * Describe signal we're analysing (AudioFile, Signal, TimeLine, etc.)
     */
    
    QUrl url(trackId);
    QString scheme = url.scheme().toLower();
    bool local = (scheme == "" || scheme == "file" || scheme.length() == 1);

    if (local) {
        if (scheme == "") {
            url.setScheme("file");
        } else if (scheme.length() == 1) { // DOS drive letter!
            url.setScheme("file");
            url.setPath(scheme + ":" + url.path());
        }
    }

    //!!! FIX: If we are appending, we need to start counting after
    //all of the existing counts that are already in the file!

    uint64_t signalCount = m_count++;

    if (m_trackSignalURIs.find(trackId) == m_trackSignalURIs.end()) {
        m_trackSignalURIs[trackId] = QString(":signal_%1").arg(signalCount);
    }
    
    if (m_suri != NULL) {
        m_trackSignalURIs[trackId] = "<" + m_suri + ">";
    }
    QString signalURI = m_trackSignalURIs[trackId];
   
    if (m_trackTimelineURIs.find(trackId) == m_trackTimelineURIs.end()) {
        m_trackTimelineURIs[trackId] = QString(":signal_timeline_%1").arg(signalCount);
    }
    QString timelineURI = m_trackTimelineURIs[trackId];

    if (trackId != "") {
        stream << "\n<" << url.toEncoded().data() << "> a mo:AudioFile .\n\n";
    }

    stream << signalURI << " a mo:Signal ;\n";

    if (trackId != "") {
        stream << "    mo:available_as <" << url.toEncoded().data()
               << "> ;\n";
    }

    if (m_metadata.find(trackId) != m_metadata.end()) {
        TrackMetadata tm = m_metadata[trackId];
        if (tm.title != "") {
            stream << "    dc:title \"\"\"" << tm.title << "\"\"\" ;\n";
        }
        if (tm.maker != "") {
            stream << "    dc:creator [ a mo:MusicArtist; foaf:name \"\"\"" << tm.maker << "\"\"\" ] ;\n";
        }
    }

    stream << "    mo:time [\n"
           << "        a tl:Interval ;\n"
           << "        tl:onTimeLine "
           << timelineURI << "\n    ] .\n\n";
} 

void
RDFFeatureWriter::writeLocalFeatureTypes(QTextStream *sptr,
                                         const Transform &transform,
                                         const Plugin::OutputDescriptor &od,
                                         PluginRDFDescription &desc)
{
    QString outputId = od.identifier.c_str();
    QTextStream &stream = *sptr;

    bool needEventType = false;
    bool needSignalType = false;

    //!!! feature attribute type is not yet supported

    //!!! bin names, extents and so on can be written out using e.g. vamp:bin_names ( "a" "b" "c" ) 

    if (desc.getOutputDisposition(outputId) == 
        PluginRDFDescription::OutputDense) {

        // no feature events, so may need signal type but won't need
        // event type

        if (m_plain) {

            needSignalType = true;

        } else if (desc.getOutputSignalTypeURI(outputId) == "") {
            
            needSignalType = true;
        }

    } else {

        // may need event type but won't need signal type

        if (m_plain) {
        
            needEventType = true;
    
        } else if (desc.getOutputEventTypeURI(outputId) == "") {

            needEventType = true;
        }
    }

    QString transformUri;
    if (m_transformURIs.find(transform) != m_transformURIs.end()) {
        transformUri = m_transformURIs[transform];
    } else {
        transformUri = QString(":transform_%1_%2").arg(m_count++).arg(outputId);
        m_transformURIs[transform] = transformUri;
    }

    if (transform.getIdentifier() != "") {
        stream << RDFTransformFactory::writeTransformToRDF(transform, transformUri)
               << endl;
    }

    if (needEventType) {

        QString uri;
        if (m_syntheticEventTypeURIs.find(transform) !=
            m_syntheticEventTypeURIs.end()) {
            uri = m_syntheticEventTypeURIs[transform];
        } else {
            uri = QString(":event_type_%1").arg(m_count++);
            m_syntheticEventTypeURIs[transform] = uri;
        }

        stream << uri
               << " rdfs:subClassOf event:Event ;" << endl
               << "    dc:title \"" << od.name.c_str() << "\" ;" << endl
               << "    dc:format \"" << od.unit.c_str() << "\" ;" << endl
               << "    dc:description \"" << od.description.c_str() << "\" ."
               << endl << endl;
    }

    if (needSignalType) {

        QString uri;
        if (m_syntheticSignalTypeURIs.find(transform) !=
            m_syntheticSignalTypeURIs.end()) {
            uri = m_syntheticSignalTypeURIs[transform];
        } else {
            uri = QString(":signal_type_%1").arg(m_count++);
            m_syntheticSignalTypeURIs[transform] = uri;
        }

        stream << uri
               << " rdfs:subClassOf af:Signal ;" << endl
               << "    dc:title \"" << od.name.c_str() << "\" ;" << endl
               << "    dc:format \"" << od.unit.c_str() << "\" ;" << endl
               << "    dc:description \"" << od.description.c_str() << "\" ."
               << endl << endl;
    }
}

void
RDFFeatureWriter::writeSparseRDF(QTextStream *sptr,
                                 const Transform &transform,
                                 const Plugin::OutputDescriptor& od,
                                 const Plugin::FeatureList& featureList,
                                 PluginRDFDescription &desc,
                                 QString timelineURI)
{
    if (featureList.empty()) return;
    QTextStream &stream = *sptr;
        
    bool plain = (m_plain || !desc.haveDescription());

    QString outputId = od.identifier.c_str();

    // iterate through FeatureLists
        
    for (int i = 0; i < featureList.size(); ++i) {

        const Plugin::Feature &feature = featureList[i];
        uint64_t featureNumber = m_count++;

        stream << ":event_" << featureNumber << " a ";

        QString eventTypeURI = desc.getOutputEventTypeURI(outputId);
        if (plain || eventTypeURI == "") {
            if (m_syntheticEventTypeURIs.find(transform) != 
                m_syntheticEventTypeURIs.end()) {
                stream << m_syntheticEventTypeURIs[transform] << " ;\n";
            } else {
                stream << ":event_type_" << outputId << " ;\n";
            }
        } else {
            stream << "<" << eventTypeURI << "> ;\n";
        }

        QString timestamp = feature.timestamp.toString().c_str();
        timestamp.replace(QRegExp("^ +"), "");

        if (feature.hasDuration && feature.duration > Vamp::RealTime::zeroTime) {

            QString duration = feature.duration.toString().c_str();
            duration.replace(QRegExp("^ +"), "");

            stream << "    event:time [ \n"
                   << "        a tl:Interval ;\n"
                   << "        tl:onTimeLine " << timelineURI << " ;\n"
                   << "        tl:beginsAt \"PT" << timestamp
                   << "S\"^^xsd:duration ;\n"
                   << "        tl:duration \"PT" << duration
                   << "S\"^^xsd:duration ;\n"
                   << "    ] ";

        } else {

            stream << "    event:time [ \n"
                   << "        a tl:Instant ;\n" //location of the event in time
                   << "        tl:onTimeLine " << timelineURI << " ;\n"
                   << "        tl:at \"PT" << timestamp
                   << "S\"^^xsd:duration ;\n    ] ";
        }

        if (transform.getIdentifier() != "") {
            stream << ";\n";
            stream << "    vamp:computed_by " << m_transformURIs[transform] << " ";
        }

        if (feature.label.length() > 0) {
            stream << ";\n";
            stream << "    rdfs:label \"" << feature.label.c_str() << "\" ";
        }

        if (!feature.values.empty()) {
            stream << ";\n";
            //!!! named bins?
            stream << "    af:feature \"" << feature.values[0];
            for (int j = 1; j < feature.values.size(); ++j) {
                stream << " " << feature.values[j];
            }
            stream << "\" ";
        }

        stream << ".\n";
    }
}

void
RDFFeatureWriter::writeDenseRDF(QTextStream *sptr,
                                const Transform &transform,
                                const Plugin::OutputDescriptor& od,
                                const Plugin::FeatureList& featureList,
                                PluginRDFDescription &desc,
                                QString signalURI, 
                                QString timelineURI)
{
    if (featureList.empty()) return;

    StringTransformPair sp(signalURI, transform);

    if (m_openDenseFeatures.find(sp) == m_openDenseFeatures.end()) {

        StreamBuffer b(sptr, "");
        m_openDenseFeatures[sp] = b;
        
        QString &str(m_openDenseFeatures[sp].second);
        QTextStream stream(&str);

        bool plain = (m_plain || !desc.haveDescription());
        QString outputId = od.identifier.c_str();

        uint64_t featureNumber = m_count++;

        // need to write out feature timeline map -- for this we need
        // the sample rate, window length and hop size from the
        // transform

        stream << "\n:feature_timeline_" << featureNumber << " a tl:DiscreteTimeLine .\n\n";

        size_t stepSize = transform.getStepSize();
        if (stepSize == 0) {
            cerr << "RDFFeatureWriter: INTERNAL ERROR: writing dense features without having set the step size properly!" << endl;
            return;
        }

        size_t blockSize = transform.getBlockSize();
        if (blockSize == 0) {
            cerr << "RDFFeatureWriter: INTERNAL ERROR: writing dense features without having set the block size properly!" << endl;
            return;
        }

        float sampleRate = transform.getSampleRate();
        if (sampleRate == 0.f) {
            cerr << "RDFFeatureWriter: INTERNAL ERROR: writing dense features without having set the sample rate properly!" << endl;
            return;
        }

        stream << ":feature_timeline_map_" << featureNumber
               << " a tl:UniformSamplingWindowingMap ;\n"
               << "    tl:rangeTimeLine :feature_timeline_" << featureNumber << " ;\n"
               << "    tl:domainTimeLine " << timelineURI << " ;\n"
               << "    tl:sampleRate \"" << int(sampleRate) << "\"^^xsd:int ;\n"
               << "    tl:windowLength \"" << blockSize << "\"^^xsd:int ;\n"
               << "    tl:hopSize \"" << stepSize << "\"^^xsd:int .\n\n";

        stream << signalURI << " af:signal_feature :feature_"
               << featureNumber << " ." << endl << endl;

        stream << ":feature_" << featureNumber << " a ";

        QString signalTypeURI = desc.getOutputSignalTypeURI(outputId);
        if (plain || signalTypeURI == "") {
            if (m_syntheticSignalTypeURIs.find(transform) !=
                m_syntheticSignalTypeURIs.end()) {
                stream << m_syntheticSignalTypeURIs[transform] << " ;\n";
            } else {
                stream << ":signal_type_" << outputId << " ;\n";
            }
        } else {
            stream << signalTypeURI << " ;\n";
        }

        stream << "    mo:time ["
               << "\n        a tl:Interval ;"
               << "\n        tl:onTimeLine :feature_timeline_" << featureNumber << " ;";

        RealTime startrt = transform.getStartTime();
        RealTime durationrt = transform.getDuration();

        int start = RealTime::realTime2Frame(startrt, sampleRate) / stepSize;
        int duration = RealTime::realTime2Frame(durationrt, sampleRate) / stepSize;

        if (start != 0) {
            stream << "\n        tl:start \"" << start << "\"^^xsd:int ;";
        }
        if (duration != 0) {
            stream << "\n        tl:duration \"" << duration << "\"^^xsd:int ;";
        }

        stream << "\n    ] ;\n";

        if (od.hasFixedBinCount) {
            // We only know the height, so write the width as zero
            stream << "    af:dimensions \"" << od.binCount << " 0\" ;\n";
        }

        stream << "    af:value \"";
    }

    QString &str = m_openDenseFeatures[sp].second;
    QTextStream stream(&str);

    for (int i = 0; i < featureList.size(); ++i) {

        const Plugin::Feature &feature = featureList[i];

        for (int j = 0; j < feature.values.size(); ++j) {
            stream << feature.values[j] << " ";
        }
    }
}

void RDFFeatureWriter::finish()
{
//    cerr << "RDFFeatureWriter::finish()" << endl;

    // close any open dense feature literals

    for (map<StringTransformPair, StreamBuffer>::iterator i =
             m_openDenseFeatures.begin();
         i != m_openDenseFeatures.end(); ++i) {
        cerr << "closing a stream" << endl;
        StreamBuffer &b = i->second;
        *(b.first) << b.second << "\" ." << endl;
    }

    m_openDenseFeatures.clear();
}


