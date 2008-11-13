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

#include "RDFImporter.h"

#include <map>
#include <vector>

#include <iostream>
#include <cmath>

#include "SimpleSPARQLQuery.h"

#include "base/ProgressReporter.h"
#include "base/RealTime.h"

#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/SparseTimeValueModel.h"
#include "data/model/EditableDenseThreeDimensionalModel.h"
#include "data/model/NoteModel.h"
#include "data/model/RegionModel.h"

using std::cerr;
using std::endl;

class RDFImporterImpl
{
public:
    RDFImporterImpl(QString url, int sampleRate);
    virtual ~RDFImporterImpl();
    
    bool isOK();
    QString getErrorString() const;

    std::vector<Model *> getDataModels(ProgressReporter *);

protected:
    QString m_uristring;
    QString m_errorString;
    int m_sampleRate;

    void getDataModelsSparse(std::vector<Model *> &, ProgressReporter *);
    void getDataModelsDense(std::vector<Model *> &, ProgressReporter *);

    void getDenseFeatureProperties(QString featureUri,
                                   int &sampleRate, int &windowLength,
                                   int &hopSize, int &width, int &height);


    void fillModel(Model *, long, long, bool, std::vector<float> &, QString);

/*

    typedef std::vector<std::pair<RealTime, float> > DurationValueList;
    typedef std::map<RealTime, DurationValueList> TimeDurationValueMap;
    typedef std::map<QString, TimeDurationValueMap> TypeTimeDurationValueMap;
    typedef std::map<QString, TypeTimeDurationValueMap> SourceTypeTimeDurationValueMap;

    void extractStructure(const TimeDurationValueMap &map, bool &sparse,
                          int &minValueCount, int &maxValueCount);

    void fillModel(SparseOneDimensionalModel *, const TimeDurationValueMap &);
    void fillModel(SparseTimeValueModel *, const TimeDurationValueMap &);
    void fillModel(EditableDenseThreeDimensionalModel *, const TimeDurationValueMap &);
*/
};


QString
RDFImporter::getKnownExtensions()
{
    return "*.rdf *.n3 *.ttl";
}

RDFImporter::RDFImporter(QString url, int sampleRate) :
    m_d(new RDFImporterImpl(url, sampleRate)) 
{
}

RDFImporter::~RDFImporter()
{
    delete m_d;
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

std::vector<Model *>
RDFImporter::getDataModels(ProgressReporter *r)
{
    return m_d->getDataModels(r);
}

RDFImporterImpl::RDFImporterImpl(QString uri, int sampleRate) :
    m_uristring(uri),
    m_sampleRate(sampleRate)
{
}

RDFImporterImpl::~RDFImporterImpl()
{
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

std::vector<Model *>
RDFImporterImpl::getDataModels(ProgressReporter *reporter)
{
    std::vector<Model *> models;

    getDataModelsDense(models, reporter);

    QString error;
    if (!isOK()) error = m_errorString;
    m_errorString = "";

    getDataModelsSparse(models, reporter);

    if (isOK()) m_errorString = error;

    return models;
}

void
RDFImporterImpl::getDataModelsDense(std::vector<Model *> &models,
                                    ProgressReporter *reporter)
{
    SimpleSPARQLQuery query = SimpleSPARQLQuery
        (m_uristring,
         QString
         (
             " PREFIX mo: <http://purl.org/ontology/mo/>"
             " PREFIX af: <http://purl.org/ontology/af/>"
             
             " SELECT ?feature ?signal_source ?feature_signal_type ?value "
             " FROM <%1> "
             
             " WHERE { "
             
             "   ?signal a mo:Signal ; "
             "           mo:available_as ?signal_source ; "
             "           af:signal_feature ?feature . "
             
             "   ?feature a ?feature_signal_type ; "
             "            af:value ?value . "
    
             " } "
             )
         .arg(m_uristring));

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        m_errorString = query.getErrorString();
        return;
    }

    if (query.wasCancelled()) {
        m_errorString = "Query cancelled";
        return;
    }        

    for (int i = 0; i < results.size(); ++i) {

        QString feature = results[i]["feature"].value;
        QString source = results[i]["signal_source"].value;
        QString type = results[i]["feature_signal_type"].value;
        QString value = results[i]["value"].value;

        int sampleRate = 0;
        int windowLength = 0;
        int hopSize = 0;
        int width = 0;
        int height = 0;
        getDenseFeatureProperties
            (feature, sampleRate, windowLength, hopSize, width, height);

        if (sampleRate != 0 && sampleRate != m_sampleRate) {
            cerr << "WARNING: Sample rate in dense feature description does not match our underlying rate -- using rate from feature description" << endl;
        }
        if (sampleRate == 0) sampleRate = m_sampleRate;

        if (hopSize == 0) {
            cerr << "WARNING: Dense feature description does not specify a hop size -- assuming 1" << endl;
            hopSize = 1;
        }

        if (height == 0) {
            cerr << "WARNING: Dense feature description does not specify feature signal dimensions -- assuming one-dimensional (height = 1)" << endl;
            height = 1;
        }

        QStringList values = value.split(' ', QString::SkipEmptyParts);

        if (values.empty()) {
            cerr << "WARNING: Dense feature description does not specify any values!" << endl;
            continue;
        }

        if (height == 1) {

            SparseTimeValueModel *m = new SparseTimeValueModel
                (sampleRate, hopSize, false);

            for (int j = 0; j < values.size(); ++j) {
                float f = values[j].toFloat();
                SparseTimeValueModel::Point point(j * hopSize, f, "");
                m->addPoint(point);
            }
        
            models.push_back(m);

        } else {

            EditableDenseThreeDimensionalModel *m =
                new EditableDenseThreeDimensionalModel(sampleRate, hopSize,
                                                       height, false);
            
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

            models.push_back(m);
        }
    }
}

void
RDFImporterImpl::getDenseFeatureProperties(QString featureUri,
                                           int &sampleRate, int &windowLength,
                                           int &hopSize, int &width, int &height)
{
    QString dimensionsQuery 
        (
            " PREFIX mo: <http://purl.org/ontology/mo/>"
            " PREFIX af: <http://purl.org/ontology/af/>"
            
            " SELECT ?dimensions "
            " FROM <%1> "

            " WHERE { "

            "   <%2> af:dimensions ?dimensions . "
            
            " } "
            );

    SimpleSPARQLQuery::Value dimensionsValue =
        SimpleSPARQLQuery::singleResultQuery(m_uristring,
                                             dimensionsQuery
                                             .arg(m_uristring).arg(featureUri),
                                             "dimensions");

    cerr << "Dimensions = \"" << dimensionsValue.value.toStdString() << "\""
         << endl;

    if (dimensionsValue.value != "") {
        QStringList dl = dimensionsValue.value.split(" ");
        if (dl.empty()) dl.push_back(dimensionsValue.value);
        if (dl.size() > 0) height = dl[0].toInt();
        if (dl.size() > 1) width = dl[1].toInt();
    }

    QString queryTemplate
        (
            " PREFIX mo: <http://purl.org/ontology/mo/>"
            " PREFIX af: <http://purl.org/ontology/af/>"
            " PREFIX tl: <http://purl.org/NET/c4dm/timeline.owl#>"

            " SELECT ?%3 "
            " FROM <%1> "
            
            " WHERE { "
            
            "   <%2> mo:time ?time . "
            
            "   ?time a tl:Interval ; "
            "         tl:onTimeLine ?timeline . "

            "   ?map tl:rangeTimeLine ?timeline . "

            "   ?map tl:%3 ?%3 . "
            
            " } "
            );

    // Another laborious workaround for rasqal's failure to handle
    // multiple optionals properly

    SimpleSPARQLQuery::Value srValue = 
        SimpleSPARQLQuery::singleResultQuery(m_uristring,
                                             queryTemplate
                                             .arg(m_uristring).arg(featureUri)
                                             .arg("sampleRate"),
                                             "sampleRate");
    if (srValue.value != "") {
        sampleRate = srValue.value.toInt();
    }

    SimpleSPARQLQuery::Value hopValue = 
        SimpleSPARQLQuery::singleResultQuery(m_uristring,
                                             queryTemplate
                                             .arg(m_uristring).arg(featureUri)
                                             .arg("hopSize"),
                                             "hopSize");
    if (srValue.value != "") {
        hopSize = hopValue.value.toInt();
    }

    SimpleSPARQLQuery::Value winValue = 
        SimpleSPARQLQuery::singleResultQuery(m_uristring,
                                             queryTemplate
                                             .arg(m_uristring).arg(featureUri)
                                             .arg("windowLength"),
                                             "windowLength");
    if (winValue.value != "") {
        windowLength = winValue.value.toInt();
    }

    cerr << "sr = " << sampleRate << ", hop = " << hopSize << ", win = " << windowLength << endl;
}

void
RDFImporterImpl::getDataModelsSparse(std::vector<Model *> &models,
                                     ProgressReporter *reporter)
{
    // Our query is intended to retrieve every thing that has a time,
    // and every feature type and value associated with a thing that
    // has a time.

    // We will then need to refine this big bag of results into a set
    // of data models.

    // Results that have different source signals should go into
    // different models.

    // Results that have different feature types should go into
    // different models.

    // Results that are sparse should go into different models from
    // those that are dense (we need to examine the timestamps to
    // establish this -- if the timestamps are regular, the results
    // are dense -- so we can't do it as we go along, only after
    // collecting all results).

    // Timed things that have features associated with them should not
    // appear directly in any model -- their features should appear
    // instead -- and these should be different models from those used
    // for timed things that do not have features.

    // As we load the results, we'll push them into a partially
    // structured container that maps from source signal (URI as
    // string) -> feature type (likewise) -> time -> list of values.
    // If the source signal or feature type is unavailable, the empty
    // string will do.

//    SourceTypeTimeDurationValueMap m;

    QString prefixes = QString(
        " PREFIX event: <http://purl.org/NET/c4dm/event.owl#>"
        " PREFIX tl: <http://purl.org/NET/c4dm/timeline.owl#>"
        " PREFIX mo: <http://purl.org/ontology/mo/>"
        " PREFIX af: <http://purl.org/ontology/af/>"
        " PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>"
        );

    QString queryString = prefixes + QString(

        " SELECT ?signal_source ?timed_thing ?event_type ?value"
        " FROM <%1>"

        " WHERE {"

        "   ?signal mo:available_as ?signal_source ."
        "   ?signal a mo:Signal ."

        "   ?signal mo:time ?interval ."
        "   ?interval tl:onTimeLine ?tl ."
        "   ?time tl:onTimeLine ?tl ."
        "   ?timed_thing event:time ?time ."
        "   ?timed_thing a ?event_type ."

        "   OPTIONAL {"
        "     ?timed_thing af:feature ?value"
        "   }"
        " }"

        ).arg(m_uristring);

    QString timeQueryString = prefixes + QString(
        
        " SELECT ?time FROM <%1> "
        " WHERE { "        
        "   <%2> event:time ?t . "
        "   ?t tl:at ?time . "
        " } "

        ).arg(m_uristring);

    QString rangeQueryString = prefixes + QString(
        
        " SELECT ?time ?duration FROM <%1> "
        " WHERE { "
        "   <%2> event:time ?t . "
        "   ?t tl:beginsAt ?time . "
        "   ?t tl:duration ?duration . "
        " } "

        ).arg(m_uristring);

    QString labelQueryString = prefixes + QString(
        
        " SELECT ?label FROM <%1> "
        " WHERE { "
        "   <%2> rdfs:label ?label . "
        " } "

        ).arg(m_uristring);

    SimpleSPARQLQuery query(m_uristring, queryString);
    query.setProgressReporter(reporter);

    cerr << "Query will be: " << queryString.toStdString() << endl;

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        m_errorString = query.getErrorString();
        return;
    }

    if (query.wasCancelled()) {
        m_errorString = "Query cancelled";
        return;
    }        



    /*

      This function is now only used for sparse data (for dense data
      we would be in getDataModelsDense instead).

      For sparse data, the determining factors in deciding what model
      to use are: Do the features have values? and Do the features
      have duration?

      We can run through the results and check off whether we find
      values and duration for each of the source+type keys, and then
      run through the source+type keys pushing each of the results
      into a suitable model.

      Unfortunately, at this point we do not yet have any actual
      timing data (time/duration) -- just the time URI.

      What we _could_ do is to create one of each type of model at the
      start, for each of the source+type keys, and then push each
      feature into the relevant model depending on what we find out
      about it.  Then return only non-empty models.

      
    */

    // Map from signal source to event type to dimensionality to
    // presence of duration to model ptr.  Whee!
    std::map<QString, std::map<QString, std::map<int, std::map<bool, Model *> > > >
        modelMap;

    for (int i = 0; i < results.size(); ++i) {

        QString source = results[i]["signal_source"].value;
        QString type = results[i]["event_type"].value;
        QString thinguri = results[i]["timed_thing"].value;
        
        RealTime time;
        RealTime duration;

        bool haveTime = false;
        bool haveDuration = false;

        QString label = SimpleSPARQLQuery::singleResultQuery
            (m_uristring, labelQueryString.arg(thinguri), "label").value;

        SimpleSPARQLQuery rangeQuery(m_uristring, rangeQueryString.arg(thinguri));
        SimpleSPARQLQuery::ResultList rangeResults = rangeQuery.execute();
        if (!rangeResults.empty()) {
//                std::cerr << rangeResults.size() << " range results" << std::endl;
            time = RealTime::fromXsdDuration
                (rangeResults[0]["time"].value.toStdString());
            duration = RealTime::fromXsdDuration
                (rangeResults[0]["duration"].value.toStdString());
//                std::cerr << "duration string " << rangeResults[0]["duration"].value.toStdString() << std::endl;
            haveTime = true;
            haveDuration = true;
        } else {
            QString timestring = SimpleSPARQLQuery::singleResultQuery
                (m_uristring, timeQueryString.arg(thinguri), "time").value;
            if (timestring != "") {
                time = RealTime::fromXsdDuration(timestring.toStdString());
                haveTime = true;
            }
        }

        QString valuestring = results[i]["value"].value;
        std::vector<float> values;

        if (valuestring != "") {
            QStringList vsl = valuestring.split(" ", QString::SkipEmptyParts);
            for (int j = 0; j < vsl.size(); ++j) {
                bool success = false;
                float v = vsl[j].toFloat(&success);
                if (success) values.push_back(v);
            }
        }

        int dimensions = 1;
        if (values.size() == 1) dimensions = 2;
        else if (values.size() > 1) dimensions = 3;

        Model *model = 0;

        if (modelMap[source][type][dimensions].find(haveDuration) ==
            modelMap[source][type][dimensions].end()) {

/*
            std::cerr << "Creating new model: source = " << source.toStdString()
                      << ", type = " << type.toStdString() << ", dimensions = "
                      << dimensions << ", haveDuration = " << haveDuration
                      << ", time = " << time << ", duration = " << duration
                      << std::endl;
*/
            
            if (!haveDuration) {

                if (dimensions == 1) {

//                    std::cerr << "SparseOneDimensionalModel" << std::endl;
                    model = new SparseOneDimensionalModel(m_sampleRate, 1, false);

                } else if (dimensions == 2) {

//                    std::cerr << "SparseTimeValueModel" << std::endl;
                    model = new SparseTimeValueModel(m_sampleRate, 1, false);

                } else {

                    // We don't have a three-dimensional sparse model,
                    // so use a note model.  We do have some logic (in
                    // extractStructure below) for guessing whether
                    // this should after all have been a dense model,
                    // but it's hard to apply it because we don't have
                    // all the necessary timing data yet... hmm

//                    std::cerr << "NoteModel" << std::endl;
                    model = new NoteModel(m_sampleRate, 1, false);
                }

            } else { // haveDuration

                if (dimensions == 1 || dimensions == 2) {

                    // If our units are frequency or midi pitch, we
                    // should be using a note model... hm
                    
//                    std::cerr << "RegionModel" << std::endl;
                    model = new RegionModel(m_sampleRate, 1, false);

                } else {

                    // We don't have a three-dimensional sparse model,
                    // so use a note model.  We do have some logic (in
                    // extractStructure below) for guessing whether
                    // this should after all have been a dense model,
                    // but it's hard to apply it because we don't have
                    // all the necessary timing data yet... hmm

//                    std::cerr << "NoteModel" << std::endl;
                    model = new NoteModel(m_sampleRate, 1, false);
                }
            }

            modelMap[source][type][dimensions][haveDuration] = model;
            models.push_back(model);
        }

        model = modelMap[source][type][dimensions][haveDuration];

        if (model) {
            long ftime = RealTime::realTime2Frame(time, m_sampleRate);
            long fduration = RealTime::realTime2Frame(duration, m_sampleRate);
            fillModel(model, ftime, fduration, haveDuration, values, label);
        }
    }
    

/*
    for (SourceTypeTimeDurationValueMap::const_iterator mi = m.begin();
         mi != m.end(); ++mi) {
        
        QString source = mi->first;

        for (TypeTimeDurationValueMap::const_iterator ttvi = mi->second.begin();
             ttvi != mi->second.end(); ++ttvi) {
            
            QString type = ttvi->first;

            // Now we need to work out what sort of model to use for
            // this source/type combination.  Ultimately we'll
            // hopefully be able to map directly from the type to the
            // model on the basis of known structures for the types,
            // but we also want to be able to handle untyped data
            // according to its apparent structure so let's do that
            // first.

            bool sparse = false;
            int minValueCount = 0, maxValueCount = 0;

            extractStructure(ttvi->second, sparse, minValueCount, maxValueCount);
    
            cerr << "For source \"" << source.toStdString() << "\", type \""
                 << type.toStdString() << "\" we have sparse = " << sparse
                 << ", min value count = " << minValueCount << ", max = "
                 << maxValueCount << endl;

            // Model allocations:
            //
            // Sparse, no values: SparseOneDimensionalModel
            //
            // Sparse, always 1 value: SparseTimeValueModel
            //
            // Sparse, > 1 value: No standard model for this.  If
            // there are always 2 values, perhaps hack it into
            // NoteModel for now?  Or always use SparseTimeValueModel
            // and discard all but the first value.
            //
            // Dense, no values: Meaningless; no suitable model
            //
            // Dense, > 0 values: EditableDenseThreeDimensionalModel
            //
            // These should just be our fallback positions; we want to
            // be reading semantic data from the RDF in order to pick
            // the right model directly

            enum { SODM, STVM, EDTDM } modelType = SODM;

            if (sparse) {
                if (maxValueCount == 0) {
                    modelType = SODM;
                } else if (minValueCount == 1 && maxValueCount == 1) {
                    modelType = STVM;
                } else {
                    cerr << "WARNING: No suitable model available for sparse data with between " << minValueCount << " and " << maxValueCount << " values" << endl;
                    modelType = STVM;
                }
            } else {
                if (maxValueCount == 0) {
                    cerr << "WARNING: Dense data set with no values is not meaningful, skipping" << endl;
                    continue;
                } else {
                    modelType = EDTDM;
                }
            }

            //!!! set model name &c

            if (modelType == SODM) {

                SparseOneDimensionalModel *model = 
                    new SparseOneDimensionalModel(m_sampleRate, 1, false);
                
                fillModel(model, ttvi->second);
                models.push_back(model);

            } else if (modelType == STVM) {

                SparseTimeValueModel *model = 
                    new SparseTimeValueModel(m_sampleRate, 1, false);
                
                fillModel(model, ttvi->second);
                models.push_back(model);

            } else {
                
                EditableDenseThreeDimensionalModel *model =
                    new EditableDenseThreeDimensionalModel(m_sampleRate, 1, 0,
                                                           false);

                fillModel(model, ttvi->second);
                models.push_back(model);
            }
        }
    }
*/
}

/*
void
RDFImporterImpl::extractStructure(const TimeDurationValueMap &tvm,
                                  bool &sparse,
                                  int &minValueCount,
                                  int &maxValueCount)
{
    // These are floats intentionally rather than RealTime --
    // see logic for handling rounding error below
    float firstTime = 0.f;
    float timeStep = 0.f;
    bool haveTimeStep = false;
    
    for (TimeDurationValueMap::const_iterator tvi = tvm.begin(); tvi != tvm.end(); ++tvi) {
        
        RealTime time = tvi->first;
        int valueCount = tvi->second.size();
        
        if (tvi == tvm.begin()) {
            
            minValueCount = valueCount;
            maxValueCount = valueCount;
            
            firstTime = time.toDouble();
            
        } else {
            
            if (valueCount < minValueCount) minValueCount = valueCount;
            if (valueCount > maxValueCount) maxValueCount = valueCount;
            
            if (!haveTimeStep) {
                timeStep = time.toDouble() - firstTime;
                if (timeStep == 0.f) sparse = true;
                haveTimeStep = true;
            } else if (!sparse) {
                // test whether this time is within
                // rounding-error range of being an integer
                // multiple of some constant away from the
                // first time
                float timeAsFloat = time.toDouble();
                int count = int((timeAsFloat - firstTime) / timeStep + 0.5);
                float expected = firstTime + (timeStep * count);
                if (fabsf(expected - timeAsFloat) > 1e-6) {
                    cerr << "Event at " << timeAsFloat << " is not evenly spaced -- would expect it to be " << expected << " for a spacing of " << count << " * " << timeStep << endl;
                    sparse = true;
                }
            }
        }
    }
}
*/

void
RDFImporterImpl::fillModel(Model *model,
                           long ftime,
                           long fduration,
                           bool haveDuration,
                           std::vector<float> &values,
                           QString label)
{
    SparseOneDimensionalModel *sodm =
        dynamic_cast<SparseOneDimensionalModel *>(model);
    if (sodm) {
        SparseOneDimensionalModel::Point point(ftime, label);
        sodm->addPoint(point);
        return;
    }

    SparseTimeValueModel *stvm =
        dynamic_cast<SparseTimeValueModel *>(model);
    if (stvm) {
        SparseTimeValueModel::Point point
            (ftime, values.empty() ? 0.f : values[0], label);
        stvm->addPoint(point);
        return;
    }

    NoteModel *nm =
        dynamic_cast<NoteModel *>(model);
    if (nm) {
        if (haveDuration) {
            float value = 0.f, level = 1.f;
            if (!values.empty()) {
                value = values[0];
                if (values.size() > 1) {
                    level = values[1];
                }
            }
            NoteModel::Point point(ftime, value, fduration, level, label);
            nm->addPoint(point);
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
            NoteModel::Point point(ftime, value, duration, level, label);
            nm->addPoint(point);
        }
        return;
    }

    RegionModel *rm = 
        dynamic_cast<RegionModel *>(model);
    if (rm) {
        if (haveDuration) {
            RegionModel::Point point
                (ftime, values.empty() ? 0.f : values[0], fduration, label);
            rm->addPoint(point);
        } else {
            // This won't actually happen -- we only create region models
            // if we do have duration -- but just for completeness
            float value = 0.f, duration = 1.f;
            if (!values.empty()) {
                value = values[0];
                if (values.size() > 1) {
                    duration = values[1];
                }
            }
            RegionModel::Point point(ftime, value, duration, label);
            rm->addPoint(point);
        }
        return;
    }
            
    std::cerr << "WARNING: RDFImporterImpl::fillModel: Unknown or unexpected model type" << std::endl;
    return;
}


/*
void
RDFImporterImpl::fillModel(SparseOneDimensionalModel *model,
                           const TimeDurationValueMap &tvm)
{
    //!!! labels &c not yet handled

    for (TimeDurationValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        RealTime time = tvi->first;
        long frame = RealTime::realTime2Frame(time, m_sampleRate);

        SparseOneDimensionalModel::Point point(frame);

        model->addPoint(point);
    }
}

void
RDFImporterImpl::fillModel(SparseTimeValueModel *model,
                           const TimeDurationValueMap &tvm)
{
    //!!! labels &c not yet handled

    for (TimeDurationValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        RealTime time = tvi->first;
        long frame = RealTime::realTime2Frame(time, m_sampleRate);

        float value = 0.f;
        if (!tvi->second.empty()) value = *tvi->second.begin()->second;
        
        SparseTimeValueModel::Point point(frame, value, "");

        model->addPoint(point);
    }
}

void
RDFImporterImpl::fillModel(EditableDenseThreeDimensionalModel *model,
                           const TimeDurationValueMap &tvm)
{
    //!!! labels &c not yet handled

    //!!! start time offset not yet handled

    size_t col = 0;

    for (TimeDurationValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        model->setColumn(col++, tvi->second.second);
    }
}

*/
