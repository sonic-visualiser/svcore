/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "base/XmlExportable.h"
#include "base/Window.h"

#include <vamp-sdk/RealTime.h>

#include <QString>

typedef QString TransformId;

namespace Vamp {
    class PluginBase;
}

class Transform : public XmlExportable
{
public:
    Transform();
    virtual ~Transform();

    void setIdentifier(TransformId id) { m_id = id; }
    TransformId getIdentifier() const { return m_id; }
    
    void setPlugin(QString pluginIdentifier);
    void setOutput(QString output);

    enum Type { FeatureExtraction, RealTimeEffect };

    Type getType() const;
    QString getPluginIdentifier() const;
    QString getOutput() const;

    typedef std::map<QString, float> ParameterMap;
    
    ParameterMap getParameters() const { return m_parameters; }
    void setParameters(const ParameterMap &pm) { m_parameters = pm; }

    typedef std::map<QString, QString> ConfigurationMap;

    ConfigurationMap getConfiguration() const { return m_configuration; }
    void setConfiguration(const ConfigurationMap &cm) { m_configuration = cm; }

    QString getProgram() const { return m_program; }
    void setProgram(QString program) { m_program = program; }
    
    size_t getStepSize() const { return m_stepSize; }
    void setStepSize(size_t s) { m_stepSize = s; }
    
    size_t getBlockSize() const { return m_blockSize; }
    void setBlockSize(size_t s) { m_blockSize = s; }

    WindowType getWindowType() const { return m_windowType; }
    void setWindowType(WindowType type) { m_windowType = type; }

    Vamp::RealTime getStartTime() const { return m_startTime; }
    void setStartTime(Vamp::RealTime t) { m_startTime = t; }

    Vamp::RealTime getDuration() const { return m_duration; } // 0 -> all
    void setDuration(Vamp::RealTime d) { m_duration = d; }
    
    float getSampleRate() const { return m_sampleRate; } // 0 -> as input
    void setSampleRate(float rate) { m_sampleRate = rate; }

    void toXml(QTextStream &stream, QString indent = "",
               QString extraAttributes = "") const;

    static Transform fromXmlString(QString xml);

protected:
    TransformId m_id; // pluginid:output, that is type:soname:label:output
    
    static QString createIdentifier
    (QString type, QString soName, QString label, QString output);

    static void parseIdentifier
    (QString identifier,
     QString &type, QString &soName, QString &label, QString &output);

    ParameterMap m_parameters;
    ConfigurationMap m_configuration;
    QString m_program;
    size_t m_stepSize;
    size_t m_blockSize;
    WindowType m_windowType;
    Vamp::RealTime m_startTime;
    Vamp::RealTime m_duration;
    float m_sampleRate;
};

#endif

