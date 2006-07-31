/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "EditableDenseThreeDimensionalModel.h"

#include <QTextStream>

EditableDenseThreeDimensionalModel::EditableDenseThreeDimensionalModel(size_t sampleRate,
                                                                       size_t resolution,
                                                                       size_t yBinCount,
                                                                       bool notifyOnAdd) :
    m_sampleRate(sampleRate),
    m_resolution(resolution),
    m_yBinCount(yBinCount),
    m_minimum(0.0),
    m_maximum(0.0),
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

    for (size_t i = 0; i < m_data.size(); ++i) {
	model->setBinValues(i * m_resolution, m_data[i]);
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
EditableDenseThreeDimensionalModel::getYBinCount() const
{
    return m_yBinCount;
}

void
EditableDenseThreeDimensionalModel::setYBinCount(size_t sz)
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
EditableDenseThreeDimensionalModel::getBinValues(long windowStart,
					 BinValueSet &result) const
{
    QMutexLocker locker(&m_mutex);
    
    long index = windowStart / m_resolution;

    if (index >= 0 && index < long(m_data.size())) {
	result = m_data[index];
    } else {
	result.clear();
    }

    while (result.size() < m_yBinCount) result.push_back(m_minimum);
}

float
EditableDenseThreeDimensionalModel::getBinValue(long windowStart,
					size_t n) const
{
    QMutexLocker locker(&m_mutex);
    
    long index = windowStart / m_resolution;

    if (index >= 0 && index < long(m_data.size())) {
	const BinValueSet &s = m_data[index];
	if (n < s.size()) return s[n];
    }

    return m_minimum;
}

void
EditableDenseThreeDimensionalModel::setBinValues(long windowStart,
					 const BinValueSet &values)
{
    QMutexLocker locker(&m_mutex);

    long index = windowStart / m_resolution;

    while (index >= long(m_data.size())) {
	m_data.push_back(BinValueSet());
    }

    bool newExtents = (m_data.empty() && (m_minimum == m_maximum));
    bool allChange = false;

    for (size_t i = 0; i < values.size(); ++i) {
	if (newExtents || values[i] < m_minimum) {
	    m_minimum = values[i];
	    allChange = true;
	}
	if (newExtents || values[i] > m_maximum) {
	    m_maximum = values[i];
	    allChange = true;
	}
    }

    m_data[index] = values;

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
EditableDenseThreeDimensionalModel::setCompletion(int completion)
{
    if (m_completion != completion) {
	m_completion = completion;

	if (completion == 100) {

	    m_notifyOnAdd = true; // henceforth
	    emit modelChanged();

	} else if (!m_notifyOnAdd) {

	    if (m_sinceLastNotifyMin >= 0 &&
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

void
EditableDenseThreeDimensionalModel::toXml(QTextStream &out,
                                  QString indent,
                                  QString extraAttributes) const
{
    // For historical reasons we read and write "resolution" as "windowSize"

    out << Model::toXmlString
	(indent, QString("type=\"dense\" dimensions=\"3\" windowSize=\"%1\" yBinCount=\"%2\" minimum=\"%3\" maximum=\"%4\" dataset=\"%5\" %6")
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
    }

    out << indent + "</dataset>\n";
}

QString
EditableDenseThreeDimensionalModel::toXmlString(QString indent,
					QString extraAttributes) const
{
    QString s;

    {
        QTextStream out(&s);
        toXml(out, indent, extraAttributes);
    }

    return s;
}

#ifdef INCLUDE_MOCFILES
#include "EditableDenseThreeDimensionalModel.moc.cpp"
#endif

