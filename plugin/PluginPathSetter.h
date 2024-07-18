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

#include "checker/knownplugins.h"

namespace sv {

class PluginPathSetter
{
public:
    typedef std::pair<KnownPlugins::PluginType,
                      KnownPlugins::BinaryFormat> TypeKey;

    typedef std::vector<TypeKey> TypeKeys;

    struct PathConfig {
        /** List of directories which may arise from user settings,
         *  environment variables, and defaults as appropriate
         */
        QStringList directories;

        /** Name of environment variable responsible, e.g. LADSPA_PATH
         */
        QString envVariable;

        /** True if the environment variable should override any user
         *  settings for this type. If so, and if the variable is set,
         *  it will be preferentially used by getPaths and its
         *  contents will not be replaced by
         *  initialiseEnvironmentVariables.
         */
        bool useEnvVariable;
    };

    typedef std::map<TypeKey, PathConfig> Paths;

    /** Update *_PATH environment variables based on the settings, so
     *  that subsequent lookups through APIs unaware of this class
     *  will follow the same paths as we have in the user config. This
     *  should be called once on startup, after the QApplication has
     *  been initialised so application settings are available.
     */
    static void initialiseEnvironmentVariables();

    /**
     *  Return default values of paths only, without taking any
     *  environment variables or user-defined preferences into
     *  account.
     */
    static Paths getDefaultPaths();

    /**
     *  Return paths arising from environment variables only, falling
     *  back to the default as reported by getDefaultPaths in cases
     *  where the variables are unset but without taking any
     *  user-defined preferences into account. Note that calling
     *  initialiseEnvironmentVariables does not affect the value
     *  returned here - the original contents of the variables are
     *  saved and reported here regardless.
     */
    static Paths getEnvironmentPaths();

    /**
     *  Return paths arising from the combination of user settings,
     *  environment variables, and defaults as appropriate. These are
     *  the paths the user should be expecting to be searched.
     */
    static Paths getPaths();

    /**
     *  Save the given paths to the application settings.
     */
    static void savePathSettings(Paths paths);

    /**
     *  Return the original value observed on startup for the given
     *  environment variable, if it is one of the variables used by a
     *  known path config. This can only be reported if
     *  initialiseEnvironmentVariables has been called (as the other
     *  APIs delegate environment variable handling to the plugin
     *  checker layer).
     */
    static QString getOriginalEnvironmentValue(QString envVariable);
    
private:
    static Paths m_defaultPaths;
    static Paths m_environmentPaths;
    static std::map<QString, QString> m_originalEnvValues;
    static TypeKeys m_supportedKeys;
    static QMutex m_mutex;

    static std::vector<TypeKey> getSupportedKeys();
    static Paths getEnvironmentPathsUncached(const TypeKeys &keys);
    static QString getSettingTagFor(TypeKey);
};

} // end namespace sv

#endif
