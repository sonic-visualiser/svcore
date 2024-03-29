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

#include <QRegularExpression>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QDomAttr>

#include <QTextStream>

#include <vamp-hostsdk/PluginBase.h>
#include "RealTimePluginInstance.h"

#include <iostream>

namespace sv {

using std::shared_ptr;
using std::dynamic_pointer_cast;

PluginXml::PluginXml(shared_ptr<Vamp::PluginBase> plugin) :
    m_plugin(plugin)
{
}

PluginXml::~PluginXml() { }

QString
PluginXml::encodeConfigurationChars(QString text)
{
    QString rv(text);
    rv.replace(";", "[[SEMICOLON]]");
    rv.replace("=", "[[EQUALS]]");
    return rv;
}

QString
PluginXml::decodeConfigurationChars(QString text)
{
    QString rv(text);
    rv.replace("[[SEMICOLON]]", ";");
    rv.replace("[[EQUALS]]", "=");
    return rv;
}
    
void
PluginXml::toXml(QTextStream &stream,
                 QString indent, QString extraAttributes) const
{
    stream << indent;

    stream << QString("<plugin identifier=\"%1\" name=\"%2\" description=\"%3\" maker=\"%4\" version=\"%5\" copyright=\"%6\" %7 ")
        .arg(encodeEntities(QString(m_plugin->getIdentifier().c_str())))
        .arg(encodeEntities(QString(m_plugin->getName().c_str())))
        .arg(encodeEntities(QString(m_plugin->getDescription().c_str())))
        .arg(encodeEntities(QString(m_plugin->getMaker().c_str())))
        .arg(m_plugin->getPluginVersion())
        .arg(encodeEntities(QString(m_plugin->getCopyright().c_str())))
        .arg(extraAttributes);

    if (!m_plugin->getPrograms().empty()) {
        stream << QString("program=\"%1\" ")
            .arg(encodeEntities(m_plugin->getCurrentProgram().c_str()));
    }

    Vamp::PluginBase::ParameterList parameters =
        m_plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {

//        SVDEBUG << "PluginXml::toXml: parameter name \""
//                  << i->name.c_str() << "\" has value "
//                  << m_plugin->getParameter(i->name) << endl;

        stream << QString("param-%1=\"%2\" ")
            .arg(stripInvalidParameterNameCharacters(QString(i->identifier.c_str())))
            .arg(m_plugin->getParameter(i->identifier));
    }

    auto rtpi = dynamic_pointer_cast<RealTimePluginInstance>(m_plugin);
    if (rtpi) {
        std::map<std::string, std::string> configurePairs =
            rtpi->getConfigurePairs();
        QString config;
        for (std::map<std::string, std::string>::iterator i = configurePairs.begin();
             i != configurePairs.end(); ++i) {
            QString key = i->first.c_str();
            QString value = i->second.c_str();
            key = encodeConfigurationChars(key);
            value = encodeConfigurationChars(value);
            if (config != "") config += ";";
            config += QString("%1=%2").arg(key).arg(value);
        }
        if (config != "") {
            stream << QString("configuration=\"%1\" ")
                .arg(encodeEntities(config));
        }
    }

    stream << "/>\n";
}

#define CHECK_ATTRIBUTE(ATTRIBUTE, ACCESSOR) \
    QString ATTRIBUTE = attrs.value(#ATTRIBUTE); \
    if (ATTRIBUTE != "" && ATTRIBUTE != ACCESSOR().c_str()) { \
        SVCERR << "WARNING: PluginXml::setParameters: Plugin " \
                  << #ATTRIBUTE << " does not match (attributes have \"" \
                  << ATTRIBUTE << "\", my " \
                  << #ATTRIBUTE << " is \"" << ACCESSOR() << "\")" << endl; \
    }

void
PluginXml::setParameters(const Attributes &attrs)
{
    CHECK_ATTRIBUTE(identifier, m_plugin->getIdentifier);
    CHECK_ATTRIBUTE(name, m_plugin->getName);
    CHECK_ATTRIBUTE(description, m_plugin->getDescription);
    CHECK_ATTRIBUTE(maker, m_plugin->getMaker);
    CHECK_ATTRIBUTE(copyright, m_plugin->getCopyright);

    bool ok;
    int version = attrs.value("version").trimmed().toInt(&ok);
    if (ok && version != m_plugin->getPluginVersion()) {
        SVCERR << "WARNING: PluginXml::setParameters: Plugin version does not match (attributes have " << version << ", my version is " << m_plugin->getPluginVersion() << ")" << endl;
    }

    auto rtpi = dynamic_pointer_cast<RealTimePluginInstance>(m_plugin);
    if (rtpi) {
        QString config = attrs.value("configuration");
        if (config != "") {
            QStringList configList = config.split(";");
            for (QStringList::iterator i = configList.begin();
                 i != configList.end(); ++i) {
                QStringList kv = i->split("=");
                if (kv.count() < 2) {
                    SVCERR << "WARNING: PluginXml::setParameters: Malformed configure pair string: \"" << *i << "\"" << endl;
                    continue;
                }
                QString key(kv[0]), value(kv[1]);
                key = decodeConfigurationChars(key);
                value = decodeConfigurationChars(value);
                rtpi->configure(key.toStdString(), value.toStdString());
            }
        }
    }

    if (!m_plugin->getPrograms().empty()) {
        m_plugin->selectProgram(attrs.value("program").toStdString());
    }

    Vamp::PluginBase::ParameterList parameters =
        m_plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i =
             parameters.begin(); i != parameters.end(); ++i) {

        QString pname = QString("param-%1")
            .arg(stripInvalidParameterNameCharacters
                 (QString(i->identifier.c_str())));

        if (attrs.value(pname) == "") {
//            SVDEBUG << "PluginXml::setParameters: no parameter \"" << i->name << "\" (attribute \"" << name << "\")" << endl;
            continue;
        }

        float value = attrs.value(pname).trimmed().toFloat(&ok);
        if (ok) {
//            SVDEBUG << "PluginXml::setParameters: setting parameter \""
//                      << i->identifier << "\" to value " << value << endl;
            m_plugin->setParameter(i->identifier, value);
        } else {
            SVCERR << "WARNING: PluginXml::setParameters: Invalid value \"" << attrs.value(pname) << "\" for parameter \"" << i->identifier << "\" (attribute \"" << pname << "\")" << endl;
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

//    SVDEBUG << "PluginXml::setParametersFromXml: XML is \""
//              << xml << "\"" << endl;

    if (!doc.setContent(xml, false, &error, &errorLine, &errorColumn)) {
        SVCERR << "PluginXml::setParametersFromXml: Error in parsing XML: " << error << " at line " << errorLine << ", column " << errorColumn << endl;
        SVCERR << "Input follows:" << endl;
        SVCERR << xml << endl;
        SVCERR << "Input ends." << endl;
        return;
    }

    QDomElement pluginElt = doc.firstChildElement("plugin");
    QDomNamedNodeMap attrNodes = pluginElt.attributes();
    Attributes attrs;

    for (int i = 0; i < attrNodes.length(); ++i) {
        QDomAttr attr = attrNodes.item(i).toAttr();
        if (attr.isNull()) continue;
//        SVDEBUG << "PluginXml::setParametersFromXml: Adding attribute \"" << attr.name()//                  << "\" with value \"" << attr.value() << "\"" << endl;
        attrs[attr.name()] = attr.value();
    }

    setParameters(attrs);
}

QString
PluginXml::stripInvalidParameterNameCharacters(QString s) const
{
    s.replace(QRegularExpression("[^a-zA-Z0-9_]*"), "");
    return s;
}

} // end namespace sv

