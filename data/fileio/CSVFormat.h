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

#ifndef _CSV_FORMAT_H_
#define _CSV_FORMAT_H_

#include <QString>
#include <QStringList>

class CSVFormat
{
public:
    enum ModelType {
	OneDimensionalModel,
	TwoDimensionalModel,
        TwoDimensionalModelWithDuration,
	ThreeDimensionalModel
    };
    
    enum TimingType {
	ExplicitTiming,
	ImplicitTiming
    };

    enum DurationType {
        Durations,
        EndTimes
    };
    
    enum TimeUnits {
	TimeSeconds,
	TimeAudioFrames,
	TimeWindows
    };

    CSVFormat(QString path); // guess format

    CSVFormat() : // arbitrary defaults
        m_modelType(TwoDimensionalModel),
        m_timingType(ExplicitTiming),
        m_durationType(Durations),
        m_timeUnits(TimeSeconds),
        m_separator(","),
        m_sampleRate(44100),
        m_windowSize(1024),
        m_behaviour(QString::KeepEmptyParts)
    { }
 
    ModelType    getModelType()     const { return m_modelType;     }
    TimingType   getTimingType()    const { return m_timingType;    }
    DurationType getDurationType()  const { return m_durationType;  }
    TimeUnits    getTimeUnits()     const { return m_timeUnits;     }
    QString      getSeparator()     const { return m_separator;     }
    size_t       getSampleRate()    const { return m_sampleRate;    }
    size_t       getWindowSize()    const { return m_windowSize;    }

    QString::SplitBehavior getSplitBehaviour() const { return m_behaviour; }
	
    void setModelType(ModelType t)        { m_modelType    = t; }
    void setTimingType(TimingType t)      { m_timingType   = t; }
    void setDurationType(DurationType t)  { m_durationType = t; }
    void setTimeUnits(TimeUnits t)        { m_timeUnits    = t; }
    void setSeparator(QString s)          { m_separator    = s; }
    void setSampleRate(size_t r)          { m_sampleRate   = r; }
    void setWindowSize(size_t s)          { m_windowSize   = s; }

    void setSplitBehaviour(QString::SplitBehavior b) { m_behaviour = b; }
    
    // only valid if constructor that guesses format was used:
    QList<QStringList> getExample() const { return m_example; }
    int getMaxExampleCols() const { return m_maxExampleCols; }

protected:
    ModelType    m_modelType;
    TimingType   m_timingType;
    DurationType m_durationType;
    TimeUnits    m_timeUnits;
    QString      m_separator;
    size_t       m_sampleRate;
    size_t       m_windowSize;

    QString::SplitBehavior m_behaviour;

    QList<QStringList> m_example;
    int m_maxExampleCols;
};

#endif
