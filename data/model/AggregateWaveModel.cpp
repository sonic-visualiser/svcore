/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AggregateWaveModel.h"

#include <iostream>

#include <QTextStream>

using namespace std;

PowerOfSqrtTwoZoomConstraint
AggregateWaveModel::m_zoomConstraint;

AggregateWaveModel::AggregateWaveModel(ChannelSpecList channelSpecs) :
    m_components(channelSpecs),
    m_invalidated(false)
{
    for (ChannelSpecList::const_iterator i = channelSpecs.begin();
         i != channelSpecs.end(); ++i) {

        connect(i->model, SIGNAL(aboutToBeDeleted()),
                this, SLOT(componentModelAboutToBeDeleted()));
        
        if (i->model->getSampleRate() !=
            channelSpecs.begin()->model->getSampleRate()) {
            SVDEBUG << "AggregateWaveModel::AggregateWaveModel: WARNING: Component models do not all have the same sample rate" << endl;
            break;
        }
    }
}

AggregateWaveModel::~AggregateWaveModel()
{
}

void
AggregateWaveModel::componentModelAboutToBeDeleted()
{
    SVDEBUG << "AggregateWaveModel::componentModelAboutToBeDeleted: invalidating"
            << endl;
    m_components.clear();
    m_invalidated = true;
    emit modelInvalidated();
}

bool
AggregateWaveModel::isOK() const
{
    if (m_invalidated) {
        return false;
    }
    for (ChannelSpecList::const_iterator i = m_components.begin();
         i != m_components.end(); ++i) {
        if (!i->model->isOK()) {
            return false;
        }
    }
    return true;
}

bool
AggregateWaveModel::isReady(int *completion) const
{
    if (completion) *completion = 100;

    bool ready = true;
    for (ChannelSpecList::const_iterator i = m_components.begin();
         i != m_components.end(); ++i) {
        int completionHere = 100;
        if (!i->model->isReady(&completionHere)) ready = false;
        if (completion && completionHere < *completion) {
            *completion = completionHere;
        }
    }
    return ready;
}

sv_frame_t
AggregateWaveModel::getFrameCount() const
{
    sv_frame_t count = 0;
    for (ChannelSpecList::const_iterator i = m_components.begin();
         i != m_components.end(); ++i) {
        sv_frame_t thisCount =
            i->model->getEndFrame() - i->model->getStartFrame();
        if (thisCount > count) count = thisCount;
    }
    return count;
}

int
AggregateWaveModel::getChannelCount() const
{
    return int(m_components.size());
}

sv_samplerate_t
AggregateWaveModel::getSampleRate() const
{
    if (m_components.empty()) return 0;
    return m_components.begin()->model->getSampleRate();
}

floatvec_t
AggregateWaveModel::getData(int channel, sv_frame_t start, sv_frame_t count) const
{
    int ch0 = channel, ch1 = channel;
    if (channel == -1) {
        ch0 = 0;
        ch1 = getChannelCount()-1;
    }

    floatvec_t result(count, 0.f);
    sv_frame_t longest = 0;
    
    for (int c = ch0; c <= ch1; ++c) {

        auto here = m_components[c].model->getData(m_components[c].channel,
                                                   start, count);
        if (sv_frame_t(here.size()) > longest) {
            longest = sv_frame_t(here.size());
        }
        for (sv_frame_t i = 0; in_range_for(here, i); ++i) {
            result[i] += here[i];
        }
    }

    result.resize(longest);
    return result;
}

vector<floatvec_t>
AggregateWaveModel::getMultiChannelData(int fromchannel, int tochannel,
                                        sv_frame_t start, sv_frame_t count) const
{
    sv_frame_t min = count;

    vector<floatvec_t> result;

    for (int c = fromchannel; c <= tochannel; ++c) {
        auto here = getData(c, start, count);
        if (sv_frame_t(here.size()) < min) {
            min = sv_frame_t(here.size());
        }
        result.push_back(here);
    }

    if (min < count) {
        for (auto &v : result) v.resize(min);
    }
    
    return result;
}

int
AggregateWaveModel::getSummaryBlockSize(int desired) const
{
    //!!! complete
    return desired;
}
        
void
AggregateWaveModel::getSummaries(int, sv_frame_t, sv_frame_t,
                                 RangeBlock &, int &) const
{
    //!!! complete
}

AggregateWaveModel::Range
AggregateWaveModel::getSummary(int, sv_frame_t, sv_frame_t) const
{
    //!!! complete
    return Range();
}
        
int
AggregateWaveModel::getComponentCount() const
{
    return int(m_components.size());
}

AggregateWaveModel::ModelChannelSpec
AggregateWaveModel::getComponent(int c) const
{
    return m_components[c];
}

void
AggregateWaveModel::componentModelChanged()
{
    emit modelChanged();
}

void
AggregateWaveModel::componentModelChangedWithin(sv_frame_t start, sv_frame_t end)
{
    emit modelChangedWithin(start, end);
}

void
AggregateWaveModel::componentModelCompletionChanged()
{
    emit completionChanged();
}

void
AggregateWaveModel::toXml(QTextStream &out,
                          QString indent,
                          QString extraAttributes) const
{
    QStringList componentStrings;
    for (const auto &c: m_components) {
        componentStrings.push_back(QString("%1").arg(getObjectExportId(c.model)));
    }
    Model::toXml(out, indent,
                 QString("type=\"aggregatewave\" components=\"%1\" %2")
                 .arg(componentStrings.join(","))
                 .arg(extraAttributes));
}

