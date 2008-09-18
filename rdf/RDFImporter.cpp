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

    typedef std::vector<float> ValueList;
    typedef std::map<RealTime, ValueList> TimeValueMap;
    typedef std::map<QString, TimeValueMap> TypeTimeValueMap;
    typedef std::map<QString, TypeTimeValueMap> SourceTypeTimeValueMap;

    void extractStructure(const TimeValueMap &map, bool &sparse,
                          int &minValueCount, int &maxValueCount);

    void fillModel(SparseOneDimensionalModel *, const TimeValueMap &);
    void fillModel(SparseTimeValueModel *, const TimeValueMap &);
    void fillModel(EditableDenseThreeDimensionalModel *, const TimeValueMap &);
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

    SourceTypeTimeValueMap m;

    QString queryString = QString(

        " PREFIX event: <http://purl.org/NET/c4dm/event.owl#>"
        " PREFIX time: <http://purl.org/NET/c4dm/timeline.owl#>"
        " PREFIX mo: <http://purl.org/ontology/mo/>"
        " PREFIX af: <http://purl.org/ontology/af/>"

        " SELECT ?signalSource ?time ?eventType ?value"
        " FROM <%1>"

        " WHERE {"
        "   ?signal mo:available_as ?signalSource ."
        "   ?signal mo:time ?interval ."
        "   ?interval time:onTimeLine ?tl ."
        "   ?t time:onTimeLine ?tl ."
        "   ?t time:at ?time ."
        "   ?timedThing event:time ?t ."
        "   ?timedThing a ?eventType ."
        "   OPTIONAL {"
        "     ?timedThing af:hasFeature ?feature ."
        "     ?feature af:value ?value"
        "   }"
        " }"

        ).arg(m_uristring);

    SimpleSPARQLQuery query(queryString);
    query.setProgressReporter(reporter);

    cerr << "Query will be: " << queryString.toStdString() << endl;

    SimpleSPARQLQuery::ResultList results = query.execute();

    if (!query.isOK()) {
        m_errorString = query.getErrorString();
        return models;
    }

    if (query.wasCancelled()) {
        m_errorString = "Query cancelled";
        return models;
    }        

    for (int i = 0; i < results.size(); ++i) {

        QString source = results[i]["signalSource"].value;

        QString timestring = results[i]["time"].value;
        RealTime time;
        time = RealTime::fromXsdDuration(timestring.toStdString());
        cerr << "time = " << time.toString() << " (from xsd:duration \""
             << timestring.toStdString() << "\")" << endl;

        QString type = results[i]["eventType"].value;

        QString valuestring = results[i]["value"].value;
        float value = 0.f;
        bool haveValue = false;
        if (valuestring != "") {
            value = valuestring.toFloat(&haveValue);
            cerr << "value = " << value << endl;
        }

        if (haveValue) {
            m[source][type][time].push_back(value);
        } else if (m[source][type].find(time) == m[source][type].end()) {
            m[source][type][time] = ValueList();
        }
    }

    for (SourceTypeTimeValueMap::const_iterator mi = m.begin();
         mi != m.end(); ++mi) {
        
        QString source = mi->first;

        for (TypeTimeValueMap::const_iterator ttvi = mi->second.begin();
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


    return models;
}

void
RDFImporterImpl::extractStructure(const TimeValueMap &tvm,
                                  bool &sparse,
                                  int &minValueCount,
                                  int &maxValueCount)
{
    // These are floats intentionally rather than RealTime --
    // see logic for handling rounding error below
    float firstTime = 0.f;
    float timeStep = 0.f;
    bool haveTimeStep = false;
    
    for (TimeValueMap::const_iterator tvi = tvm.begin(); tvi != tvm.end(); ++tvi) {
        
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

void
RDFImporterImpl::fillModel(SparseOneDimensionalModel *model,
                           const TimeValueMap &tvm)
{
    //!!! labels &c not yet handled

    for (TimeValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        RealTime time = tvi->first;
        long frame = RealTime::realTime2Frame(time, m_sampleRate);

        SparseOneDimensionalModel::Point point(frame);

        model->addPoint(point);
    }
}

void
RDFImporterImpl::fillModel(SparseTimeValueModel *model,
                           const TimeValueMap &tvm)
{
    //!!! labels &c not yet handled

    for (TimeValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        RealTime time = tvi->first;
        long frame = RealTime::realTime2Frame(time, m_sampleRate);

        float value = 0.f;
        if (!tvi->second.empty()) value = *tvi->second.begin();
        
        SparseTimeValueModel::Point point(frame, value, "");

        model->addPoint(point);
    }
}

void
RDFImporterImpl::fillModel(EditableDenseThreeDimensionalModel *model,
                           const TimeValueMap &tvm)
{
    //!!! labels &c not yet handled

    //!!! start time offset not yet handled

    size_t col = 0;

    for (TimeValueMap::const_iterator tvi = tvm.begin();
         tvi != tvm.end(); ++tvi) {
        
        model->setColumn(col++, tvi->second);
    }
}

