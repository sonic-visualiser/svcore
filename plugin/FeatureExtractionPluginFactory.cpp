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

#include "FeatureExtractionPluginFactory.h"
#include "PluginIdentifier.h"

#include "vamp/vamp.h"
#include "vamp-sdk/PluginHostAdapter.h"

#include "system/System.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <iostream>

static FeatureExtractionPluginFactory *_nativeInstance = 0;

FeatureExtractionPluginFactory *
FeatureExtractionPluginFactory::instance(QString pluginType)
{
    if (pluginType == "vamp") {
	if (!_nativeInstance) {
	    std::cerr << "FeatureExtractionPluginFactory::instance(" << pluginType.toStdString()
		      << "): creating new FeatureExtractionPluginFactory" << std::endl;
	    _nativeInstance = new FeatureExtractionPluginFactory();
	}
	return _nativeInstance;
    }

    else return 0;
}

FeatureExtractionPluginFactory *
FeatureExtractionPluginFactory::instanceFor(QString identifier)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    return instance(type);
}

std::vector<QString>
FeatureExtractionPluginFactory::getPluginPath()
{
    if (!m_pluginPath.empty()) return m_pluginPath;

    std::vector<QString> path;
    std::string envPath;

    char *cpath = getenv("VAMP_PATH");
    if (cpath) envPath = cpath;

    if (envPath == "") {
        envPath = DEFAULT_VAMP_PATH;
        char *chome = getenv("HOME");
        if (chome) {
            std::string home(chome);
            int f;
            while ((f = envPath.find("$HOME")) >= 0 && f < envPath.length()) {
                envPath.replace(f, 5, home);
            }
        }
#ifdef Q_WS_WIN32
        char *cpfiles = getenv("ProgramFiles");
        if (!cpfiles) cpfiles = "C:\\Program Files";
        std::string pfiles(cpfiles);
        int f;
        while ((f = envPath.find("%ProgramFiles%")) >= 0 && f < envPath.length()) {
            envPath.replace(f, 14, pfiles);
        }
#endif
    }

    std::cerr << "VAMP path is: \"" << envPath << "\"" << std::endl;

    std::string::size_type index = 0, newindex = 0;

    while ((newindex = envPath.find(PATH_SEPARATOR, index)) < envPath.size()) {
	path.push_back(envPath.substr(index, newindex - index).c_str());
	index = newindex + 1;
    }
    
    path.push_back(envPath.substr(index).c_str());

    m_pluginPath = path;
    return path;
}

std::vector<QString>
FeatureExtractionPluginFactory::getAllPluginIdentifiers()
{
    FeatureExtractionPluginFactory *factory;
    std::vector<QString> rv;
    
    factory = instance("vamp");
    if (factory) {
	std::vector<QString> tmp = factory->getPluginIdentifiers();
	for (size_t i = 0; i < tmp.size(); ++i) {
	    rv.push_back(tmp[i]);
	}
    }

    // Plugins can change the locale, revert it to default.
    setlocale(LC_ALL, "C");
    return rv;
}

std::vector<QString>
FeatureExtractionPluginFactory::getPluginIdentifiers()
{
    std::vector<QString> rv;
    std::vector<QString> path = getPluginPath();
    
    for (std::vector<QString>::iterator i = path.begin(); i != path.end(); ++i) {

//        std::cerr << "FeatureExtractionPluginFactory::getPluginIdentifiers: scanning directory " << i->toStdString() << std::endl;

	QDir pluginDir(*i, PLUGIN_GLOB,
                       QDir::Name | QDir::IgnoreCase,
                       QDir::Files | QDir::Readable);

	for (unsigned int j = 0; j < pluginDir.count(); ++j) {

            QString soname = pluginDir.filePath(pluginDir[j]);

            void *libraryHandle = DLOPEN(soname, RTLD_LAZY);
            
            if (!libraryHandle) {
                std::cerr << "WARNING: FeatureExtractionPluginFactory::getPluginIdentifiers: Failed to load library " << soname.toStdString() << ": " << DLERROR() << std::endl;
                continue;
            }

            VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
                DLSYM(libraryHandle, "vampGetPluginDescriptor");

            if (!fn) {
                std::cerr << "WARNING: FeatureExtractionPluginFactory::getPluginIdentifiers: No descriptor function in " << soname.toStdString() << std::endl;
                if (DLCLOSE(libraryHandle) != 0) {
                    std::cerr << "WARNING: FeatureExtractionPluginFactory::getPluginIdentifiers: Failed to unload library " << soname.toStdString() << std::endl;
                }
                continue;
            }

            const VampPluginDescriptor *descriptor = 0;
            int index = 0;

            while ((descriptor = fn(index))) {
                QString id = PluginIdentifier::createIdentifier
                    ("vamp", soname, descriptor->name);
                rv.push_back(id);
                std::cerr << "Found id " << id.toStdString() << std::endl;
                ++index;
            }
            
            if (DLCLOSE(libraryHandle) != 0) {
                std::cerr << "WARNING: FeatureExtractionPluginFactory::getPluginIdentifiers: Failed to unload library " << soname.toStdString() << std::endl;
            }
	}
    }

    return rv;
}

QString
FeatureExtractionPluginFactory::findPluginFile(QString soname, QString inDir)
{
    QString file = "";

    if (inDir != "") {

        QDir dir(inDir, PLUGIN_GLOB,
                 QDir::Name | QDir::IgnoreCase,
                 QDir::Files | QDir::Readable);
        if (!dir.exists()) return "";

        file = dir.filePath(QFileInfo(soname).fileName());
        if (QFileInfo(file).exists()) {
            return file;
        }

	for (unsigned int j = 0; j < dir.count(); ++j) {
            file = dir.filePath(dir[j]);
            if (QFileInfo(file).baseName() == QFileInfo(soname).baseName()) {
                return file;
            }
        }

        return "";

    } else {

        QFileInfo fi(soname);
        if (fi.exists()) return soname;

        if (fi.isAbsolute() && fi.absolutePath() != "") {
            file = findPluginFile(soname, fi.absolutePath());
            if (file != "") return file;
        }

        std::vector<QString> path = getPluginPath();
        for (std::vector<QString>::iterator i = path.begin();
             i != path.end(); ++i) {
            if (*i != "") {
                file = findPluginFile(soname, *i);
                if (file != "") return file;
            }
        }

        return "";
    }
}

Vamp::Plugin *
FeatureExtractionPluginFactory::instantiatePlugin(QString identifier,
						  float inputSampleRate)
{
    Vamp::Plugin *rv = 0;

    const VampPluginDescriptor *descriptor = 0;
    int index = 0;

    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);
    if (type != "vamp") {
	std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Wrong factory for plugin type " << type.toStdString() << std::endl;
	return 0;
    }

    QString found = findPluginFile(soname);

    if (found == "") {
        std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Failed to find library file " << soname.toStdString() << std::endl;
        return 0;
    } else if (found != soname) {
//        std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: WARNING: Given library name was " << soname.toStdString() << ", found at " << found.toStdString() << std::endl;
//        std::cerr << soname.toStdString() << " -> " << found.toStdString() << std::endl;
    }

    soname = found;

    void *libraryHandle = DLOPEN(soname, RTLD_LAZY);
            
    if (!libraryHandle) {
        std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Failed to load library " << soname.toStdString() << ": " << DLERROR() << std::endl;
        return 0;
    }

    VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
        DLSYM(libraryHandle, "vampGetPluginDescriptor");
    
    if (!fn) {
        std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: No descriptor function in " << soname.toStdString() << std::endl;
        goto done;
    }

    while ((descriptor = fn(index))) {
        if (label == descriptor->name) break;
        ++index;
    }

    if (!descriptor) {
        std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Failed to find plugin \"" << label.toStdString() << "\" in library " << soname.toStdString() << std::endl;
        goto done;
    }

    rv = new Vamp::PluginHostAdapter(descriptor, inputSampleRate);

//    std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Constructed Vamp plugin, rv is " << rv << std::endl;

    //!!! need to dlclose() when plugins from a given library are unloaded

done:
    if (!rv) {
        if (DLCLOSE(libraryHandle) != 0) {
            std::cerr << "WARNING: FeatureExtractionPluginFactory::instantiatePlugin: Failed to unload library " << soname.toStdString() << std::endl;
        }
    }

//    std::cerr << "FeatureExtractionPluginFactory::instantiatePlugin: Instantiated plugin " << label.toStdString() << " from library " << soname.toStdString() << ": descriptor " << descriptor << ", rv "<< rv << ", label " << rv->getName() << ", outputs " << rv->getOutputDescriptors().size() << std::endl;
    
    return rv;
}

