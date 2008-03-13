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

#ifndef _CSV_FILE_READER_H_
#define _CSV_FILE_READER_H_

#include "DataFileReader.h"

#include <QList>
#include <QStringList>
#include <QDialog>

class QFile;
class QTableWidget;
class QComboBox;
class QLabel;


class CSVFileReader : public DataFileReader
{
public:
    CSVFileReader(QString path, size_t mainModelSampleRate);
    virtual ~CSVFileReader();

    virtual bool isOK() const;
    virtual QString getError() const;
    virtual Model *load() const;

protected:
    QFile *m_file;
    QString m_error;
    size_t m_mainModelSampleRate;
};


class CSVFormatDialog : public QDialog
{
    Q_OBJECT
    
public:
    CSVFormatDialog(QWidget *parent, QFile *file, size_t defaultSampleRate);
    
    ~CSVFormatDialog();
    
    enum ModelType {
	OneDimensionalModel,
	TwoDimensionalModel,
	ThreeDimensionalModel
    };
    
    enum TimingType {
	ExplicitTiming,
	ImplicitTiming
    };
    
    enum TimeUnits {
	TimeSeconds,
	TimeAudioFrames,
	TimeWindows
    };

    ModelType  getModelType()   const { return m_modelType;   }
    TimingType getTimingType()  const { return m_timingType;  }
    TimeUnits  getTimeUnits()   const { return m_timeUnits;   }
    QString    getSeparator()   const { return m_separator;   }
    size_t     getSampleRate()  const { return m_sampleRate;  }
    size_t     getWindowSize()  const { return m_windowSize;  }

    QString::SplitBehavior getSplitBehaviour() const { return m_behaviour; }

protected slots:
    void modelTypeChanged(int type);
    void timingTypeChanged(int type);
    void sampleRateChanged(QString);
    void windowSizeChanged(QString);

protected:
    ModelType  m_modelType;
    TimingType m_timingType;
    TimeUnits  m_timeUnits;
    QString    m_separator;
    size_t     m_sampleRate;
    size_t     m_windowSize;

    QString::SplitBehavior m_behaviour;
    
    QList<QStringList> m_example;
    int m_maxExampleCols;
    QTableWidget *m_exampleWidget;
    
    QComboBox *m_modelTypeCombo;
    QComboBox *m_timingTypeCombo;
    QLabel *m_sampleRateLabel;
    QComboBox *m_sampleRateCombo;
    QLabel *m_windowSizeLabel;
    QComboBox *m_windowSizeCombo;

    bool guessFormat(QFile *file);
    void populateExample();
};

#endif

