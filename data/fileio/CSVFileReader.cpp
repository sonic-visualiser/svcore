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

#include "CSVFileReader.h"

#include "model/Model.h"
#include "base/RealTime.h"
#include "model/SparseOneDimensionalModel.h"
#include "model/SparseTimeValueModel.h"
#include "model/EditableDenseThreeDimensionalModel.h"
#include "model/RegionModel.h"
#include "DataFileReaderFactory.h"

#include <QFile>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>

#include <iostream>
#include <map>

CSVFileReader::CSVFileReader(QString path, CSVFormat format,
                             size_t mainModelSampleRate) :
    m_format(format),
    m_file(0),
    m_mainModelSampleRate(mainModelSampleRate)
{
    m_file = new QFile(path);
    bool good = false;
    
    if (!m_file->exists()) {
	m_error = QFile::tr("File \"%1\" does not exist").arg(path);
    } else if (!m_file->open(QIODevice::ReadOnly | QIODevice::Text)) {
	m_error = QFile::tr("Failed to open file \"%1\"").arg(path);
    } else {
	good = true;
    }

    if (!good) {
	delete m_file;
	m_file = 0;
    }
}

CSVFileReader::~CSVFileReader()
{
    std::cerr << "CSVFileReader::~CSVFileReader: file is " << m_file << std::endl;

    if (m_file) {
        std::cerr << "CSVFileReader::CSVFileReader: Closing file" << std::endl;
        m_file->close();
    }
    delete m_file;
}

bool
CSVFileReader::isOK() const
{
    return (m_file != 0);
}

QString
CSVFileReader::getError() const
{
    return m_error;
}

Model *
CSVFileReader::load() const
{
    if (!m_file) return 0;
/*!!!
    CSVFormatDialog *dialog = new CSVFormatDialog
	(0, m_file, m_mainModelSampleRate);

    if (dialog->exec() == QDialog::Rejected) {
	delete dialog;
        throw DataFileReaderFactory::ImportCancelled;
    }
*/

    CSVFormat::ModelType modelType = m_format.getModelType();
    CSVFormat::TimingType timingType = m_format.getTimingType();
    CSVFormat::DurationType durationType = m_format.getDurationType();
    CSVFormat::TimeUnits timeUnits = m_format.getTimeUnits();
    QString separator = m_format.getSeparator();
    QString::SplitBehavior behaviour = m_format.getSplitBehaviour();
    size_t sampleRate = m_format.getSampleRate();
    size_t windowSize = m_format.getWindowSize();

    if (timingType == CSVFormat::ExplicitTiming) {
        if (modelType == CSVFormat::ThreeDimensionalModel) {
            // This will be overridden later if more than one line
            // appears in our file, but we want to choose a default
            // that's likely to be visible
            windowSize = 1024;
        } else {
            windowSize = 1;
        }
	if (timeUnits == CSVFormat::TimeSeconds) {
	    sampleRate = m_mainModelSampleRate;
	}
    }

    SparseOneDimensionalModel *model1 = 0;
    SparseTimeValueModel *model2 = 0;
    RegionModel *model2a = 0;
    EditableDenseThreeDimensionalModel *model3 = 0;
    Model *model = 0;

    QTextStream in(m_file);
    in.seek(0);

    unsigned int warnings = 0, warnLimit = 10;
    unsigned int lineno = 0;

    float min = 0.0, max = 0.0;

    size_t frameNo = 0;
    size_t duration = 0;
    size_t startFrame = 0; // for calculation of dense model resolution

    std::map<QString, float> labelValueMap;
    float syntheticMax = 0.f;

    while (!in.atEnd()) {

        // QTextStream's readLine doesn't cope with old-style Mac
        // CR-only line endings.  Why did they bother making the class
        // cope with more than one sort of line ending, if it still
        // can't be configured to cope with all the common sorts?

        // For the time being we'll deal with this case (which is
        // relatively uncommon for us, but still necessary to handle)
        // by reading the entire file using a single readLine, and
        // splitting it.  For CR and CR/LF line endings this will just
        // read a line at a time, and that's obviously OK.

        QString chunk = in.readLine();
        QStringList lines = chunk.split('\r', QString::SkipEmptyParts);
        
        for (size_t li = 0; li < lines.size(); ++li) {

            QString line = lines[li];

            if (line.startsWith("#")) continue;

            QStringList list = line.split(separator, behaviour);

            if (!model) {

                switch (modelType) {

                case CSVFormat::OneDimensionalModel:
                    model1 = new SparseOneDimensionalModel(sampleRate, windowSize);
                    model = model1;
                    break;
		
                case CSVFormat::TwoDimensionalModel:
                    model2 = new SparseTimeValueModel(sampleRate, windowSize, false);
                    model = model2;
                    break;
		
                case CSVFormat::TwoDimensionalModelWithDuration:
                    model2a = new RegionModel(sampleRate, windowSize, false);
                    model = model2a;
                    break;
		
                case CSVFormat::ThreeDimensionalModel:
                    model3 = new EditableDenseThreeDimensionalModel
                        (sampleRate,
                         windowSize,
                         list.size(),
                         EditableDenseThreeDimensionalModel::NoCompression);
                    model = model3;
                    break;
                }
            }

            QStringList tidyList;
            QRegExp nonNumericRx("[^0-9eE.,+-]");

            float value = 0.f;

            for (int i = 0; i < list.size(); ++i) {
	    
                QString s(list[i].trimmed());

                if (s.length() >= 2 && s.startsWith("\"") && s.endsWith("\"")) {
                    s = s.mid(1, s.length() - 2);
                } else if (s.length() >= 2 && s.startsWith("'") && s.endsWith("'")) {
                    s = s.mid(1, s.length() - 2);
                }

                if (timingType == CSVFormat::ExplicitTiming) {

                    size_t calculatedFrame = 0;

                    if (i == 0 ||
                        (i == 1 &&
                         modelType == CSVFormat::TwoDimensionalModelWithDuration)) {

                        bool ok = false;
                        QString numeric = s;
                        numeric.remove(nonNumericRx);

                        if (timeUnits == CSVFormat::TimeSeconds) {

                            double time = numeric.toDouble(&ok);
                            calculatedFrame = int(time * sampleRate + 0.5);

                        } else {

                            calculatedFrame = numeric.toInt(&ok);

                            if (timeUnits == CSVFormat::TimeWindows) {
                                calculatedFrame *= windowSize;
                            }
                        }
			       
                        if (!ok) {
                            if (warnings < warnLimit) {
                                std::cerr << "WARNING: CSVFileReader::load: "
                                          << "Bad time format (\"" << s.toStdString()
                                          << "\") in data line "
                                          << lineno+1 << ":" << std::endl;
                                std::cerr << line.toStdString() << std::endl;
                            } else if (warnings == warnLimit) {
                                std::cerr << "WARNING: Too many warnings" << std::endl;
                            }
                            ++warnings;
                        }

                        if (i == 0) frameNo = calculatedFrame;
                        else {
                            if (durationType == CSVFormat::EndTimes) {
                                duration = calculatedFrame - frameNo;
                            } else {
                                duration = calculatedFrame;
                            }
                        }

                        continue;
                    }
                }

                if ((i == 1 &&
                     modelType == CSVFormat::TwoDimensionalModel) ||
                    (i == 2 &&
                     modelType == CSVFormat::TwoDimensionalModelWithDuration)) {
                    bool ok = false;
                    value = s.toFloat(&ok);
                    if (!ok) {
                        // cf. RDFImporter::fillModel
                        if (labelValueMap.find(s) == labelValueMap.end()) {
                            syntheticMax = syntheticMax + 1.f;
                            labelValueMap[s] = syntheticMax;
                        }
                        value = labelValueMap[s];
                    } else {
                        if (value > syntheticMax) syntheticMax = value;
                    }
                    if (i + 1 == list.size()) {
                        // keep text around for use as label (none other given)
                        tidyList.push_back(s);
                    }
                    continue;
                }

                tidyList.push_back(s);
            }

            if (modelType == CSVFormat::OneDimensionalModel) {
	    
                SparseOneDimensionalModel::Point point
                    (frameNo,
                     tidyList.size() > 0 ? tidyList[tidyList.size()-1] :
                     QString("%1").arg(lineno+1));

                model1->addPoint(point);

            } else if (modelType == CSVFormat::TwoDimensionalModel) {

                SparseTimeValueModel::Point point
                    (frameNo,
                     value,
                     tidyList.size() > 0 ? tidyList[0] : QString("%1").arg(lineno+1));

                model2->addPoint(point);

            } else if (modelType == CSVFormat::TwoDimensionalModelWithDuration) {

                RegionModel::Point point
                    (frameNo,
                     value,
                     duration,
                     tidyList.size() > 0 ? tidyList[0] : QString("%1").arg(lineno+1));

                model2a->addPoint(point);

            } else if (modelType == CSVFormat::ThreeDimensionalModel) {

                DenseThreeDimensionalModel::Column values;

                for (int i = 0; i < tidyList.size(); ++i) {

                    bool ok = false;
                    float value = list[i].toFloat(&ok);

                    if (i > 0 || timingType != CSVFormat::ExplicitTiming) {
                        values.push_back(value);
                    }
	    
                    bool firstEver = (lineno == 0 && i == 0);

                    if (firstEver || value < min) min = value;
                    if (firstEver || value > max) max = value;

                    if (firstEver) {
                        startFrame = frameNo;
                        model3->setStartFrame(startFrame);
                    } else if (lineno == 1 &&
                               timingType == CSVFormat::ExplicitTiming) {
                        model3->setResolution(frameNo - startFrame);
                    }

                    if (!ok) {
                        if (warnings < warnLimit) {
                            std::cerr << "WARNING: CSVFileReader::load: "
                                      << "Non-numeric value \""
                                      << list[i].toStdString()
                                      << "\" in data line " << lineno+1
                                      << ":" << std::endl;
                            std::cerr << line.toStdString() << std::endl;
                            ++warnings;
                        } else if (warnings == warnLimit) {
//                            std::cerr << "WARNING: Too many warnings" << std::endl;
                        }
                    }
                }
	
//                std::cerr << "Setting bin values for count " << lineno << ", frame "
//                          << frameNo << ", time " << RealTime::frame2RealTime(frameNo, sampleRate) << std::endl;

                model3->setColumn(lineno, values);
            }

            ++lineno;
            if (timingType == CSVFormat::ImplicitTiming ||
                list.size() == 0) {
                frameNo += windowSize;
            }
        }
    }

    if (modelType == CSVFormat::ThreeDimensionalModel) {
	model3->setMinimumLevel(min);
	model3->setMaximumLevel(max);
    }

    return model;
}

