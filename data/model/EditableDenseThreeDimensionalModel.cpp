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

#include <QTextStream>

#include <iostream>

#include <cmath>

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
    EditableDenseThreeDimensionalModel *model =
        new EditableDenseThreeDimensionalModel
	(m_sampleRate, m_resolution, m_yBinCount);

    model->m_minimum = m_minimum;
    model->m_maximum = m_maximum;
    model->m_haveExtents = m_haveExtents;

    for (size_t i = 0; i < m_data.size(); ++i) {
	model->setColumn(i, m_data[i]);
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

void
EditableDenseThreeDimensionalModel::getColumn(size_t index,
                                              Column &result) const
{
    QMutexLocker locker(&m_mutex);

    if (index < m_data.size()) {
	result = m_data[index];
    } else {
	result.clear();
    }

    while (result.size() < m_yBinCount) result.push_back(m_minimum);
}

float
EditableDenseThreeDimensionalModel::getValueAt(size_t index, size_t n) const
{
    QMutexLocker locker(&m_mutex);

    if (index < m_data.size()) {
	const Column &s = m_data[index];
//        std::cerr << "index " << index << ", n " << n << ", res " << m_resolution << ", size " << s.size()
//                  << std::endl;
	if (n < s.size()) return s[n];
    }

    return m_minimum;
}

void
EditableDenseThreeDimensionalModel::setColumn(size_t index,
                                              const Column &values)
{
    QMutexLocker locker(&m_mutex);

    while (index >= m_data.size()) {
	m_data.push_back(Column());
    }

    bool allChange = false;

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

    m_data[index] = values;

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
    QString s;
    for (size_t i = 0; i < m_data.size(); ++i) {
        QStringList list;
	for (size_t j = 0; j < m_data[i].size(); ++j) {
            list << QString("%1").arg(m_data[i][j]);
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
	for (size_t j = 0; j < m_data[i].size(); ++j) {
	    if (j > 0) out << " ";
	    out << m_data[i][j];
	}
	out << QString("</row>\n");
        out.flush();
    }

    out << indent + "</dataset>\n";
}


