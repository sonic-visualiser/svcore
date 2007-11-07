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

Transform::Transform() :
    m_stepSize(0),
    m_blockSize(0),
    m_windowType(HanningWindow),
    m_sampleRate(0)
{
}

Transform::~Transform()
{
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
Transform::toXml(QTextStream &stream, QString indent, QString extraAttributes) const
{
    
}
