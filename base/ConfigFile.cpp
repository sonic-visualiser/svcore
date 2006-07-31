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

#include "ConfigFile.h"

#include "Exceptions.h"

#include <iostream>

#include <QFile>
#include <QMutexLocker>
#include <QTextStream>
#include <QStringList>

ConfigFile::ConfigFile(QString filename) :
    m_filename(filename),
    m_loaded(false),
    m_modified(false)
{
}

ConfigFile::~ConfigFile()
{
    try {
        commit();
    } catch (FileOperationFailed f) {
        std::cerr << "WARNING: ConfigFile::~ConfigFile: Commit failed for "
                  << m_filename.toStdString() << std::endl;
    }
}

QString
ConfigFile::get(QString key, QString deft)
{
    if (!m_loaded) load();
        
    QMutexLocker locker(&m_mutex);

    if (m_data.find(key) == m_data.end()) return deft;
    return m_data[key];
}

int
ConfigFile::getInt(QString key, int deft)
{
    return get(key, QString("%1").arg(deft)).toInt();
}

bool
ConfigFile::getBool(QString key, bool deft)
{
    QString value = get(key, deft ? "true" : "false").trimmed().toLower();
    return (value == "true" || value == "yes" || value == "on" || value == "1");
}
 
float
ConfigFile::getFloat(QString key, float deft)
{
    return get(key, QString("%1").arg(deft)).toFloat();
}

QStringList
ConfigFile::getStringList(QString key)
{
    return get(key).split('|');
}

void
ConfigFile::set(QString key, QString value)
{
    if (!m_loaded) load();
        
    QMutexLocker locker(&m_mutex);

    m_data[key] = value;

    m_modified = true;
}

void
ConfigFile::set(QString key, int value)
{
    set(key, QString("%1").arg(value));
}

void
ConfigFile::set(QString key, bool value)
{
    set(key, value ? QString("true") : QString("false"));
}

void
ConfigFile::set(QString key, float value)
{
    set(key, QString("%1").arg(value));
}

void
ConfigFile::set(QString key, const QStringList &values)
{
    set(key, values.join("|"));
}

void
ConfigFile::commit()
{
    QMutexLocker locker(&m_mutex);

    if (!m_modified) return;

    // Really we should write to another file and then move to the
    // intended target, but I don't think we're all that particular
    // about reliability here at the moment

    QFile file(m_filename);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        throw FileOperationFailed(m_filename, "open for writing");
    }
    
    QTextStream out(&file);

    for (DataMap::const_iterator i = m_data.begin(); i != m_data.end(); ++i) {
        out << i->first << "=" << i->second << endl;
    }

    m_modified = false;
}

bool
ConfigFile::load()
{
    QMutexLocker locker(&m_mutex);

    if (m_loaded) return true;

    QFile file(m_filename);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    QTextStream in(&file);

    m_data.clear();

    while (!in.atEnd()) {
        
        QString line = in.readLine(2048);
        QString key = line.section('=', 0, 0);
        QString value = line.section('=', 1, -1);
        if (key == "") continue;

        m_data[key] = value;
    }
    
    m_loaded = true;
    m_modified = false;
    return true;
}

void
ConfigFile::reset()
{
    QMutexLocker locker(&m_mutex);
    m_loaded = false;
    m_modified = false;
}

