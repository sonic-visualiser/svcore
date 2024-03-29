/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008-2012 QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RDFImporter.h"

#include <map>
#include <vector>

#include <iostream>
#include <cmath>

#include "base/ProgressReporter.h"
#include "base/RealTime.h"
#include "base/StringBits.h"

#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/SparseTimeValueModel.h"
#include "data/model/EditableDenseThreeDimensionalModel.h"
#include "data/model/NoteModel.h"
#include "data/model/TextModel.h"
#include "data/model/RegionModel.h"
#include "data/model/ReadOnlyWaveFileModel.h"

#include "data/fileio/FileSource.h"
#include "data/fileio/CachedFile.h"
#include "data/fileio/FileFinder.h"
#include "data/fileio/TextTest.h"

#include <QRegularExpression>

#include <dataquay/BasicStore.h>
#include <dataquay/PropertyObject.h>

namespace sv {

using Dataquay::Uri;
using Dataquay::Node;
using Dataquay::Nodes;
using Dataquay::Triple;
using Dataquay::Triples;
using Dataquay::BasicStore;
using Dataquay::PropertyObject;

class RDFImporterImpl
{
public:
    RDFImporterImpl(QString url, sv_samplerate_t sampleRate);
    virtual ~RDFImporterImpl();

    void setSampleRate(sv_samplerate_t sampleRate) { m_sampleRate = sampleRate; }
    
    bool isOK();
    QString getErrorString() const;

    std::vector<ModelId> getDataModels(ProgressReporter *);

protected:
    BasicStore *m_store;
    Uri expand(QString s) { return m_store->expand(s); }

    QString m_uristring;
    QString m_errorString;
    std::map<QString, ModelId> m_audioModelMap;
    sv_samplerate_t m_sampleRate;

    std::map<ModelId, std::map<QString, float> > m_labelValueMap;

    void getDataModelsAudio(std::vector<ModelId> &, ProgressReporter *);
    void getDataModelsSparse(std::vector<ModelId> &, ProgressReporter *);
    void getDataModelsDense(std::vector<ModelId> &, ProgressReporter *);

    QString getDenseModelTitle(QString featureUri, QString featureTypeUri);

    void getDenseFeatureProperties(QString featureUri,
                                   sv_samplerate_t &sampleRate, int &windowLength,
                                   int &hopSize, int &width, int &height);

    void fillModel(ModelId, sv_frame_t, sv_frame_t,
                   bool, std::vector<float> &, QString);
};

QString
RDFImporter::getKnownExtensions()
{
    return "*.rdf *.n3 *.ttl";
}

RDFImporter::RDFImporter(QString url, sv_samplerate_t sampleRate) :
    m_d(new RDFImporterImpl(url, sampleRate)) 
{
}

RDFImporter::~RDFImporter()
{
    delete m_d;
}

void
RDFImporter::setSampleRate(sv_samplerate_t sampleRate)
{
    m_d->setSampleRate(sampleRate);
}

bool
RDFImporter::isOK()
{
    return m_d->isOK();
}

QString
RDFImporter::getErrorString() const
{
    return m_d->getErrorString();
}

std::vector<ModelId>
RDFImporter::getDataModels(ProgressReporter *r)
{
    return m_d->getDataModels(r);
}

RDFImporterImpl::RDFImporterImpl(QString uri, sv_samplerate_t sampleRate) :
    m_store(new BasicStore),
    m_uristring(uri),
    m_sampleRate(sampleRate)
{
    //!!! retrieve data if remote... then

    m_store->addPrefix("mo", Uri("http://purl.org/ontology/mo/"));
    m_store->addPrefix("af", Uri("http://purl.org/ontology/af/"));
    m_store->addPrefix("dc", Uri("http://purl.org/dc/elements/1.1/"));
    m_store->addPrefix("tl", Uri("http://purl.org/NET/c4dm/timeline.owl#"));
    m_store->addPrefix("event", Uri("http://purl.org/NET/c4dm/event.owl#"));
    m_store->addPrefix("rdfs", Uri("http://www.w3.org/2000/01/rdf-schema#"));

    try {
        QUrl url;
        if (uri.startsWith("file:")) {
            url = QUrl(uri);
        } else {
            url = QUrl::fromLocalFile(uri);
        }
        m_store->import(url, BasicStore::ImportIgnoreDuplicates);
    } catch (std::exception &e) {
        m_errorString = e.what();
    }
}

RDFImporterImpl::~RDFImporterImpl()
{
    delete m_store;
}

bool
RDFImporterImpl::isOK()
{
    return (m_errorString == "");
}

QString
RDFImporterImpl::getErrorString() const
{
    return m_errorString;
}

std::vector<ModelId>
RDFImporterImpl::getDataModels(ProgressReporter *reporter)
{
    std::vector<ModelId> models;

    getDataModelsAudio(models, reporter);

    if (m_sampleRate == 0) {
        m_errorString = QString("Invalid audio data model (is audio file format supported?)");
        SVCERR << m_errorString << endl;
        return models;
    }

    QString error;

    if (m_errorString != "") {
        error = m_errorString;
    }
    m_errorString = "";

    getDataModelsDense(models, reporter);

    if (m_errorString != "") {
        error = m_errorString;
    }
    m_errorString = "";

    getDataModelsSparse(models, reporter);

    if (m_errorString == "" && error != "") {
        m_errorString = error;
    }

    return models;
}

void
RDFImporterImpl::getDataModelsAudio(std::vector<ModelId> &models,
                                    ProgressReporter *reporter)
{
    Nodes sigs = m_store->match
        (Triple(Node(), Uri("a"), expand("mo:Signal"))).subjects();

    foreach (Node sig, sigs) {
        
        Node file = m_store->complete(Triple(Node(), expand("mo:encodes"), sig));
        if (file == Node()) {
            file = m_store->complete(Triple(sig, expand("mo:available_as"), Node()));
        }
        if (file == Node()) {
            SVCERR << "RDFImporterImpl::getDataModelsAudio: ERROR: No source for signal " << sig << endl;
            continue;
        }

        QString signal = sig.value;
        QString source = file.value;

        SVDEBUG << "NOTE: Seeking signal source \"" << source
                << "\"..." << endl;

        FileSource *fs = new FileSource(source, reporter);
        if (fs->isAvailable()) {
            SVDEBUG << "NOTE: Source is available: Local filename is \""
                    << fs->getLocalFilename()
                    << "\"..." << endl;
        }
            
#ifdef NO_SV_GUI
        if (!fs->isAvailable()) {
            m_errorString = QString("Signal source \"%1\" is not available").arg(source);
            delete fs;
            continue;
        }
#else
        if (!fs->isAvailable()) {
            SVDEBUG << "NOTE: Signal source \"" << source
                    << "\" is not available, using file finder..." << endl;
            FileFinder *ff = FileFinder::getInstance();
            if (ff) {
                QString path = ff->find(FileFinder::AudioFile,
                                        fs->getLocation(),
                                        m_uristring);
                if (path != "") {
                    SVCERR << "File finder returns: \"" << path
                              << "\"" << endl;
                    delete fs;
                    fs = new FileSource(path, reporter);
                    if (!fs->isAvailable()) {
                        delete fs;
                        m_errorString = QString("Signal source \"%1\" is not available").arg(source);
                        continue;
                    }
                }
            }
        }
#endif

        if (reporter) {
            reporter->setMessage(RDFImporter::tr("Importing audio referenced in RDF..."));
        }
        fs->waitForData();
        auto newModel = std::make_shared<ReadOnlyWaveFileModel>
            (*fs, m_sampleRate);
        if (newModel->isOK()) {
            SVCERR << "Successfully created wave file model from source at \"" << source << "\"" << endl;
            auto modelId = ModelById::add(newModel);
            models.push_back(modelId);
            m_audioModelMap[signal] = modelId;
            if (m_sampleRate == 0) {
                m_sampleRate = newModel->getSampleRate();
            }
        } else {
            m_errorString = QString("Failed to create wave file model from source at \"%1\"").arg(source);
        }
        delete fs;
    }
}

void
RDFImporterImpl::getDataModelsDense(std::vector<ModelId> &models,
                                    ProgressReporter *reporter)
{
    if (reporter) {
        reporter->setMessage(RDFImporter::tr("Importing dense signal data from RDF..."));
    }

    Nodes sigFeatures = m_store->match
        (Triple(Node(), expand("af:signal_feature"), Node())).objects();

    foreach (Node sf, sigFeatures) {

        if (sf.type != Node::URI && sf.type != Node::Blank) continue;
        
        Node t = m_store->complete(Triple(sf, expand("a"), Node()));
        Node v = m_store->complete(Triple(sf, expand("af:value"), Node()));

        QString feature = sf.value;
        QString type = t.value;
        QString value = v.value;
        
        if (type == "" || value == "") continue;

        sv_samplerate_t sampleRate = 0;
        int windowLength = 0;
        int hopSize = 0;
        int width = 0;
        int height = 0;
        getDenseFeatureProperties
            (feature, sampleRate, windowLength, hopSize, width, height);

        if (sampleRate != 0 && sampleRate != m_sampleRate) {
            SVCERR << "WARNING: Sample rate in dense feature description does not match our underlying rate -- using rate from feature description" << endl;
        }
        if (sampleRate == 0) sampleRate = m_sampleRate;

        if (hopSize == 0) {
            SVCERR << "WARNING: Dense feature description does not specify a hop size -- assuming 1" << endl;
            hopSize = 1;
        }

        if (height == 0) {
            SVCERR << "WARNING: Dense feature description does not specify feature signal dimensions -- assuming one-dimensional (height = 1)" << endl;
            height = 1;
        }

        QStringList values = value.split(' ', Qt::SkipEmptyParts);

        if (values.empty()) {
            SVCERR << "WARNING: Dense feature description does not specify any values!" << endl;
            continue;
        }

        if (height == 1) {

            auto m = std::make_shared<SparseTimeValueModel>
                (sampleRate, hopSize, false);

            for (int j = 0; j < values.size(); ++j) {
                float f = values[j].toFloat();
                Event e(j * hopSize, f, "");
                m->add(e);
            }

            m->setObjectName(getDenseModelTitle(feature, type));
            m->setRDFTypeURI(type);
            models.push_back(ModelById::add(m));

        } else {

            auto m = std::make_shared<EditableDenseThreeDimensionalModel>
                (sampleRate, hopSize, height, false);
            
            EditableDenseThreeDimensionalModel::Column column;

            int x = 0;

            for (int j = 0; j < values.size(); ++j) {
                if (j % height == 0 && !column.empty()) {
                    m->setColumn(x++, column);
                    column.clear();
                }
                column.push_back(values[j].toFloat());
            }

            if (!column.empty()) {
                m->setColumn(x++, column);
            }

            m->setObjectName(getDenseModelTitle(feature, type));
            m->setRDFTypeURI(type);
            models.push_back(ModelById::add(m));
        }
    }
}

QString
RDFImporterImpl::getDenseModelTitle(QString featureUri,
                                    QString featureTypeUri)
{
    Node n = m_store->complete
        (Triple(Uri(featureUri), expand("dc:title"), Node()));

    if (n.type == Node::Literal && n.value != "") {
        SVDEBUG << "RDFImporterImpl::getDenseModelTitle: Title (from signal) \"" << n.value << "\"" << endl;
        return n.value;
    }

    n = m_store->complete
        (Triple(Uri(featureTypeUri), expand("dc:title"), Node()));

    if (n.type == Node::Literal && n.value != "") {
        SVDEBUG << "RDFImporterImpl::getDenseModelTitle: Title (from signal type) \"" << n.value << "\"" << endl;
        return n.value;
    }

    SVDEBUG << "RDFImporterImpl::getDenseModelTitle: No title available for feature <" << featureUri << ">" << endl;
    return {};
}

void
RDFImporterImpl::getDenseFeatureProperties(QString featureUri,
                                           sv_samplerate_t &sampleRate, int &windowLength,
                                           int &hopSize, int &width, int &height)
{
    Node dim = m_store->complete
        (Triple(Uri(featureUri), expand("af:dimensions"), Node()));

    SVCERR << "Dimensions = \"" << dim.value << "\"" << endl;

    if (dim.type == Node::Literal && dim.value != "") {
        QStringList dl = dim.value.split(" ");
        if (dl.empty()) dl.push_back(dim.value);
        if (dl.size() > 0) height = dl[0].toInt();
        if (dl.size() > 1) width = dl[1].toInt();
    }
    
    // Looking for rate, hop, window from:
    //
    // ?feature mo:time ?time .
    // ?time a tl:Interval .
    // ?time tl:onTimeLine ?timeline .
    // ?map tl:rangeTimeLine ?timeline .
    // ?map tl:sampleRate ?rate .
    // ?map tl:hopSize ?hop .
    // ?map tl:windowLength ?window .

    Node interval = m_store->complete(Triple(Uri(featureUri), expand("mo:time"), Node()));

    if (!m_store->contains(Triple(interval, expand("a"), expand("tl:Interval")))) {
        SVCERR << "RDFImporterImpl::getDenseFeatureProperties: Feature time node "
             << interval << " is not a tl:Interval" << endl;
        return;
    }

    Node tl = m_store->complete(Triple(interval, expand("tl:onTimeLine"), Node()));
    
    if (tl == Node()) {
        SVCERR << "RDFImporterImpl::getDenseFeatureProperties: Interval node "
             << interval << " lacks tl:onTimeLine property" << endl;
        return;
    }

    Node map = m_store->complete(Triple(Node(), expand("tl:rangeTimeLine"), tl));
    
    if (map == Node()) {
        SVCERR << "RDFImporterImpl::getDenseFeatureProperties: No map for "
             << "timeline node " << tl << endl;
    }

    PropertyObject po(m_store, "tl:", map);

    if (po.hasProperty("sampleRate")) {
        sampleRate = po.getProperty("sampleRate").toDouble();
    }
    if (po.hasProperty("hopSize")) {
        hopSize = po.getProperty("hopSize").toInt();
    }
    if (po.hasProperty("windowLength")) {
        windowLength = po.getProperty("windowLength").toInt();
    }

    SVCERR << "sr = " << sampleRate << ", hop = " << hopSize << ", win = " << windowLength << endl;
}

void
RDFImporterImpl::getDataModelsSparse(std::vector<ModelId> &models,
                                     ProgressReporter *reporter)
{
    if (reporter) {
        reporter->setMessage(RDFImporter::tr("Importing event data from RDF..."));
    }

    /*
      This function is only used for sparse data (for dense data we
      would be in getDataModelsDense instead).

      Our query is intended to retrieve every thing that has a time,
      and every feature type and value associated with a thing that
      has a time.

      We will then need to refine this big bag of results into a set
      of data models.

      Results that have different source signals should go into
      different models.

      Results that have different feature types should go into
      different models.
    */

    Nodes sigs = m_store->match
        (Triple(Node(), expand("a"), expand("mo:Signal"))).subjects();

    // Map from timeline uri to event type to dimensionality to
    // presence of duration to model id.  Whee!
    std::map<QString, std::map<QString, std::map<int, std::map<bool, ModelId> > > >
        modelMap;

    foreach (Node sig, sigs) {
        
        Node interval = m_store->complete(Triple(sig, expand("mo:time"), Node()));
        if (interval == Node()) continue;

        Node tl = m_store->complete(Triple(interval, expand("tl:onTimeLine"), Node()));
        if (tl == Node()) continue;

        Nodes times = m_store->match(Triple(Node(), expand("tl:onTimeLine"), tl)).subjects();
        
        foreach (Node tn, times) {
            
            Nodes timedThings = m_store->match(Triple(Node(), expand("event:time"), tn)).subjects();

            foreach (Node thing, timedThings) {
                
                Node typ = m_store->complete(Triple(thing, expand("a"), Node()));
                if (typ == Node()) continue;

                Node valu = m_store->complete(Triple(thing, expand("af:feature"), Node()));

                QString source = sig.value;
                QString timeline = tl.value;
                QString type = typ.value;
                QString thinguri = thing.value;

                /*
                  For sparse data, the determining factors in deciding
                  what model to use are: Do the features have values?
                  and Do the features have duration?

                  We can run through the results and check off whether
                  we find values and duration for each of the
                  source+type keys, and then run through the
                  source+type keys pushing each of the results into a
                  suitable model.

                  Unfortunately, at this point we do not yet have any
                  actual timing data (time/duration) -- just the time
                  URI.

                  What we _could_ do is to create one of each type of
                  model at the start, for each of the source+type
                  keys, and then push each feature into the relevant
                  model depending on what we find out about it.  Then
                  return only non-empty models.
                */

                QString label = "";
                bool text = (type.contains("Text") || type.contains("text")); // Ha, ha
                bool note = (type.contains("Note") || type.contains("note")); // Guffaw

                if (text) {
                    label = m_store->complete(Triple(thing, expand("af:text"), Node())).value;
                }
                
                if (label == "") {
                    label = m_store->complete(Triple(thing, expand("rdfs:label"), Node())).value;
                }

                RealTime time;
                RealTime duration;

//                bool haveTime = false;
                bool haveDuration = false;

                Node at = m_store->complete(Triple(tn, expand("tl:at"), Node()));

                if (at != Node()) {
                    time = RealTime::fromXsdDuration(at.value.toStdString());
//                    haveTime = true;
                } else {
    //!!! NB we're using rather old terminology for these things, apparently:
    // beginsAt -> start
    // onTimeLine -> timeline

                    Node start = m_store->complete(Triple(tn, expand("tl:beginsAt"), Node()));
                    Node dur = m_store->complete(Triple(tn, expand("tl:duration"), Node()));
                    if (start != Node() && dur != Node()) {
                        time = RealTime::fromXsdDuration
                            (start.value.toStdString());
                        duration = RealTime::fromXsdDuration
                            (dur.value.toStdString());
//                        haveTime = haveDuration = true;
                    }
                }

                QString valuestring = valu.value;
                std::vector<float> values;

                if (valuestring != "") {
                    QStringList vsl = valuestring.split(" ", Qt::SkipEmptyParts);
                    for (int j = 0; j < vsl.size(); ++j) {
                        bool success = false;
                        float v = vsl[j].toFloat(&success);
                        if (success) values.push_back(v);
                    }
                }
                
                int dimensions = 1;
                if (values.size() == 1) dimensions = 2;
                else if (values.size() > 1) dimensions = 3;

                ModelId modelId;

                if (modelMap[timeline][type][dimensions].find(haveDuration) ==
                    modelMap[timeline][type][dimensions].end()) {

/*
            SVDEBUG << "Creating new model: source = " << source                      << ", type = " << type << ", dimensions = "
                      << dimensions << ", haveDuration = " << haveDuration
                      << ", time = " << time << ", duration = " << duration
                      << endl;
*/

                    Model *model = nullptr;
                    
                    if (!haveDuration) {

                        if (dimensions == 1) {
                            if (text) {
                                model = new TextModel(m_sampleRate, 1, false);
                            } else {
                                model = new SparseOneDimensionalModel(m_sampleRate, 1, false);
                            }
                        } else if (dimensions == 2) {
                            if (text) {
                                model = new TextModel(m_sampleRate, 1, false);
                            } else {
                                model = new SparseTimeValueModel(m_sampleRate, 1, false);
                            }
                        } else {
                            // We don't have a three-dimensional sparse model,
                            // so use a note model.  We do have some logic (in
                            // extractStructure below) for guessing whether
                            // this should after all have been a dense model,
                            // but it's hard to apply it because we don't have
                            // all the necessary timing data yet... hmm
                            model = new NoteModel(m_sampleRate, 1, false);
                        }

                    } else { // haveDuration

                        if (note || (dimensions > 2)) {
                            model = new NoteModel(m_sampleRate, 1, false);
                        } else {
                            // If our units are frequency or midi pitch, we
                            // should be using a note model... hm
                            model = new RegionModel(m_sampleRate, 1, false);
                        }
                    }

                    model->setRDFTypeURI(type);

                    if (m_audioModelMap.find(source) != m_audioModelMap.end()) {
                        SVCERR << "source model for " << model << " is " << m_audioModelMap[source] << endl;
                        model->setSourceModel(m_audioModelMap[source]);
                    }

                    QString title = m_store->complete
                        (Triple(typ, expand("dc:title"), Node())).value;
                    if (title == "") {
                        // take it from the end of the event type
                        title = type;
                        title.replace(QRegularExpression("^.*[/#]"), "");
                    }
                    model->setObjectName(title);

                    modelId = ModelById::add(std::shared_ptr<Model>(model));
                    modelMap[timeline][type][dimensions][haveDuration] = modelId;
                    models.push_back(modelId);
                }

                modelId = modelMap[timeline][type][dimensions][haveDuration];

                if (!modelId.isNone()) {
                    sv_frame_t ftime =
                        RealTime::realTime2Frame(time, m_sampleRate);
                    sv_frame_t fduration =
                        RealTime::realTime2Frame(duration, m_sampleRate);
                    fillModel(modelId, ftime, fduration,
                              haveDuration, values, label);
                }
            }
        }
    }
}

void
RDFImporterImpl::fillModel(ModelId modelId,
                           sv_frame_t ftime,
                           sv_frame_t fduration,
                           bool haveDuration,
                           std::vector<float> &values,
                           QString label)
{
//    SVDEBUG << "RDFImporterImpl::fillModel: adding point at frame " << ftime << endl;

    if (auto sodm = ModelById::getAs<SparseOneDimensionalModel>(modelId)) {
        Event point(ftime, label);
        sodm->add(point);
        return;
    }

    if (auto tm = ModelById::getAs<TextModel>(modelId)) {
        Event e
            (ftime,
             values.empty() ? 0.5f : values[0] < 0.f ? 0.f : values[0] > 1.f ? 1.f : values[0], // I was young and feckless once too
             label);
        tm->add(e);
        return;
    }

    if (auto stvm = ModelById::getAs<SparseTimeValueModel>(modelId)) {
        Event e(ftime, values.empty() ? 0.f : values[0], label);
        stvm->add(e);
        return;
    }

    if (auto nm = ModelById::getAs<NoteModel>(modelId)) {
        if (haveDuration) {
            float value = 0.f, level = 1.f;
            if (!values.empty()) {
                value = values[0];
                if (values.size() > 1) {
                    level = values[1];
                }
            }
            Event e(ftime, value, fduration, level, label);
            nm->add(e);
        } else {
            float value = 0.f, duration = 1.f, level = 1.f;
            if (!values.empty()) {
                value = values[0];
                if (values.size() > 1) {
                    duration = values[1];
                    if (values.size() > 2) {
                        level = values[2];
                    }
                }
            }
            Event e(ftime, value, sv_frame_t(lrintf(duration)),
                        level, label);
            nm->add(e);
        }
        return;
    }

    if (auto rm = ModelById::getAs<RegionModel>(modelId)) {
        float value = 0.f;
        if (values.empty()) {
            // no values? map each unique label to a distinct value
            if (m_labelValueMap[modelId].find(label) == m_labelValueMap[modelId].end()) {
                m_labelValueMap[modelId][label] = rm->getValueMaximum() + 1.f;
            }
            value = m_labelValueMap[modelId][label];
        } else {
            value = values[0];
        }
        if (haveDuration) {
            Event e(ftime, value, fduration, label);
            rm->add(e);
        } else {
            // This won't actually happen -- we only create region models
            // if we do have duration -- but just for completeness
            float duration = 1.f;
            if (!values.empty()) {
                value = values[0];
                if (values.size() > 1) {
                    duration = values[1];
                }
            }
            Event e(ftime, value, sv_frame_t(lrintf(duration)), label);
            rm->add(e);
        }
        return;
    }
            
    SVCERR << "WARNING: RDFImporterImpl::fillModel: Unknown or unexpected model type" << endl;
    return;
}

RDFImporter::RDFDocumentType
RDFImporter::identifyDocumentType(QUrl url)
{
    bool haveAudio = false;
    bool haveAnnotations = false;
    bool haveRDF = false;

    if (!isPlausibleDocumentOfAnyKind(url)) {
        return NotRDF;
    }

    BasicStore *store = nullptr;
    
    // This is not expected to return anything useful, but if it does
    // anything at all then we know we have RDF
    try {
        store = BasicStore::load(url);
        Triple t = store->matchOnce(Triple());
        if (t != Triple()) haveRDF = true;
    } catch (std::exception &) {
        // nothing; haveRDF will be false so the next bit catches it
    }

    if (!haveRDF) {
        delete store;
        return NotRDF;
    }

    store->addPrefix("mo", Uri("http://purl.org/ontology/mo/"));
    store->addPrefix("event", Uri("http://purl.org/NET/c4dm/event.owl#"));
    store->addPrefix("af", Uri("http://purl.org/ontology/af/"));

    // "MO-conformant" structure for audio files

    Node n = store->complete(Triple(Node(), Uri("a"), store->expand("mo:AudioFile")));
    if (n != Node() && n.type == Node::URI) {

        haveAudio = true;

    } else {

        // Sonic Annotator v0.2 and below used to write this structure
        // (which is not properly in conformance with the Music
        // Ontology)

        Nodes sigs = store->match(Triple(Node(), Uri("a"), store->expand("mo:Signal"))).subjects();
        foreach (Node sig, sigs) {
            Node aa = store->complete(Triple(sig, store->expand("mo:available_as"), Node()));
            if (aa != Node()) {
                haveAudio = true;
                break;
            }
        }
    }

    SVDEBUG << "NOTE: RDFImporter::identifyDocumentType: haveAudio = "
              << haveAudio << endl;

    // can't call complete() with two Nothing nodes
    n = store->matchOnce(Triple(Node(), store->expand("event:time"), Node())).c;
    if (n != Node()) {
        haveAnnotations = true;
    }

    if (!haveAnnotations) {
        // can't call complete() with two Nothing nodes
        n = store->matchOnce(Triple(Node(), store->expand("af:signal_feature"), Node())).c;
        if (n != Node()) {
            haveAnnotations = true;
        }
    }

    SVDEBUG << "NOTE: RDFImporter::identifyDocumentType: haveAnnotations = "
              << haveAnnotations << endl;

    delete store;

    if (haveAudio) {
        if (haveAnnotations) {
            return AudioRefAndAnnotations;
        } else {
            return AudioRef;
        }
    } else {
        if (haveAnnotations) {
            return Annotations;
        } else {
            return OtherRDFDocument;
        }
    }

    return OtherRDFDocument;
}

bool
RDFImporter::isPlausibleDocumentOfAnyKind(QUrl url)
{
    return TextTest::isApparentTextDocument(FileSource(url));
}

} // end namespace sv

