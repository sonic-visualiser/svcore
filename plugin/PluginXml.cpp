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

#include "PluginXml.h"

#include <QRegExp>
#include <QXmlAttributes>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QDomAttr>

#include "vamp-sdk/PluginBase.h"

#include <iostream>

PluginXml::PluginXml(Vamp::PluginBase *plugin) :
    m_plugin(plugin)
{
}

PluginXml::~PluginXml() { }

QString
PluginXml::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    s += indent;

    s += QString("<plugin name=\"%1\" description=\"%2\" maker=\"%3\" version=\"%4\" copyright=\"%5\" %6 ")
        .arg(encodeEntities(QString(m_plugin->getName().c_str())))
        .arg(encodeEntities(QString(m_plugin->getDescription().c_str())))
        .arg(encodeEntities(QString(m_plugin->getMaker().c_str())))
        .arg(m_plugin->getPluginVersion())
        .arg(encodeEntities(QString(m_plugin->getCopyright().c_str())))
        .arg(extraAttributes);

    if (!m_plugin->getPrograms().empty()) {
        s += QString("program=\"%1\" ")
            .arg(encodeEntities(m_plugin->getCurrentProgram().c_str()));
    }

    Vamp::PluginBase::ParameterList parameters =
        m_plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        s += QString("param-%1=\"%2\" ")
            .arg(stripInvalidParameterNameCharacters(QString(i->name.c_str())))
            .arg(m_plugin->getParameter(i->name));
    }

    s += "/>\n";
    return s;
}

#define CHECK_ATTRIBUTE(ATTRIBUTE, ACCESSOR) \
    QString ATTRIBUTE = attrs.value(#ATTRIBUTE); \
    if (ATTRIBUTE != "" && ATTRIBUTE != ACCESSOR().c_str()) { \
        std::cerr << "WARNING: PluginXml::setParameters: Plugin " \
                  << #ATTRIBUTE << " does not match (attributes have \"" \
                  << ATTRIBUTE.toStdString() << "\", my " \
                  << #ATTRIBUTE << " is \"" << ACCESSOR() << "\")" << std::endl; \
    }

void
PluginXml::setParameters(const QXmlAttributes &attrs)
{
    CHECK_ATTRIBUTE(name, m_plugin->getName);
    CHECK_ATTRIBUTE(description, m_plugin->getDescription);
    CHECK_ATTRIBUTE(maker, m_plugin->getMaker);
    CHECK_ATTRIBUTE(copyright, m_plugin->getCopyright);

    bool ok;
    int version = attrs.value("version").trimmed().toInt(&ok);
    if (ok && version != m_plugin->getPluginVersion()) {
        std::cerr << "WARNING: PluginXml::setParameters: Plugin version does not match (attributes have " << version << ", my version is " << m_plugin->getPluginVersion() << ")" << std::endl;
    }

    if (!m_plugin->getPrograms().empty()) {
        m_plugin->selectProgram(attrs.value("program").toStdString());
    }

    Vamp::PluginBase::ParameterList parameters =
        m_plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i =
             parameters.begin(); i != parameters.end(); ++i) {

        QString name = QString("param-%1")
            .arg(stripInvalidParameterNameCharacters
                 (QString(i->name.c_str())));

        if (attrs.value(name) == "") {
            std::cerr << "PluginXml::setParameters: no parameter \"" << i->name << "\" (attribute \"" << name.toStdString() << "\")" << std::endl;
            continue;
        }

        bool ok;
        float value = attrs.value(name).trimmed().toFloat(&ok);
        if (ok) {
            m_plugin->setParameter(i->name, value);
        } else {
            std::cerr << "WARNING: PluginXml::setParameters: Invalid value \"" << attrs.value(name).toStdString() << "\" for parameter \"" << i->name << "\" (attribute \"" << name.toStdString() << "\")" << std::endl;
        }
    }
}

void
PluginXml::setParametersFromXml(QString xml)
{
    QDomDocument doc;

    QString error;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(xml, false, &error, &errorLine, &errorColumn)) {
        std::cerr << "PluginXml::setParametersFromXml: Error in parsing XML: " << error.toStdString() << " at line " << errorLine << ", column " << errorColumn << std::endl;
        std::cerr << "Input follows:" << std::endl;
        std::cerr << xml.toStdString() << std::endl;
        std::cerr << "Input ends." << std::endl;
        return;
    }

    QDomElement pluginElt = doc.firstChildElement("plugin");
    QDomNamedNodeMap attrNodes = pluginElt.attributes();
    QXmlAttributes attrs;

    for (unsigned int i = 0; i < attrNodes.length(); ++i) {
        QDomAttr attr = attrNodes.item(i).toAttr();
        if (attr.isNull()) continue;
        std::cerr << "Adding attribute \"" << attr.name().toStdString()
                  << "\" with value \"" << attr.value().toStdString() << "\"" << std::endl;
        attrs.append(attr.name(), "", "", attr.value());
    }

    setParameters(attrs);
}
    
QString
PluginXml::stripInvalidParameterNameCharacters(QString s) const
{
    s.replace(QRegExp("[^a-zA-Z0-9_]*"), "");
    return s;
}

