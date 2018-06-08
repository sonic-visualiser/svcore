/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginPathSetter.h"

#include <vamp-hostsdk/PluginHostAdapter.h>

#include "RealTimePluginFactory.h"
#include "LADSPAPluginFactory.h"
#include "DSSIPluginFactory.h"

#include <QSettings>
#include <QMutexLocker>

#include "system/System.h"

QMutex
PluginPathSetter::m_mutex;

PluginPathSetter::Paths
PluginPathSetter::m_defaultPaths;

PluginPathSetter::Paths
PluginPathSetter::m_environmentPaths;

std::map<QString, QString>
PluginPathSetter::m_originalEnvValues;

using std::string;

PluginPathSetter::Paths
PluginPathSetter::getEnvironmentPathsUncached()
{
    Paths paths;

    auto vampPath = Vamp::PluginHostAdapter::getPluginPath();

    QStringList qVampPath;
    for (auto s: vampPath) {
        qVampPath.push_back(QString::fromStdString(s));
    }
    paths["Vamp"] = { qVampPath, "VAMP_PATH", true };

    auto dssiPath = DSSIPluginFactory::getPluginPath();

    QStringList qDssiPath;
    for (auto s: dssiPath) {
        qDssiPath.push_back(s);
    }
    paths["DSSI"] = { qDssiPath, "DSSI_PATH", true };
            
    auto ladspaPath = LADSPAPluginFactory::getPluginPath();

    QStringList qLadspaPath;
    for (auto s: ladspaPath) {
        qLadspaPath.push_back(s);
    }
    paths["LADSPA"] = { qLadspaPath, "LADSPA_PATH", true };

    return paths;
}

PluginPathSetter::Paths
PluginPathSetter::getDefaultPaths()
{
    QMutexLocker locker(&m_mutex);

    if (!m_defaultPaths.empty()) {
        return m_defaultPaths;
    }

    string savedPathVamp, savedPathDssi, savedPathLadspa;
    (void)getEnvUtf8("VAMP_PATH", savedPathVamp);
    (void)getEnvUtf8("DSSI_PATH", savedPathDssi);
    (void)getEnvUtf8("LADSPA_PATH", savedPathLadspa);

    putEnvUtf8("VAMP_PATH", "");
    putEnvUtf8("DSSI_PATH", "");
    putEnvUtf8("LADSPA_PATH", "");

    Paths paths = getEnvironmentPathsUncached();

    putEnvUtf8("VAMP_PATH", savedPathVamp);
    putEnvUtf8("DSSI_PATH", savedPathDssi);
    putEnvUtf8("LADSPA_PATH", savedPathLadspa);

    m_defaultPaths = paths;
    return m_defaultPaths;
}

PluginPathSetter::Paths
PluginPathSetter::getEnvironmentPaths()
{
    QMutexLocker locker(&m_mutex);

    if (!m_environmentPaths.empty()) {
        return m_environmentPaths;
    }
        
    m_environmentPaths = getEnvironmentPathsUncached();
    return m_environmentPaths;
}

PluginPathSetter::Paths
PluginPathSetter::getPaths()
{
    Paths paths = getEnvironmentPaths();
       
    QSettings settings;
    settings.beginGroup("Plugins");

    for (auto p: paths) {

        QString tag = p.first;

        QStringList directories =
            settings.value(QString("directories-%1").arg(tag),
                           p.second.directories)
            .toStringList();
        QString envVariable =
            settings.value(QString("env-variable-%1").arg(tag),
                           p.second.envVariable)
            .toString();
        bool useEnvVariable =
            settings.value(QString("use-env-variable-%1").arg(tag),
                           p.second.useEnvVariable)
            .toBool();

        string envVarStr = envVariable.toStdString();
        string currentValue;
        (void)getEnvUtf8(envVarStr, currentValue);

        if (currentValue != "" && useEnvVariable) {
            directories = QString::fromStdString(currentValue).split(
#ifdef Q_OS_WIN
               ";"
#else
               ":"
#endif
                );
        }
        
        paths[tag] = { directories, envVariable, useEnvVariable };
    }

    settings.endGroup();

    return paths;
}

void
PluginPathSetter::savePathSettings(Paths paths)
{
    QSettings settings;
    settings.beginGroup("Plugins");

    for (auto p: paths) {
        QString tag = p.first;
        settings.setValue(QString("directories-%1").arg(tag),
                          p.second.directories);
        settings.setValue(QString("env-variable-%1").arg(tag),
                          p.second.envVariable);
        settings.setValue(QString("use-env-variable-%1").arg(tag),
                          p.second.useEnvVariable);
    }

    settings.endGroup();
}

QString
PluginPathSetter::getOriginalEnvironmentValue(QString envVariable)
{
    if (m_originalEnvValues.find(envVariable) != m_originalEnvValues.end()) {
        return m_originalEnvValues.at(envVariable);
    } else {
        return QString();
    }
}

void
PluginPathSetter::initialiseEnvironmentVariables()
{
    // Set the relevant environment variables from user configuration,
    // so that later lookups through the standard APIs will follow the
    // same paths as we have in the user config

    // First ensure the default paths have been recorded for later, so
    // we don't erroneously re-read them from the environment
    // variables we've just set
    (void)getDefaultPaths();
    (void)getEnvironmentPaths();
    
    Paths paths = getPaths();

    for (auto p: paths) {
        QString envVariable = p.second.envVariable;
        string envVarStr = envVariable.toStdString();
        QString currentValue = qEnvironmentVariable(envVarStr.c_str());
        m_originalEnvValues[envVariable] = currentValue;
        if (currentValue != QString() && p.second.useEnvVariable) {
            // don't override
            continue;
        }
        QString separator =
#ifdef Q_OS_WIN
            ";"
#else
            ":"
#endif
            ;
        QString proposedValue = p.second.directories.join(separator);
        putEnvUtf8(envVarStr, proposedValue.toStdString());
    }
}

