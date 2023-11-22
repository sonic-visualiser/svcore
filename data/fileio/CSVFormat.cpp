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

#include "base/StringBits.h"
#include "base/UnitDatabase.h"

#include <QFile>
#include <QString>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <iostream>

#include "base/Debug.h"

//#define DEBUG_COLUMN_QUALITIES 1

CSVFormat::CSVFormat(QString path) :
    m_separator(""),
    m_sampleRate(44100),
    m_increment(1024),
    m_headerStatus(HeaderUnknown),
    m_allowQuoting(true),
    m_maxExampleCols(0)
{
    (void)guessFormatFor(path);
}

bool
CSVFormat::guessFormatFor(QString path)
{
    m_modelType = TwoDimensionalModel;
    m_timingType = ExplicitTiming;
    m_timeUnits = TimeSeconds;

    m_maxExampleCols = 0;
    m_columnCount = 0;
    m_variableColumnCount = false;

    m_example.clear();
    m_columnQualities.clear();
    m_columnPurposes.clear();
    m_prevValues.clear();

    QFile file(path);
    if (!file.exists()) {
        SVCERR << "CSVFormat::guessFormatFor(" << path
               << "): File does not exist" << endl;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        SVCERR << "CSVFormat::guessFormatFor(" << path
               << "): File could not be opened for reading" << endl;
        return false;
    }
    SVDEBUG << "CSVFormat::guessFormatFor(" << path << ")" << endl;

    QTextStream in(&file);
    in.seek(0);

    int lineno = 0;

    while (!in.atEnd()) {

        // See comment about line endings in CSVFileReader::load() 

        QString chunk = in.readLine();
        QStringList lines = chunk.split('\r', Qt::SkipEmptyParts);

        for (int li = 0; li < lines.size(); ++li) {

            QString line = lines[li];
            if (line.startsWith("#") || line == "") {
                continue;
            }

            guessQualities(line, lineno);

            ++lineno;
        }

        if (lineno >= 150) break;
    }

    guessPurposes();
    guessAudioSampleRange();

    return true;
}

void
CSVFormat::guessSeparator(QString line)
{
    QString candidates = "\t|,/: ";

    for (int i = 0; i < candidates.length(); ++i) {
        auto bits = StringBits::split(line, candidates[i], m_allowQuoting);
        if (bits.size() >= 2) {
            m_plausibleSeparators.insert(candidates[i]);
            if (m_separator == "") {
                m_separator = candidates[i];
                SVDEBUG << "Estimated column separator: '" << m_separator
                        << "'" << endl;
            }
        }
    }
}

void
CSVFormat::guessQualities(QString line, int lineno)
{
    guessSeparator(line);

    QStringList list = StringBits::split(line, getSeparator(), m_allowQuoting);

    int cols = list.size();

    int firstLine = 0;
    if (m_headerStatus == HeaderPresent) {
        firstLine = 1;
    }
    
    if (lineno == firstLine || (cols > m_columnCount)) {
        m_columnCount = cols;
    }
    if (cols != m_columnCount) {
        m_variableColumnCount = true;
    }

    // All columns are regarded as having these qualities until we see
    // something that indicates otherwise:

    ColumnQualities defaultQualities =
        ColumnNumeric | ColumnIntegral | ColumnSmall |
        ColumnIncreasing | ColumnNearEmpty;
    
    for (int i = 0; i < cols; ++i) {

#ifdef DEBUG_COLUMN_QUALITIES
        SVCERR << "line no " << lineno << ": column " << i << " contains: \"" << list[i] << "\"" << endl;
#endif

        if (m_columnQualities.find(i) == m_columnQualities.end()) {
            m_columnQualities[i] = defaultQualities;
            m_prevValues[i] = 0.f;
        }

        QString s(list[i]);
        bool ok = false;

        ColumnQualities qualities = m_columnQualities[i];

// Looks like this is defined on Windows
#undef small
        
        bool numeric    = (qualities & ColumnNumeric);
        bool integral   = (qualities & ColumnIntegral);
        bool increasing = (qualities & ColumnIncreasing);
        bool small      = (qualities & ColumnSmall);
        bool large      = (qualities & ColumnLarge); // this one defaults to off
        bool signd      = (qualities & ColumnSigned); // also defaults to off
        bool emptyish   = (qualities & ColumnNearEmpty);

        if (s.trimmed() != "") {
        
            if (lineno > firstLine) {
                emptyish = false;
            }
        
            float value = 0.f;

            if (numeric) {
                value = s.toFloat(&ok);
                if (!ok) {
                    value = (float)StringBits::stringToDoubleLocaleFree(s, &ok);
                }
                if (ok) {
                    if (lineno < firstLine + 2 && value > 1000.f) {
                        large = true;
                    }
                    if (value < 0.f) {
                        signd = true;
                    }
                    if (value < -1.f || value > 1.f) {
                        small = false;
                    }
                } else {
                    numeric = false;

                    // If the column is not numeric, it can't be any of
                    // these things either
                    integral = false;
                    increasing = false;
                    small = false;
                    large = false;
                    signd = false;
                }
            }

            if (numeric) {

                if (integral) {
                    if (s.contains('.') || s.contains(',')) {
                        integral = false;
                    }
                }

                if (increasing) {
                    if (lineno > firstLine && value <= m_prevValues[i]) {
                        increasing = false;
                    }
                }

                m_prevValues[i] = value;
            }
        }
        
        m_columnQualities[i] =
            (numeric    ? ColumnNumeric : 0) |
            (integral   ? ColumnIntegral : 0) |
            (increasing ? ColumnIncreasing : 0) |
            (small      ? ColumnSmall : 0) |
            (large      ? ColumnLarge : 0) |
            (signd      ? ColumnSigned : 0) |
            (emptyish   ? ColumnNearEmpty : 0);
    }

    if (lineno == 0 && m_headerStatus == HeaderUnknown) {
        // If we have at least one column, and every column has
        // quality == ColumnNearEmpty, i.e. not empty and not numeric,
        // then we probably have a header row
        bool couldBeHeader = (cols > 0);
        std::map<int, QString> headings;
        for (int i = 0; i < cols; ++i) {
            if (m_columnQualities[i] != ColumnNearEmpty) {
                couldBeHeader = false;
            } else {
                headings[i] = list[i].trimmed();
            }
        }
        if (couldBeHeader) {
            m_headerStatus = HeaderPresent;
            m_columnHeadings = headings;
        } else {
            m_headerStatus = HeaderAbsent;
        }
    }

    if (lineno == 0 && m_headerStatus == HeaderPresent) {
        // Start again with the qualities:
        m_columnQualities.clear();
        m_prevValues.clear();
    }

    if (lineno < firstLine + 10) {
        m_example.push_back(list);
        if (lineno == 0 || cols > m_maxExampleCols) {
            m_maxExampleCols = cols;
        }
    }

    if (lineno < firstLine + 10) {
        SVDEBUG << "Estimated column qualities for line " << lineno << " (reporting up to first 10): ";
        if (lineno == 0 && m_headerStatus == HeaderPresent &&
            m_columnCount > 0 && m_columnQualities.empty()) {
            SVDEBUG << "[whole line classified as a header row]";
        } else {
            for (int i = 0; i < cols; ++i) {
                if (m_columnQualities.find(i) == m_columnQualities.end()) {
                    SVDEBUG << "(not set) ";
                } else {
                    SVDEBUG << int(m_columnQualities[i]) << " ";
                }
            }
        }
        SVDEBUG << endl;
        SVDEBUG << "Estimated header status: " << m_headerStatus << endl;
    }
}

void
CSVFormat::guessPurposes()
{
    m_timingType = CSVFormat::ImplicitTiming;
    m_timeUnits = CSVFormat::TimeWindows;
        
    int timingColumnCount = 0;
    bool haveDurationOrEndTime = false;

    SVDEBUG << "Estimated column qualities overall: ";
    for (int i = 0; i < m_columnCount; ++i) {
        if (m_columnQualities.find(i) == m_columnQualities.end()) {
            SVDEBUG << "(not set) ";
        } else {
            SVDEBUG << int(m_columnQualities[i]) << " ";
        }
    }
    SVDEBUG << endl;

    // if our first column has zero or one entries in it and the rest
    // have more, then we'll default to ignoring the first column and
    // counting the next one as primary. (e.g. Sonic Annotator output
    // with filename at start of first column.)

    int primaryColumnNo = 0;

    if (m_columnCount >= 2) {
        if ( (m_columnQualities[0] & ColumnNearEmpty) &&
            !(m_columnQualities[1] & ColumnNearEmpty)) {
            primaryColumnNo = 1;
        }
    }

    m_columnPossibleUnits.clear();
    
    for (int i = 0; i < m_columnCount; ++i) {
        
        ColumnPurpose purpose = ColumnUnknown;

        if (i < primaryColumnNo) {
            setColumnPurpose(i, purpose);
            continue;
        }
        
        bool primary = (i == primaryColumnNo);

        ColumnQualities qualities = m_columnQualities[i];

        bool numeric    = (qualities & ColumnNumeric);
        bool integral   = (qualities & ColumnIntegral);
        bool increasing = (qualities & ColumnIncreasing);
        bool large      = (qualities & ColumnLarge);

        bool timingColumn = (numeric && increasing);

        QString heading;
        UnitDatabase *udb = UnitDatabase::getInstance();

        if (m_columnHeadings.find(i) != m_columnHeadings.end()) {

            QString headingAsSeen = m_columnHeadings[i];

            if (headingAsSeen != "") {
                heading = headingAsSeen.split(' ', Qt::SkipEmptyParts)[0]
                    .toLower();
            }
            
            QString possibleUnit;
            if (udb->getUnitId(headingAsSeen, false) >= 0) {
                possibleUnit = headingAsSeen;
            } else if (headingAsSeen.contains('(')) {
                QString test = headingAsSeen;
                test.replace(QRegularExpression("^[^(]*\\(([^)]+)\\)$"), "\\1");
                if (test != "") {
                    SVDEBUG << "Extracted possible unit \"" << test
                            << "\" from heading \"" << headingAsSeen << "\""
                            << endl;
                    if (udb->getUnitId(test, false) >= 0) {
                        possibleUnit = test;
                    } else {
                        SVDEBUG << "(but it isn't recognised)" << endl;
                    }
                }
            } else if (heading == "frequency") {
                possibleUnit = "Hz";
            }
            if (possibleUnit != "") {
                m_columnPossibleUnits[i] = possibleUnit;
            }
        }
        
        if (heading == "time" || heading == "frame" ||
            heading == "duration" || heading == "endtime") {
            timingColumn = true;
        }

        if (heading == "value" || heading == "height" || heading == "label") {
            timingColumn = false;
        }
        
        if (timingColumn) {

            ++timingColumnCount;

            if (heading == "endtime") {

                purpose = ColumnEndTime;
                haveDurationOrEndTime = true;

            } else if (heading == "duration") {

                purpose = ColumnDuration;
                haveDurationOrEndTime = true;
                              
            } else if (primary || heading == "time" || heading == "frame") {

                purpose = ColumnStartTime;
                m_timingType = ExplicitTiming;

                if ((integral && large) || heading == "frame") {
                    m_timeUnits = TimeAudioFrames;
                } else {
                    m_timeUnits = TimeSeconds;
                }

            } else if (timingColumnCount == 2 &&
                       m_timingType == ExplicitTiming) {
                purpose = ColumnEndTime;
                haveDurationOrEndTime = true;
            }
        }

        if (purpose == ColumnUnknown) {
            if (heading == "label") {
                purpose = ColumnLabel;
            } else if (numeric || heading == "value" || heading == "height") {
                purpose = ColumnValue;
            } else {
                purpose = ColumnLabel;
            }
        }

        setColumnPurpose(i, purpose);
    }            

    int valueCount = 0;
    for (int i = 0; i < m_columnCount; ++i) {
        if (m_columnPurposes[i] == ColumnValue) {
            ++valueCount;
        }
    }

    if (valueCount == 2 && timingColumnCount == 1) {
        // If we have exactly two apparent value columns and only one
        // timing column, but one value column is integral and the
        // other is not, guess that whichever one matches the integral
        // status of the time column is either duration or end time
        if (m_timingType == ExplicitTiming) {
            int a = -1, b = -1;
            for (int i = 0; i < m_columnCount; ++i) {
                if (m_columnPurposes[i] == ColumnValue) {
                    if (a == -1) a = i;
                    else b = i;
                }
            }
            if ((m_columnQualities[a] & ColumnIntegral) !=
                (m_columnQualities[b] & ColumnIntegral)) {
                int timecol = a;
                if ((m_columnQualities[a] & ColumnIntegral) !=
                    (m_columnQualities[0] & ColumnIntegral)) {
                    timecol = b;
                }
                if (m_columnQualities[timecol] & ColumnIncreasing) {
                    // This shouldn't happen; should have been settled above
                    m_columnPurposes[timecol] = ColumnEndTime;
                    haveDurationOrEndTime = true;
                } else {
                    m_columnPurposes[timecol] = ColumnDuration;
                    haveDurationOrEndTime = true;
                }
                --valueCount;
            }
        }
    }

    if (timingColumnCount > 1 || haveDurationOrEndTime) {
        m_modelType = TwoDimensionalModelWithDuration;
    } else {
        if (valueCount == 0) {
            m_modelType = OneDimensionalModel;
        } else if (valueCount == 1) {
            m_modelType = TwoDimensionalModel;
        } else {
            m_modelType = ThreeDimensionalModel;
        }
    }

    updateScaleUnits();
    
    SVDEBUG << "Estimated column purposes: ";
    for (int i = 0; i < m_columnCount; ++i) {
        SVDEBUG << int(m_columnPurposes[i]) << " ";
    }
    SVDEBUG << endl;

    SVDEBUG << "Estimated model type: " << m_modelType << endl;
    SVDEBUG << "Estimated timing type: " << m_timingType << endl;
    SVDEBUG << "Estimated time units: " << m_timeUnits << endl;
    SVDEBUG << "Estimated scale units: " << m_scaleUnits << endl;
}

void
CSVFormat::guessAudioSampleRange()
{
    AudioSampleRange range = SampleRangeSigned1;
    
    range = SampleRangeSigned1;
    bool knownSigned = false;
    bool knownNonIntegral = false;

    SVDEBUG << "CSVFormat::guessAudioSampleRange: starting with assumption of "
            << range << endl;
    
    for (int i = 0; i < m_columnCount; ++i) {
        if (m_columnPurposes[i] != ColumnValue) {
            SVDEBUG << "... column " << i
                    << " is not apparently a value, ignoring" << endl;
            continue;
        }
        if (!(m_columnQualities[i] & ColumnIntegral)) {
            knownNonIntegral = true;
            if (range == SampleRangeUnsigned255 ||
                range == SampleRangeSigned32767) {
                range = SampleRangeOther;
            }
            SVDEBUG << "... column " << i
                    << " is non-integral, updating range to " << range << endl;
        }
        if (m_columnQualities[i] & ColumnLarge) {
            if (range == SampleRangeSigned1 ||
                range == SampleRangeUnsigned255) {
                if (knownNonIntegral) {
                    range = SampleRangeOther;
                } else {
                    range = SampleRangeSigned32767;
                }
            }
            SVDEBUG << "... column " << i << " is large, updating range to "
                    << range << endl;
        }
        if (m_columnQualities[i] & ColumnSigned) {
            knownSigned = true;
            if (range == SampleRangeUnsigned255) {
                range = SampleRangeSigned32767;
            }
            SVDEBUG << "... column " << i << " is signed, updating range to "
                    << range << endl;
        }
        if (!(m_columnQualities[i] & ColumnSmall)) {
            if (range == SampleRangeSigned1) {
                if (knownNonIntegral) {
                    range = SampleRangeOther;
                } else if (knownSigned) {
                    range = SampleRangeSigned32767;
                } else {
                    range = SampleRangeUnsigned255;
                }
            }
            SVDEBUG << "... column " << i << " is not small, updating range to "
                    << range << endl;
        }
    }

    SVDEBUG << "CSVFormat::guessAudioSampleRange: ended up with range "
            << range << endl;
    
    m_audioSampleRange = range;
}

QList<CSVFormat::ColumnPurpose>
CSVFormat::getColumnPurposes() const
{
    QList<ColumnPurpose> purposes;
    for (int i = 0; i < m_columnCount; ++i) {
        purposes.push_back(getColumnPurpose(i));
    }
    return purposes;
}

void
CSVFormat::setColumnPurposes(QList<ColumnPurpose> cl)
{
    m_columnPurposes.clear();
    for (int i = 0; in_range_for(cl, i); ++i) {
        setColumnPurpose(i, cl[i]);
    }
}

CSVFormat::ColumnPurpose
CSVFormat::getColumnPurpose(int i) const
{
    if (m_columnPurposes.find(i) == m_columnPurposes.end()) {
        return ColumnUnknown;
    } else {
        return m_columnPurposes.at(i);
    }
}

void
CSVFormat::setColumnPurpose(int i, ColumnPurpose p)
{
    m_columnPurposes[i] = p;
    updateScaleUnits();
}

QList<CSVFormat::ColumnQualities>
CSVFormat::getColumnQualities() const
{
    QList<ColumnQualities> qualities;
    for (int i = 0; i < m_columnCount; ++i) {
        if (m_columnQualities.find(i) == m_columnQualities.end()) {
            qualities.push_back(0);
        } else {
            qualities.push_back(m_columnQualities.at(i));
        }
    }
    return qualities;
}

void
CSVFormat::updateScaleUnits()
{
    m_scaleUnits = "";
    for (int i = 0; i < m_columnCount; ++i) {
        if (m_columnPurposes[i] == ColumnValue &&
            m_columnPossibleUnits.find(i) != m_columnPossibleUnits.end()) {
            m_scaleUnits = m_columnPossibleUnits[i];
        }
    }
}

