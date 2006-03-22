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

#include "PluginInstance.h"

#include <QRegExp>
#include <QXmlAttributes>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QDomAttr>

#include <iostream>

QString
PluginInstance::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    s += indent;

    s += QString("<plugin name=\"%1\" description=\"%2\" maker=\"%3\" version=\"%4\" copyright=\"%5\" %6 ")
        .arg(encodeEntities(QString(getName().c_str())))
        .arg(encodeEntities(QString(getDescription().c_str())))
        .arg(encodeEntities(QString(getMaker().c_str())))
        .arg(getPluginVersion())
        .arg(encodeEntities(QString(getCopyright().c_str())))
        .arg(extraAttributes);

    if (!getPrograms().empty()) {
        s += QString("program=\"%1\" ")
            .arg(encodeEntities(getCurrentProgram().c_str()));
    }

    ParameterList parameters = getParameterDescriptors();

    for (ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        s += QString("param-%1=\"%2\" ")
            .arg(stripInvalidParameterNameCharacters(QString(i->name.c_str())))
            .arg(getParameter(i->name));
    }

    s += "/>\n";
    return s;
}

#define CHECK_ATTRIBUTE(ATTRIBUTE, ACCESSOR) \
    QString ATTRIBUTE = attrs.value(#ATTRIBUTE); \
    if (ATTRIBUTE != "" && ATTRIBUTE != ACCESSOR().c_str()) { \
        std::cerr << "WARNING: PluginInstance::setParameters: Plugin " \
                  << #ATTRIBUTE << " does not match (attributes have \"" \
                  << ATTRIBUTE.toStdString() << "\", my " \
                  << #ATTRIBUTE << " is \"" << ACCESSOR() << "\")" << std::endl; \
    }

void
PluginInstance::setParameters(const QXmlAttributes &attrs)
{
    CHECK_ATTRIBUTE(name, getName);
    CHECK_ATTRIBUTE(description, getDescription);
    CHECK_ATTRIBUTE(maker, getMaker);
    CHECK_ATTRIBUTE(copyright, getCopyright);

    bool ok;
    int version = attrs.value("version").trimmed().toInt(&ok);
    if (ok && version != getPluginVersion()) {
        std::cerr << "WARNING: PluginInstance::setParameters: Plugin version does not match (attributes have " << version << ", my version is " << getPluginVersion() << ")" << std::endl;
    }

    if (!getPrograms().empty()) {
        selectProgram(attrs.value("program").toStdString());
    }

    ParameterList parameters = getParameterDescriptors();

    for (ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        QString name = QString("param-%1")
            .arg(stripInvalidParameterNameCharacters
                 (QString(i->name.c_str())));
        bool ok;
        float value = attrs.value(name).trimmed().toFloat(&ok);
        if (ok) {
            setParameter(i->name, value);
        } else {
            std::cerr << "WARNING: PluginInstance::setParameters: Invalid value \"" << attrs.value(name).toStdString() << "\" for parameter \"" << i->name << "\" (attribute \"" << name.toStdString() << "\")" << std::endl;
        }
    }
}

void
PluginInstance::setParametersFromXml(QString xml)
{
    QDomDocument doc;

    QString error;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(xml, false, &error, &errorLine, &errorColumn)) {
        std::cerr << "PluginInstance::setParametersFromXml: Error in parsing XML: " << error.toStdString() << " at line " << errorLine << ", column " << errorColumn << std::endl;
        std::cerr << "Input follows:" << std::endl;
        std::cerr << xml.toStdString() << std::endl;
        std::cerr << "Input ends." << std::endl;
        return;
    }

    QDomElement pluginElt = doc.firstChildElement("plugin");

    if (pluginElt.isNull()) {
        std::cerr << "pluginElt is null" << std::endl;
        pluginElt = doc.documentElement();
        if (pluginElt.isNull()) {
            std::cerr << "pluginElt is still null" << std::endl;
        }
    }

    QDomNamedNodeMap attrNodes = pluginElt.attributes();
    QXmlAttributes attrs;

    for (int i = 0; i < attrNodes.length(); ++i) {
        QDomAttr attr = attrNodes.item(i).toAttr();
        if (attr.isNull()) continue;
        std::cerr << "Adding attribute \"" << attr.name().toStdString()
                  << "\" with value \"" << attr.value().toStdString() << "\"" << std::endl;
        attrs.append(attr.name(), "", "", attr.value());
    }

    setParameters(attrs);
}
    
QString
PluginInstance::stripInvalidParameterNameCharacters(QString s) const
{
    s.replace(QRegExp("[^a-zA-Z0-9_]*"), "");
    return s;
}

