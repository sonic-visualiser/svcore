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

#include "CSVFormat.h"

#include <QFile>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>

#include <iostream>

CSVFormat::CSVFormat(QString filename) :
    m_modelType(TwoDimensionalModel),
    m_timingType(ExplicitTiming),
    m_timeUnits(TimeSeconds),
    m_separator(","),
    m_sampleRate(44100),
    m_windowSize(1024),
    m_behaviour(QString::KeepEmptyParts),
    m_maxExampleCols(0)
{
    QFile file(filename);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    in.seek(0);

    unsigned int lineno = 0;

    bool nonIncreasingPrimaries = false;
    bool nonNumericPrimaries = false;
    bool floatPrimaries = false;
    bool variableItemCount = false;
    int itemCount = 1;
    int earliestNonNumericItem = -1;

    float prevPrimary = 0.0;

    m_maxExampleCols = 0;
    m_separator = "";

    while (!in.atEnd()) {

        // See comment about line endings in CSVFileReader::load() 

        QString chunk = in.readLine();
        QStringList lines = chunk.split('\r', QString::SkipEmptyParts);

        for (size_t li = 0; li < lines.size(); ++li) {

            QString line = lines[li];

            if (line.startsWith("#")) continue;

            m_behaviour = QString::KeepEmptyParts;

            if (m_separator == "") {
                //!!! to do: ask the user
                if (line.split(",").size() >= 2) m_separator = ",";
                else if (line.split("\t").size() >= 2) m_separator = "\t";
                else if (line.split("|").size() >= 2) m_separator = "|";
                else if (line.split("/").size() >= 2) m_separator = "/";
                else if (line.split(":").size() >= 2) m_separator = ":";
                else {
                    m_separator = " ";
                    m_behaviour = QString::SkipEmptyParts;
                }
            }

//            std::cerr << "separator = \"" << m_separator.toStdString() << "\"" << std::endl;

            QStringList list = line.split(m_separator, m_behaviour);
            QStringList tidyList;

            for (int i = 0; i < list.size(); ++i) {
	    
                QString s(list[i]);
                bool numeric = false;

                if (s.length() >= 2 && s.startsWith("\"") && s.endsWith("\"")) {
                    s = s.mid(1, s.length() - 2);
                } else if (s.length() >= 2 && s.startsWith("'") && s.endsWith("'")) {
                    s = s.mid(1, s.length() - 2);
                } else {
                    float f = s.toFloat(&numeric);
//                    std::cerr << "converted \"" << s.toStdString() << "\" to float, got " << f << " and success = " << numeric << std::endl;
                }

                tidyList.push_back(s);

                if (lineno == 0 || (list.size() < itemCount)) {
                    itemCount = list.size();
                } else {
                    if (itemCount != list.size()) {
                        variableItemCount = true;
                    }
                }
	    
                if (i == 0) { // primary

                    if (numeric) {

                        float primary = s.toFloat();

                        if (lineno > 0 && primary <= prevPrimary) {
                            nonIncreasingPrimaries = true;
                        }

                        if (s.contains(".") || s.contains(",")) {
                            floatPrimaries = true;
                        }

                        prevPrimary = primary;

                    } else {
                        nonNumericPrimaries = true;
                    }
                } else { // secondary

                    if (!numeric) {
                        if (earliestNonNumericItem < 0 ||
                            i < earliestNonNumericItem) {
                            earliestNonNumericItem = i;
                        }
                    }
                }
            }

            if (lineno < 10) {
                m_example.push_back(tidyList);
                if (lineno == 0 || tidyList.size() > m_maxExampleCols) {
                    m_maxExampleCols = tidyList.size();
                }
            }

            ++lineno;

            if (lineno == 50) break;
        }
    }

    if (nonNumericPrimaries || nonIncreasingPrimaries) {
	
	// Primaries are probably not a series of times

	m_timingType = CSVFormat::ImplicitTiming;
	m_timeUnits = CSVFormat::TimeWindows;
	
	if (nonNumericPrimaries) {
	    m_modelType = CSVFormat::OneDimensionalModel;
	} else if (itemCount == 1 || variableItemCount ||
		   (earliestNonNumericItem != -1)) {
	    m_modelType = CSVFormat::TwoDimensionalModel;
	} else {
	    m_modelType = CSVFormat::ThreeDimensionalModel;
	}

    } else {

	// Increasing numeric primaries -- likely to be time

	m_timingType = CSVFormat::ExplicitTiming;

	if (floatPrimaries) {
	    m_timeUnits = CSVFormat::TimeSeconds;
	} else {
	    m_timeUnits = CSVFormat::TimeAudioFrames;
	}

	if (itemCount == 1) {
	    m_modelType = CSVFormat::OneDimensionalModel;
	} else if (variableItemCount || (earliestNonNumericItem != -1)) {
	    if (earliestNonNumericItem != -1 && earliestNonNumericItem < 2) {
		m_modelType = CSVFormat::OneDimensionalModel;
	    } else {
		m_modelType = CSVFormat::TwoDimensionalModel;
	    }
	} else {
	    m_modelType = CSVFormat::ThreeDimensionalModel;
	}
    }

    std::cerr << "Estimated model type: " << m_modelType << std::endl;
    std::cerr << "Estimated timing type: " << m_timingType << std::endl;
    std::cerr << "Estimated units: " << m_timeUnits << std::endl;
}

