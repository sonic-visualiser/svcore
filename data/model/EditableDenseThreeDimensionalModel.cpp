/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "EditableDenseThreeDimensionalModel.h"

#include "base/LogRange.h"

#include <QTextStream>
#include <QStringList>

#include <iostream>

#include <cmath>
#include <cassert>

EditableDenseThreeDimensionalModel::EditableDenseThreeDimensionalModel(size_t sampleRate,
                                                                       size_t resolution,
                                                                       size_t yBinCount,
                                                                       bool notifyOnAdd) :
    m_sampleRate(sampleRate),
    m_resolution(resolution),
    m_yBinCount(yBinCount),
    m_minimum(0.0),
    m_maximum(0.0),
    m_haveExtents(false),
    m_notifyOnAdd(notifyOnAdd),
    m_sinceLastNotifyMin(-1),
    m_sinceLastNotifyMax(-1),
    m_completion(100)
{
}    

bool
EditableDenseThreeDimensionalModel::isOK() const
{
    return true;
}

size_t
EditableDenseThreeDimensionalModel::getSampleRate() const
{
    return m_sampleRate;
}

size_t
EditableDenseThreeDimensionalModel::getStartFrame() const
{
    return 0;
}

size_t
EditableDenseThreeDimensionalModel::getEndFrame() const
{
    return m_resolution * m_data.size() + (m_resolution - 1);
}

Model *
EditableDenseThreeDimensionalModel::clone() const
{
    QMutexLocker locker(&m_mutex);

    EditableDenseThreeDimensionalModel *model =
        new EditableDenseThreeDimensionalModel
	(m_sampleRate, m_resolution, m_yBinCount);

    model->m_minimum = m_minimum;
    model->m_maximum = m_maximum;
    model->m_haveExtents = m_haveExtents;

    for (size_t i = 0; i < m_data.size(); ++i) {
	model->setColumn(i, m_data.at(i));
    }

    return model;
}

size_t
EditableDenseThreeDimensionalModel::getResolution() const
{
    return m_resolution;
}

void
EditableDenseThreeDimensionalModel::setResolution(size_t sz)
{
    m_resolution = sz;
}

size_t
EditableDenseThreeDimensionalModel::getWidth() const
{
    return m_data.size();
}

size_t
EditableDenseThreeDimensionalModel::getHeight() const
{
    return m_yBinCount;
}

void
EditableDenseThreeDimensionalModel::setHeight(size_t sz)
{
    m_yBinCount = sz;
}

float
EditableDenseThreeDimensionalModel::getMinimumLevel() const
{
    return m_minimum;
}

void
EditableDenseThreeDimensionalModel::setMinimumLevel(float level)
{
    m_minimum = level;
}

float
EditableDenseThreeDimensionalModel::getMaximumLevel() const
{
    return m_maximum;
}

void
EditableDenseThreeDimensionalModel::setMaximumLevel(float level)
{
    m_maximum = level;
}

EditableDenseThreeDimensionalModel::Column
EditableDenseThreeDimensionalModel::getColumn(size_t index) const
{
    QMutexLocker locker(&m_mutex);
    if (index >= m_data.size()) return Column();
    return expandAndRetrieve(index);
}

float
EditableDenseThreeDimensionalModel::getValueAt(size_t index, size_t n) const
{
    Column c = getColumn(index);
    if (n < c.size()) return s.at(n);
    return m_minimum;
}

static int given = 0, stored = 0;

void
EditableDenseThreeDimensionalModel::truncateAndStore(size_t index,
                                                     const Column &values)
{
    assert(index < m_data.size());

    //std::cout << "truncateAndStore(" << index << ", " << values.size() << ")" << std::endl;

    m_trunc[index] = 0;
    if (index == 0 || values.size() != m_yBinCount) {
        given += values.size();
        stored += values.size();
        m_data[index] = values;
        return;
    }

    static int maxdist = 120;

    bool known = false;
    bool top = false;

    int tdist = 1;
    int ptrunc = m_trunc[index-1];
    if (ptrunc < 0) {
        top = false;
        known = true;
        tdist = -ptrunc + 1;
    } else if (ptrunc > 0) {
        top = true;
        known = true;
        tdist = ptrunc + 1;
    }

    Column p = expandAndRetrieve(index - tdist);
    int h = m_yBinCount;

    if (p.size() == h && tdist <= maxdist) {

        int bcount = 0, tcount = 0;
        if (!known || !top) {
            for (int i = 0; i < h; ++i) {
                if (values.at(i) == p.at(i)) ++bcount;
                else break;
            }
        }
        if (!known || top) {
            for (int i = h; i > 0; --i) {
                if (values.at(i-1) == p.at(i-1)) ++tcount;
                else break;
            }
        }
        if (!known) top = (tcount > bcount);

        int limit = h / 4;
        if ((top ? tcount : bcount) > limit) {
        
            if (!top) {
                Column tcol(h - bcount);
                given += values.size();
                stored += h - bcount;
                for (int i = bcount; i < h; ++i) {
                    tcol[i - bcount] = values.at(i);
                }
                m_data[index] = tcol;
                m_trunc[index] = -tdist;
                //std::cout << "bottom " << bcount << " as col at " << -tdist << std::endl;
                return;
            } else {
                Column tcol(h - tcount);
                given += values.size();
                stored += h - tcount;
                for (int i = 0; i < h - tcount; ++i) {
                    tcol[i] = values.at(i);
                }
                m_data[index] = tcol;
                m_trunc[index] = tdist;
                //std::cout << "top " << tcount << " as col at " << -tdist << std::endl;
                return;
            }
        }
    }                

    given += values.size();
    stored += values.size();

//    std::cout << "given: " << given << ", stored: " << stored << " (" 
//              << ((float(stored) / float(given)) * 100.f) << "%)" << std::endl;

    m_data[index] = values;
    return;
}

EditableDenseThreeDimensionalModel::Column
EditableDenseThreeDimensionalModel::expandAndRetrieve(size_t index) const
{
    assert(index < m_data.size());
    Column c = m_data.at(index);
    if (index == 0) {
        return c;
    }
    int trunc = (int)m_trunc[index];
    if (trunc == 0) {
        return c;
    }
    bool top = true;
    int tdist = trunc;
    if (trunc < 0) { top = false; tdist = -trunc; }
    Column p = expandAndRetrieve(index - tdist);
    if (p.size() != m_yBinCount) {
        std::cerr << "WARNING: EditableDenseThreeDimensionalModel::expandAndRetrieve: Trying to expand from incorrectly sized column" << std::endl;
    }
    if (top) {
        for (int i = c.size(); i < p.size(); ++i) {
            c.push_back(p.at(i));
        }
    } else {
        for (int i = int(p.size()) - int(c.size()); i >= 0; --i) {
            c.push_front(p.at(i));
        }
    }
    return c;
}

void
EditableDenseThreeDimensionalModel::setColumn(size_t index,
                                              const Column &values)
{
    QMutexLocker locker(&m_mutex);

    while (index >= m_data.size()) {
	m_data.push_back(Column());
        m_trunc.push_back(0);
    }

    bool allChange = false;

//    if (values.size() > m_yBinCount) m_yBinCount = values.size();

    for (size_t i = 0; i < values.size(); ++i) {
        float value = values[i];
        if (std::isnan(value) || std::isinf(value)) {
            continue;
        }
	if (!m_haveExtents || value < m_minimum) {
	    m_minimum = value;
	    allChange = true;
	}
	if (!m_haveExtents || value > m_maximum) {
	    m_maximum = value;
	    allChange = true;
	}
        m_haveExtents = true;
    }

    truncateAndStore(index, values);

    assert(values == expandAndRetrieve(index));

    long windowStart = index;
    windowStart *= m_resolution;

    if (m_notifyOnAdd) {
	if (allChange) {
	    emit modelChanged();
	} else {
	    emit modelChanged(windowStart, windowStart + m_resolution);
	}
    } else {
	if (allChange) {
	    m_sinceLastNotifyMin = -1;
	    m_sinceLastNotifyMax = -1;
	    emit modelChanged();
	} else {
	    if (m_sinceLastNotifyMin == -1 ||
		windowStart < m_sinceLastNotifyMin) {
		m_sinceLastNotifyMin = windowStart;
	    }
	    if (m_sinceLastNotifyMax == -1 ||
		windowStart > m_sinceLastNotifyMax) {
		m_sinceLastNotifyMax = windowStart;
	    }
	}
    }
}

QString
EditableDenseThreeDimensionalModel::getBinName(size_t n) const
{
    if (m_binNames.size() > n) return m_binNames[n];
    else return "";
}

void
EditableDenseThreeDimensionalModel::setBinName(size_t n, QString name)
{
    while (m_binNames.size() <= n) m_binNames.push_back("");
    m_binNames[n] = name;
    emit modelChanged();
}

void
EditableDenseThreeDimensionalModel::setBinNames(std::vector<QString> names)
{
    m_binNames = names;
    emit modelChanged();
}

bool
EditableDenseThreeDimensionalModel::shouldUseLogValueScale() const
{
    QMutexLocker locker(&m_mutex);

    QVector<float> sample;
    QVector<int> n;
    
    for (int i = 0; i < 10; ++i) {
        size_t index = i * 10;
        if (index < m_data.size()) {
            const Column &c = m_data.at(index);
            while (c.size() > sample.size()) {
                sample.push_back(0.f);
                n.push_back(0);
            }
            for (int j = 0; j < c.size(); ++j) {
                sample[j] += c.at(j);
                ++n[j];
            }
        }
    }

    if (sample.empty()) return false;
    for (int j = 0; j < sample.size(); ++j) {
        if (n[j]) sample[j] /= n[j];
    }
    
    return LogRange::useLogScale(sample.toStdVector());
}

void
EditableDenseThreeDimensionalModel::setCompletion(int completion, bool update)
{
    if (m_completion != completion) {
	m_completion = completion;

	if (completion == 100) {

	    m_notifyOnAdd = true; // henceforth
	    emit modelChanged();

	} else if (!m_notifyOnAdd) {

	    if (update &&
                m_sinceLastNotifyMin >= 0 &&
		m_sinceLastNotifyMax >= 0) {
		emit modelChanged(m_sinceLastNotifyMin,
				  m_sinceLastNotifyMax + m_resolution);
		m_sinceLastNotifyMin = m_sinceLastNotifyMax = -1;
	    } else {
		emit completionChanged();
	    }
	} else {
	    emit completionChanged();
	}	    
    }
}

QString
EditableDenseThreeDimensionalModel::toDelimitedDataString(QString delimiter) const
{
    QMutexLocker locker(&m_mutex);
    QString s;
    for (size_t i = 0; i < m_data.size(); ++i) {
        QStringList list;
	for (size_t j = 0; j < m_data.at(i).size(); ++j) {
            list << QString("%1").arg(m_data.at(i).at(j));
        }
        s += list.join(delimiter) + "\n";
    }
    return s;
}

void
EditableDenseThreeDimensionalModel::toXml(QTextStream &out,
                                          QString indent,
                                          QString extraAttributes) const
{
    QMutexLocker locker(&m_mutex);

    // For historical reasons we read and write "resolution" as "windowSize"

    std::cerr << "EditableDenseThreeDimensionalModel::toXml" << std::endl;

    Model::toXml
	(out, indent,
         QString("type=\"dense\" dimensions=\"3\" windowSize=\"%1\" yBinCount=\"%2\" minimum=\"%3\" maximum=\"%4\" dataset=\"%5\" %6")
	 .arg(m_resolution)
	 .arg(m_yBinCount)
	 .arg(m_minimum)
	 .arg(m_maximum)
	 .arg(getObjectExportId(&m_data))
	 .arg(extraAttributes));

    out << indent;
    out << QString("<dataset id=\"%1\" dimensions=\"3\" separator=\" \">\n")
	.arg(getObjectExportId(&m_data));

    for (size_t i = 0; i < m_binNames.size(); ++i) {
	if (m_binNames[i] != "") {
	    out << indent + "  ";
	    out << QString("<bin number=\"%1\" name=\"%2\"/>\n")
		.arg(i).arg(m_binNames[i]);
	}
    }

    for (size_t i = 0; i < m_data.size(); ++i) {
	out << indent + "  ";
	out << QString("<row n=\"%1\">").arg(i);
	for (size_t j = 0; j < m_data.at(i).size(); ++j) {
	    if (j > 0) out << " ";
	    out << m_data.at(i).at(j);
	}
	out << QString("</row>\n");
        out.flush();
    }

    out << indent + "</dataset>\n";
}


