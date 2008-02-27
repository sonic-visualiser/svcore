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

#include "Transform.h"

#include "plugin/PluginIdentifier.h"

#include "plugin/FeatureExtractionPluginFactory.h"

#include <QXmlAttributes>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QDomAttr>

#include <QTextStream>

#include <iostream>

Transform::Transform() :
    m_stepSize(0),
    m_blockSize(0),
    m_windowType(HanningWindow),
    m_sampleRate(0)
{
}

Transform::Transform(QString xml) :
    m_stepSize(0),
    m_blockSize(0),
    m_windowType(HanningWindow),
    m_sampleRate(0)
{
    QDomDocument doc;
    
    QString error;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(xml, false, &error, &errorLine, &errorColumn)) {
        std::cerr << "Transform::Transform: Error in parsing XML: "
                  << error.toStdString() << " at line " << errorLine
                  << ", column " << errorColumn << std::endl;
        std::cerr << "Input follows:" << std::endl;
        std::cerr << xml.toStdString() << std::endl;
        std::cerr << "Input ends." << std::endl;
        return;
    }
    
    QDomElement transformElt = doc.firstChildElement("transform");
    QDomNamedNodeMap attrNodes = transformElt.attributes();
    QXmlAttributes attrs;

    for (unsigned int i = 0; i < attrNodes.length(); ++i) {
        QDomAttr attr = attrNodes.item(i).toAttr();
        if (!attr.isNull()) attrs.append(attr.name(), "", "", attr.value());
    }

    setFromXmlAttributes(attrs);

    for (QDomElement paramElt = transformElt.firstChildElement("parameter");
         !paramElt.isNull();
         paramElt = paramElt.nextSiblingElement("parameter")) {

        QDomNamedNodeMap paramAttrs = paramElt.attributes();

        QDomAttr nameAttr = paramAttrs.namedItem("name").toAttr();
        if (nameAttr.isNull() || nameAttr.value() == "") continue;
        
        QDomAttr valueAttr = paramAttrs.namedItem("value").toAttr();
        if (valueAttr.isNull() || valueAttr.value() == "") continue;

        setParameter(nameAttr.value(), valueAttr.value().toFloat());
    }

    for (QDomElement configElt = transformElt.firstChildElement("configuration");
         !configElt.isNull();
         configElt = configElt.nextSiblingElement("configuration")) {

        QDomNamedNodeMap configAttrs = configElt.attributes();

        QDomAttr nameAttr = configAttrs.namedItem("name").toAttr();
        if (nameAttr.isNull() || nameAttr.value() == "") continue;
        
        QDomAttr valueAttr = configAttrs.namedItem("value").toAttr();
        if (valueAttr.isNull() || valueAttr.value() == "") continue;

        setConfigurationValue(nameAttr.value(), valueAttr.value());
    }
}

Transform::~Transform()
{
}

bool
Transform::operator==(const Transform &t)
{
    return 
        m_id == t.m_id &&
        m_parameters == t.m_parameters &&
        m_configuration == t.m_configuration &&
        m_program == t.m_program &&
        m_stepSize == t.m_stepSize &&
        m_blockSize == t.m_blockSize &&
        m_windowType == t.m_windowType &&
        m_startTime == t.m_startTime &&
        m_duration == t.m_duration &&
        m_sampleRate == t.m_sampleRate;
}

void
Transform::setIdentifier(TransformId id)
{
    m_id = id;
}

TransformId
Transform::getIdentifier() const
{
    return m_id;
}

QString
Transform::createIdentifier(QString type, QString soName, QString label,
                            QString output)
{
    QString pluginId = PluginIdentifier::createIdentifier(type, soName, label);
    return pluginId + ":" + output;
}

void
Transform::parseIdentifier(QString identifier,
                           QString &type, QString &soName,
                           QString &label, QString &output)
{
    output = identifier.section(':', 3);
    PluginIdentifier::parseIdentifier(identifier.section(':', 0, 2),
                                      type, soName, label);
}

Transform::Type
Transform::getType() const
{
    if (FeatureExtractionPluginFactory::instanceFor(getPluginIdentifier())) {
        return FeatureExtraction;
    } else {
        // We don't have an unknown/invalid return value, so always
        // return this
        return RealTimeEffect;
    }
}

QString
Transform::getPluginIdentifier() const
{
    return m_id.section(':', 0, 2);
}

QString
Transform::getOutput() const
{
    return m_id.section(':', 3);
}

void
Transform::setPluginIdentifier(QString pluginIdentifier)
{
    m_id = pluginIdentifier + ':' + getOutput();
}

void
Transform::setOutput(QString output)
{
    m_id = getPluginIdentifier() + ':' + output;
}

TransformId
Transform::getIdentifierForPluginOutput(QString pluginIdentifier,
                                        QString output)
{
    return pluginIdentifier + ':' + output;
}

const Transform::ParameterMap &
Transform::getParameters() const
{
    return m_parameters;
}

void
Transform::setParameters(const ParameterMap &pm)
{
    m_parameters = pm;
}

void
Transform::setParameter(QString name, float value)
{
    std::cerr << "Transform::setParameter(" << name.toStdString()
              << ") -> " << value << std::endl;
    m_parameters[name] = value;
}

const Transform::ConfigurationMap &
Transform::getConfiguration() const
{
    return m_configuration;
}

void
Transform::setConfiguration(const ConfigurationMap &cm)
{
    m_configuration = cm;
}

void
Transform::setConfigurationValue(QString name, QString value)
{
    std::cerr << "Transform::setConfigurationValue(" << name.toStdString()
              << ") -> " << value.toStdString() << std::endl;
    m_configuration[name] = value;
}

QString
Transform::getPluginVersion() const
{
    return m_pluginVersion;
}

void
Transform::setPluginVersion(QString version)
{
    m_pluginVersion = version;
}

QString
Transform::getProgram() const
{
    return m_program;
}

void
Transform::setProgram(QString program)
{
    m_program = program;
}

    
size_t
Transform::getStepSize() const
{
    return m_stepSize;
}

void
Transform::setStepSize(size_t s)
{
    m_stepSize = s;
}
    
size_t
Transform::getBlockSize() const
{
    return m_blockSize;
}

void
Transform::setBlockSize(size_t s)
{
    m_blockSize = s;
}

WindowType
Transform::getWindowType() const
{
    return m_windowType;
}

void
Transform::setWindowType(WindowType type)
{
    m_windowType = type;
}

RealTime
Transform::getStartTime() const
{
    return m_startTime;
}

void
Transform::setStartTime(RealTime t)
{
    m_startTime = t;
}

RealTime
Transform::getDuration() const
{
    return m_duration;
}

void
Transform::setDuration(RealTime d)
{
    m_duration = d;
}
    
float
Transform::getSampleRate() const
{
    return m_sampleRate;
}

void
Transform::setSampleRate(float rate)
{
    m_sampleRate = rate;
}

void
Transform::toXml(QTextStream &out, QString indent, QString extraAttributes) const
{
    out << indent;

    bool haveContent = true;
    if (m_parameters.empty() && m_configuration.empty()) haveContent = false;

    out << QString("<transform id=\"%1\" pluginVersion=\"%2\" program=\"%3\" stepSize=\"%4\" blockSize=\"%5\" windowType=\"%6\" startTime=\"%7\" duration=\"%8\" sampleRate=\"%9\"")
        .arg(encodeEntities(m_id))
        .arg(encodeEntities(m_pluginVersion))
        .arg(encodeEntities(m_program))
        .arg(m_stepSize)
        .arg(m_blockSize)
        .arg(encodeEntities(Window<float>::getNameForType(m_windowType).c_str()))
        .arg(encodeEntities(m_startTime.toString().c_str()))
        .arg(encodeEntities(m_duration.toString().c_str()))
        .arg(m_sampleRate);

    if (extraAttributes != "") {
        out << " " << extraAttributes;
    }

    if (haveContent) {

        out << ">\n";

        for (ParameterMap::const_iterator i = m_parameters.begin();
             i != m_parameters.end(); ++i) {
            out << indent << "  "
                << QString("<parameter name=\"%1\" value=\"%2\"/>\n")
                .arg(encodeEntities(i->first))
                .arg(i->second);
        }
        
        for (ConfigurationMap::const_iterator i = m_configuration.begin();
             i != m_configuration.end(); ++i) {
            out << indent << "  "
                << QString("<configuration name=\"%1\" value=\"%2\"/>\n")
                .arg(encodeEntities(i->first))
                .arg(encodeEntities(i->second));
        }

        out << indent << "</transform>\n";

    } else {

        out << "/>\n";
    }
}

void
Transform::setFromXmlAttributes(const QXmlAttributes &attrs)
{
    if (attrs.value("id") != "") {
        setIdentifier(attrs.value("id"));
    }

    if (attrs.value("pluginVersion") != "") {
        setPluginVersion(attrs.value("pluginVersion"));
    }

    if (attrs.value("program") != "") {
        setProgram(attrs.value("program"));
    }

    if (attrs.value("stepSize") != "") {
        setStepSize(attrs.value("stepSize").toInt());
    }

    if (attrs.value("blockSize") != "") {
        setBlockSize(attrs.value("blockSize").toInt());
    }

    if (attrs.value("windowType") != "") {
        setWindowType(Window<float>::getTypeForName
                      (attrs.value("windowType").toStdString()));
    }

    if (attrs.value("startTime") != "") {
        setStartTime(RealTime::fromString(attrs.value("startTime").toStdString()));
    }

    if (attrs.value("duration") != "") {
        setStartTime(RealTime::fromString(attrs.value("duration").toStdString()));
    }
    
    if (attrs.value("sampleRate") != "") {
        setSampleRate(attrs.value("sampleRate").toFloat());
    }
}

