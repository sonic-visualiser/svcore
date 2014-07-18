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

PowerOfSqrtTwoZoomConstraint
AggregateWaveModel::m_zoomConstraint;

AggregateWaveModel::AggregateWaveModel(ChannelSpecList channelSpecs) :
    m_components(channelSpecs)
{
    for (ChannelSpecList::const_iterator i = channelSpecs.begin();
         i != channelSpecs.end(); ++i) {
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

bool
AggregateWaveModel::isOK() const
{
    for (ChannelSpecList::const_iterator i = m_components.begin();
         i != m_components.end(); ++i) {
        if (!i->model->isOK()) return false;
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

int
AggregateWaveModel::getFrameCount() const
{
    int count = 0;

    for (ChannelSpecList::const_iterator i = m_components.begin();
         i != m_components.end(); ++i) {
        int thisCount = i->model->getEndFrame() - i->model->getStartFrame();
        if (thisCount > count) count = thisCount;
    }

    return count;
}

int
AggregateWaveModel::getChannelCount() const
{
    return m_components.size();
}

int
AggregateWaveModel::getSampleRate() const
{
    if (m_components.empty()) return 0;
    return m_components.begin()->model->getSampleRate();
}

Model *
AggregateWaveModel::clone() const
{
    return new AggregateWaveModel(m_components);
}

int
AggregateWaveModel::getData(int channel, int start, int count,
                            float *buffer) const
{
    int ch0 = channel, ch1 = channel;
    bool mixing = false;
    if (channel == -1) {
        ch0 = 0;
        ch1 = getChannelCount()-1;
        mixing = true;
    }

    float *readbuf = buffer;
    if (mixing) {
        readbuf = new float[count];
        for (int i = 0; i < count; ++i) {
            buffer[i] = 0.f;
        }
    }

    int sz = count;

    for (int c = ch0; c <= ch1; ++c) {
        int szHere = 
            m_components[c].model->getData(m_components[c].channel,
                                           start, count,
                                           readbuf);
        if (szHere < sz) sz = szHere;
        if (mixing) {
            for (int i = 0; i < count; ++i) {
                buffer[i] += readbuf[i];
            }
        }
    }

    if (mixing) delete[] readbuf;
    return sz;
}
         
int
AggregateWaveModel::getData(int channel, int start, int count,
                            double *buffer) const
{
    int ch0 = channel, ch1 = channel;
    bool mixing = false;
    if (channel == -1) {
        ch0 = 0;
        ch1 = getChannelCount()-1;
        mixing = true;
    }

    double *readbuf = buffer;
    if (mixing) {
        readbuf = new double[count];
        for (int i = 0; i < count; ++i) {
            buffer[i] = 0.0;
        }
    }

    int sz = count;
    
    for (int c = ch0; c <= ch1; ++c) {
        int szHere = 
            m_components[c].model->getData(m_components[c].channel,
                                           start, count,
                                           readbuf);
        if (szHere < sz) sz = szHere;
        if (mixing) {
            for (int i = 0; i < count; ++i) {
                buffer[i] += readbuf[i];
            }
        }
    }
    
    if (mixing) delete[] readbuf;
    return sz;
}

int
AggregateWaveModel::getData(int fromchannel, int tochannel,
                            int start, int count,
                            float **buffer) const
{
    int min = count;

    for (int c = fromchannel; c <= tochannel; ++c) {
        int here = getData(c, start, count, buffer[c - fromchannel]);
        if (here < min) min = here;
    }
    
    return min;
}

int
AggregateWaveModel::getSummaryBlockSize(int desired) const
{
    //!!! complete
    return desired;
}
        
void
AggregateWaveModel::getSummaries(int, int, int,
                                 RangeBlock &, int &) const
{
    //!!! complete
}

AggregateWaveModel::Range
AggregateWaveModel::getSummary(int, int, int) const
{
    //!!! complete
    return Range();
}
        
int
AggregateWaveModel::getComponentCount() const
{
    return m_components.size();
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
AggregateWaveModel::componentModelChangedWithin(int start, int end)
{
    emit modelChangedWithin(start, end);
}

void
AggregateWaveModel::componentModelCompletionChanged()
{
    emit completionChanged();
}

void
AggregateWaveModel::toXml(QTextStream &,
                          QString ,
                          QString ) const
{
    //!!! complete
}

