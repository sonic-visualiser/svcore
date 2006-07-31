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

#include <QFile>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <QFrame>
#include <QGridLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>

#include <iostream>

CSVFileReader::CSVFileReader(QString path, size_t mainModelSampleRate) :
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

    CSVFormatDialog *dialog = new CSVFormatDialog
	(0, m_file, m_mainModelSampleRate);

    if (dialog->exec() == QDialog::Rejected) {
	delete dialog;
	return 0;
    }

    CSVFormatDialog::ModelType   modelType = dialog->getModelType();
    CSVFormatDialog::TimingType timingType = dialog->getTimingType();
    CSVFormatDialog::TimeUnits   timeUnits = dialog->getTimeUnits();
    QString separator = dialog->getSeparator();
    size_t sampleRate = dialog->getSampleRate();
    size_t windowSize = dialog->getWindowSize();

    delete dialog;

    if (timingType == CSVFormatDialog::ExplicitTiming) {
	windowSize = 1;
	if (timeUnits == CSVFormatDialog::TimeSeconds) {
	    sampleRate = m_mainModelSampleRate;
	}
    }

    SparseOneDimensionalModel *model1 = 0;
    SparseTimeValueModel *model2 = 0;
    EditableDenseThreeDimensionalModel *model3 = 0;
    Model *model = 0;

    QTextStream in(m_file);
    in.seek(0);

    unsigned int warnings = 0, warnLimit = 10;
    unsigned int lineno = 0;

    float min = 0.0, max = 0.0;

    size_t frameNo = 0;

    while (!in.atEnd()) {

	QString line = in.readLine().trimmed();
	if (line.startsWith("#")) continue;

	QStringList list = line.split(separator);

	if (!model) {

	    switch (modelType) {

	    case CSVFormatDialog::OneDimensionalModel:
		model1 = new SparseOneDimensionalModel(sampleRate, windowSize);
		model = model1;
		break;
		
	    case CSVFormatDialog::TwoDimensionalModel:
		model2 = new SparseTimeValueModel(sampleRate, windowSize,
						  0.0, 0.0,
						  false);
		model = model2;
		break;
		
	    case CSVFormatDialog::ThreeDimensionalModel:
		model3 = new EditableDenseThreeDimensionalModel(sampleRate,
                                                                windowSize,
                                                                list.size());
		model = model3;
		break;
	    }
	}

	QStringList tidyList;
        QRegExp nonNumericRx("[^0-9.,+-]");

	for (int i = 0; i < list.size(); ++i) {
	    
	    QString s(list[i].trimmed());

	    if (s.length() >= 2 && s.startsWith("\"") && s.endsWith("\"")) {
		s = s.mid(1, s.length() - 2);
	    } else if (s.length() >= 2 && s.startsWith("'") && s.endsWith("'")) {
		s = s.mid(1, s.length() - 2);
	    }

	    if (i == 0 && timingType == CSVFormatDialog::ExplicitTiming) {

		bool ok = false;
                QString numeric = s;
                numeric.remove(nonNumericRx);

		if (timeUnits == CSVFormatDialog::TimeSeconds) {

		    double time = numeric.toDouble(&ok);
		    frameNo = int(time * sampleRate + 0.00001);

		} else {

		    frameNo = numeric.toInt(&ok);

		    if (timeUnits == CSVFormatDialog::TimeWindows) {
			frameNo *= windowSize;
		    }
		}
			       
		if (!ok) {
		    if (warnings < warnLimit) {
			std::cerr << "WARNING: CSVFileReader::load: "
				  << "Bad time format (\"" << s.toStdString()
				  << "\") in data line "
				  << lineno << ":" << std::endl;
			std::cerr << line.toStdString() << std::endl;
		    } else if (warnings == warnLimit) {
			std::cerr << "WARNING: Too many warnings" << std::endl;
		    }
                    ++warnings;
		}
	    } else {
		tidyList.push_back(s);
	    }
	}

	if (modelType == CSVFormatDialog::OneDimensionalModel) {
	    
	    SparseOneDimensionalModel::Point point
		(frameNo,
		 tidyList.size() > 0 ? tidyList[tidyList.size()-1] :
		 QString("%1").arg(lineno));

	    model1->addPoint(point);

	} else if (modelType == CSVFormatDialog::TwoDimensionalModel) {

	    SparseTimeValueModel::Point point
		(frameNo,
		 tidyList.size() > 0 ? tidyList[0].toFloat() : 0.0,
		 tidyList.size() > 1 ? tidyList[1] : QString("%1").arg(lineno));

	    model2->addPoint(point);

	} else if (modelType == CSVFormatDialog::ThreeDimensionalModel) {

	    DenseThreeDimensionalModel::BinValueSet values;

	    for (int i = 0; i < tidyList.size(); ++i) {

		bool ok = false;
		float value = list[i].toFloat(&ok);
		values.push_back(value);
	    
		if ((lineno == 0 && i == 0) || value < min) min = value;
		if ((lineno == 0 && i == 0) || value > max) max = value;

		if (!ok) {
		    if (warnings < warnLimit) {
			std::cerr << "WARNING: CSVFileReader::load: "
				  << "Non-numeric value in data line " << lineno
				  << ":" << std::endl;
			std::cerr << line.toStdString() << std::endl;
			++warnings;
		    } else if (warnings == warnLimit) {
			std::cerr << "WARNING: Too many warnings" << std::endl;
		    }
		}
	    }
	
	    std::cerr << "Setting bin values for count " << lineno << ", frame "
		      << frameNo << ", time " << RealTime::frame2RealTime(frameNo, sampleRate) << std::endl;

	    model3->setBinValues(frameNo, values);
	}

	++lineno;
	if (timingType == CSVFormatDialog::ImplicitTiming ||
	    list.size() == 0) {
	    frameNo += windowSize;
	}
    }

    if (modelType == CSVFormatDialog::ThreeDimensionalModel) {
	model3->setMinimumLevel(min);
	model3->setMaximumLevel(max);
    }

    return model;
}


CSVFormatDialog::CSVFormatDialog(QWidget *parent, QFile *file,
				 size_t defaultSampleRate) :
    QDialog(parent),
    m_modelType(OneDimensionalModel),
    m_timingType(ExplicitTiming),
    m_timeUnits(TimeAudioFrames),
    m_separator("")
{
    setModal(true);
    setWindowTitle(tr("Select Data Format"));

    (void)guessFormat(file);

    QGridLayout *layout = new QGridLayout;

    layout->addWidget(new QLabel(tr("\nPlease select the correct data format for this file.\n")),
		      0, 0, 1, 4);

    layout->addWidget(new QLabel(tr("Each row specifies:")), 1, 0);

    m_modelTypeCombo = new QComboBox;
    m_modelTypeCombo->addItem(tr("A point in time"));
    m_modelTypeCombo->addItem(tr("A value at a time"));
    m_modelTypeCombo->addItem(tr("A set of values"));
    layout->addWidget(m_modelTypeCombo, 1, 1, 1, 2);
    connect(m_modelTypeCombo, SIGNAL(activated(int)),
	    this, SLOT(modelTypeChanged(int)));
    m_modelTypeCombo->setCurrentIndex(int(m_modelType));

    layout->addWidget(new QLabel(tr("The first column contains:")), 2, 0);
    
    m_timingTypeCombo = new QComboBox;
    m_timingTypeCombo->addItem(tr("Time, in seconds"));
    m_timingTypeCombo->addItem(tr("Time, in audio sample frames"));
    m_timingTypeCombo->addItem(tr("Data (rows are consecutive in time)"));
    layout->addWidget(m_timingTypeCombo, 2, 1, 1, 2);
    connect(m_timingTypeCombo, SIGNAL(activated(int)),
	    this, SLOT(timingTypeChanged(int)));
    m_timingTypeCombo->setCurrentIndex(m_timingType == ExplicitTiming ?
                                       m_timeUnits == TimeSeconds ? 0 : 1 : 2);

    m_sampleRateLabel = new QLabel(tr("Audio sample rate (Hz):"));
    layout->addWidget(m_sampleRateLabel, 3, 0);
    
    size_t sampleRates[] = {
	8000, 11025, 12000, 22050, 24000, 32000,
	44100, 48000, 88200, 96000, 176400, 192000
    };

    m_sampleRateCombo = new QComboBox;
    m_sampleRate = defaultSampleRate;
    for (size_t i = 0; i < sizeof(sampleRates) / sizeof(sampleRates[0]); ++i) {
	m_sampleRateCombo->addItem(QString("%1").arg(sampleRates[i]));
	if (sampleRates[i] == m_sampleRate) m_sampleRateCombo->setCurrentIndex(i);
    }
    m_sampleRateCombo->setEditable(true);

    layout->addWidget(m_sampleRateCombo, 3, 1);
    connect(m_sampleRateCombo, SIGNAL(activated(QString)),
	    this, SLOT(sampleRateChanged(QString)));
    connect(m_sampleRateCombo, SIGNAL(editTextChanged(QString)),
	    this, SLOT(sampleRateChanged(QString)));

    m_windowSizeLabel = new QLabel(tr("Frame increment between rows:"));
    layout->addWidget(m_windowSizeLabel, 4, 0);

    m_windowSizeCombo = new QComboBox;
    m_windowSize = 1024;
    for (int i = 0; i <= 16; ++i) {
	int value = 1 << i;
	m_windowSizeCombo->addItem(QString("%1").arg(value));
	if (value == m_windowSize) m_windowSizeCombo->setCurrentIndex(i);
    }
    m_windowSizeCombo->setEditable(true);

    layout->addWidget(m_windowSizeCombo, 4, 1);
    connect(m_windowSizeCombo, SIGNAL(activated(QString)),
	    this, SLOT(windowSizeChanged(QString)));
    connect(m_windowSizeCombo, SIGNAL(editTextChanged(QString)),
	    this, SLOT(windowSizeChanged(QString)));

    layout->addWidget(new QLabel(tr("\nExample data from file:")), 5, 0, 1, 4);

    m_exampleWidget = new QTableWidget
	(std::min(10, m_example.size()), m_maxExampleCols);

    layout->addWidget(m_exampleWidget, 6, 0, 1, 4);
    layout->setColumnStretch(3, 10);
    layout->setRowStretch(4, 10);

    QPushButton *ok = new QPushButton(tr("OK"));
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    ok->setDefault(true);

    QPushButton *cancel = new QPushButton(tr("Cancel"));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(ok);
    buttonLayout->addWidget(cancel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(layout);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    
    timingTypeChanged(m_timingTypeCombo->currentIndex());
}

CSVFormatDialog::~CSVFormatDialog()
{
}

void
CSVFormatDialog::populateExample()
{
    m_exampleWidget->setColumnCount
	(m_timingType == ExplicitTiming ?
	 m_maxExampleCols - 1 : m_maxExampleCols);

    m_exampleWidget->setHorizontalHeaderLabels(QStringList());

    for (int i = 0; i < m_example.size(); ++i) {
	for (int j = 0; j < m_example[i].size(); ++j) {

	    QTableWidgetItem *item = new QTableWidgetItem(m_example[i][j]);

	    if (j == 0) {
		if (m_timingType == ExplicitTiming) {
		    m_exampleWidget->setVerticalHeaderItem(i, item);
		    continue;
		} else {
		    QTableWidgetItem *header =
			new QTableWidgetItem(QString("%1").arg(i));
		    header->setFlags(Qt::ItemIsEnabled);
		    m_exampleWidget->setVerticalHeaderItem(i, header);
		}
	    }
	    int index = j;
	    if (m_timingType == ExplicitTiming) --index;
	    item->setFlags(Qt::ItemIsEnabled);
	    m_exampleWidget->setItem(i, index, item);
	}
    }
}

void
CSVFormatDialog::modelTypeChanged(int type)
{
    m_modelType = (ModelType)type;

    if (m_modelType == ThreeDimensionalModel) {
        // We can't load 3d models with explicit timing, because the 3d
        // model is dense so we need a fixed sample increment
        m_timingTypeCombo->setCurrentIndex(2);
        timingTypeChanged(2);
    }
}

void
CSVFormatDialog::timingTypeChanged(int type)
{
    switch (type) {

    case 0:
	m_timingType = ExplicitTiming;
	m_timeUnits = TimeSeconds;
	m_sampleRateCombo->setEnabled(false);
	m_sampleRateLabel->setEnabled(false);
	m_windowSizeCombo->setEnabled(false);
	m_windowSizeLabel->setEnabled(false);
        if (m_modelType == ThreeDimensionalModel) {
            m_modelTypeCombo->setCurrentIndex(1);
            modelTypeChanged(1);
        }
	break;

    case 1:
	m_timingType = ExplicitTiming;
	m_timeUnits = TimeAudioFrames;
	m_sampleRateCombo->setEnabled(true);
	m_sampleRateLabel->setEnabled(true);
	m_windowSizeCombo->setEnabled(false);
	m_windowSizeLabel->setEnabled(false);
        if (m_modelType == ThreeDimensionalModel) {
            m_modelTypeCombo->setCurrentIndex(1);
            modelTypeChanged(1);
        }
	break;

    case 2:
	m_timingType = ImplicitTiming;
	m_timeUnits = TimeWindows;
	m_sampleRateCombo->setEnabled(true);
	m_sampleRateLabel->setEnabled(true);
	m_windowSizeCombo->setEnabled(true);
	m_windowSizeLabel->setEnabled(true);
	break;
    }

    populateExample();
}

void
CSVFormatDialog::sampleRateChanged(QString rateString)
{
    bool ok = false;
    int sampleRate = rateString.toInt(&ok);
    if (ok) m_sampleRate = sampleRate;
}

void
CSVFormatDialog::windowSizeChanged(QString sizeString)
{
    bool ok = false;
    int size = sizeString.toInt(&ok);
    if (ok) m_windowSize = size;
}

bool
CSVFormatDialog::guessFormat(QFile *file)
{
    QTextStream in(file);
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

    while (!in.atEnd()) {
	
	QString line = in.readLine().trimmed();
	if (line.startsWith("#")) continue;

	if (m_separator == "") {
	    //!!! to do: ask the user
	    if (line.split(",").size() >= 2) m_separator = ",";
	    else if (line.split("\t").size() >= 2) m_separator = "\t";
	    else if (line.split("|").size() >= 2) m_separator = "|";
	    else if (line.split("/").size() >= 2) m_separator = "/";
	    else if (line.split(":").size() >= 2) m_separator = ":";
	    else m_separator = " ";
	}

	QStringList list = line.split(m_separator);
	QStringList tidyList;

	for (int i = 0; i < list.size(); ++i) {
	    
	    QString s(list[i]);
	    bool numeric = false;

	    if (s.length() >= 2 && s.startsWith("\"") && s.endsWith("\"")) {
		s = s.mid(1, s.length() - 2);
	    } else if (s.length() >= 2 && s.startsWith("'") && s.endsWith("'")) {
		s = s.mid(1, s.length() - 2);
	    } else {
		(void)s.toFloat(&numeric);
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

    if (nonNumericPrimaries || nonIncreasingPrimaries) {
	
	// Primaries are probably not a series of times

	m_timingType = ImplicitTiming;
	m_timeUnits = TimeWindows;
	
	if (nonNumericPrimaries) {
	    m_modelType = OneDimensionalModel;
	} else if (itemCount == 1 || variableItemCount ||
		   (earliestNonNumericItem != -1)) {
	    m_modelType = TwoDimensionalModel;
	} else {
	    m_modelType = ThreeDimensionalModel;
	}

    } else {

	// Increasing numeric primaries -- likely to be time

	m_timingType = ExplicitTiming;

	if (floatPrimaries) {
	    m_timeUnits = TimeSeconds;
	} else {
	    m_timeUnits = TimeAudioFrames;
	}

	if (itemCount == 1) {
	    m_modelType = OneDimensionalModel;
	} else if (variableItemCount || (earliestNonNumericItem != -1)) {
	    if (earliestNonNumericItem != -1 && earliestNonNumericItem < 2) {
		m_modelType = OneDimensionalModel;
	    } else {
		m_modelType = TwoDimensionalModel;
	    }
	} else {
	    m_modelType = ThreeDimensionalModel;
	}
    }

    std::cerr << "Estimated model type: " << m_modelType << std::endl;
    std::cerr << "Estimated timing type: " << m_timingType << std::endl;
    std::cerr << "Estimated units: " << m_timeUnits << std::endl;

    in.seek(0);
    return true;
}
