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

#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include <QString>
#include <QMutex>

#include <map>

class ConfigFile
{
public:
    ConfigFile(QString filename);
    virtual ~ConfigFile();

    /**
     * Get a value, with a default if it hasn't been set.
     */
    QString get(QString key, QString deft = "");

    bool getBool(QString key, bool deft);

    int getInt(QString key, int deft);
    
    float getFloat(QString key, float deft);

    QStringList getStringList(QString key);

    /**
     * Set a value.  Values must not contain carriage return or other
     * non-printable characters.  Keys must contain [a-zA-Z0-9_-] only.
     */
    void set(QString key, QString value);

    void set(QString key, bool value);

    void set(QString key, int value);
    
    void set(QString key, float value);
    
    void set(QString key, const QStringList &values); // must not contain '|'

    /**
     * Write the data to file.  May throw FileOperationFailed.
     *
     * This is called automatically on destruction if any data has
     * changed since it was last called.  At that time, any exception
     * will be ignored.  If you want to ensure that exceptions are
     * handled, call it yourself before destruction.
     */
    void commit();

    /**
     * Return to the stored values.  You can also call this before
     * destruction if you want to ensure that any values modified so
     * far are not written out to file on destruction.
     */
    void reset();

protected:
    bool load();

    QString m_filename;

    typedef std::map<QString, QString> DataMap;
    DataMap m_data;

    bool m_loaded;
    bool m_modified;

    QMutex m_mutex;
};

#endif

