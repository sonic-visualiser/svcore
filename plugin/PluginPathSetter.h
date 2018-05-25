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

#ifndef SV_PLUGIN_PATH_SETTER_H
#define SV_PLUGIN_PATH_SETTER_H

#include <QString>
#include <QStringList>
#include <QMutex>

#include <map>

class PluginPathSetter
{
public:
    /// Text used to identify a plugin type, e.g. "LADSPA", "Vamp"
    typedef QString PluginTypeLabel;

    struct PathConfig {
        QStringList directories;
        QString envVariable; // e.g. "LADSPA_PATH" etc
        bool useEnvVariable; // true if env variable overrides directories list
    };

    typedef std::map<PluginTypeLabel, PathConfig> Paths;

    /// Return paths arising from environment variables only, without
    /// any user-defined preferences
    static Paths getDefaultPaths();

    /// Return paths arising from user settings + environment
    /// variables as appropriate
    static Paths getPaths();

    /// Save the given paths to the settings
    static void savePathSettings(Paths paths);

    /// Update *_PATH environment variables from the settings, on
    /// application startup
    static void setEnvironmentVariables();

private:
    static Paths m_defaultPaths;
    static QMutex m_mutex;
};

#endif
