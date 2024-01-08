/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TransformFactory.h"

#include "plugin/FeatureExtractionPluginFactory.h"

#include "plugin/RealTimePluginFactory.h"
#include "plugin/RealTimePluginInstance.h"
#include "plugin/PluginXml.h"
#include "plugin/PluginScan.h"

#include <vamp-hostsdk/Plugin.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>

#include "rdf/PluginRDFIndexer.h"
#include "rdf/PluginRDFDescription.h"

#include "base/XmlExportable.h"

#include <iostream>
#include <set>
#include <functional>

#include <QRegularExpression>
#include <QTextStream>

#include "base/Thread.h"

//#define DEBUG_TRANSFORM_FACTORY 1

namespace sv {

TransformFactory *
TransformFactory::m_instance = new TransformFactory;

TransformFactory *
TransformFactory::getInstance()
{
    return m_instance;
}

void
TransformFactory::deleteInstance()
{
    SVDEBUG << "TransformFactory::deleteInstance called" << endl;
    delete m_instance;
    m_instance = nullptr;
}

TransformFactory::TransformFactory() :
    m_installedTransformsPopulated(false),
    m_uninstalledTransformsPopulated(false),
    m_installedThread(nullptr),
    m_uninstalledThread(nullptr),
    m_exiting(false),
    m_populatingSlowly(false)
{
}

TransformFactory::~TransformFactory()
{
    m_exiting = true;

    if (m_installedThread) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVDEBUG << "TransformFactory::~TransformFactory: waiting on installed transform thread" << endl;
#endif
        m_installedThread->wait();
        delete m_installedThread;
#ifdef DEBUG_TRANSFORM_FACTORY
        SVDEBUG << "TransformFactory::~TransformFactory: waited" << endl;
#endif
    }

    if (m_uninstalledThread) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVDEBUG << "TransformFactory::~TransformFactory: waiting on uninstalled transform thread" << endl;
#endif
        m_uninstalledThread->wait();
        delete m_uninstalledThread;
#ifdef DEBUG_TRANSFORM_FACTORY
        SVDEBUG << "TransformFactory::~TransformFactory: waited and done" << endl;
#endif
    }
}

void
TransformFactory::restrictTransformTypes(std::set<Transform::Type> types)
{
    m_transformTypeRestriction = types;
}

void
TransformFactory::startPopulatingInstalledTransforms()
{
    m_installedTransformsMutex.lock();

    if (m_installedThread) {
        m_installedTransformsMutex.unlock();
        return;
    }
    m_installedThread = new InstalledTransformsPopulateThread(this);

    m_installedTransformsMutex.unlock();

    m_installedThread->start();
}

void
TransformFactory::startPopulatingUninstalledTransforms()
{
    m_uninstalledTransformsMutex.lock();

    if (m_uninstalledThread) {
        m_uninstalledTransformsMutex.unlock();
        return;
    }
    m_uninstalledThread = new UninstalledTransformsPopulateThread(this);

    m_uninstalledTransformsMutex.unlock();

    m_uninstalledThread->start();
}

void
TransformFactory::InstalledTransformsPopulateThread::run()
{
    m_factory->populateInstalledTransforms();
}

void
TransformFactory::UninstalledTransformsPopulateThread::run()
{
    m_factory->m_populatingSlowly = true;
    while (!m_factory->havePopulatedInstalledTransforms() &&
           !m_factory->m_exiting) {
        sleep(1);
    }
    m_factory->populateUninstalledTransforms();
}

TransformList
TransformFactory::getInstalledTransformDescriptions()
{
    populateInstalledTransforms();

    std::set<TransformDescription> dset;
    for (auto i = m_transforms.begin(); i != m_transforms.end(); ++i) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVCERR << "inserting transform into set: id = " << i->second.identifier << " (" << i->second.name << ")" << endl;
#endif
        dset.insert(i->second);
    }

    TransformList list;
    for (auto i = dset.begin(); i != dset.end(); ++i) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVCERR << "inserting transform into list: id = " << i->identifier << " (" << i->name << ")" << endl;
#endif
        list.push_back(*i);
    }

    return list;
}

TransformDescription
TransformFactory::getInstalledTransformDescription(TransformId id)
{
    populateInstalledTransforms();

    if (m_transforms.find(id) == m_transforms.end()) {
        return TransformDescription();
    }

    return m_transforms[id];
}

bool
TransformFactory::haveAnyInstalledTransforms()
{
    populateInstalledTransforms();
    return !m_transforms.empty();
}

TransformList
TransformFactory::getUninstalledTransformDescriptions()
{
    m_populatingSlowly = false;
    populateUninstalledTransforms();
    
    std::set<TransformDescription> dset;
    for (auto i = m_uninstalledTransforms.begin();
         i != m_uninstalledTransforms.end(); ++i) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVCERR << "inserting transform into set: id = " << i->second.identifier << endl;
#endif
        dset.insert(i->second);
    }

    TransformList list;
    for (auto i = dset.begin(); i != dset.end(); ++i) {
#ifdef DEBUG_TRANSFORM_FACTORY
        SVCERR << "inserting transform into uninstalled list: id = " << i->identifier << endl;
#endif
        list.push_back(*i);
    }

    return list;
}

TransformDescription
TransformFactory::getUninstalledTransformDescription(TransformId id)
{
    m_populatingSlowly = false;
    populateUninstalledTransforms();

    if (m_uninstalledTransforms.find(id) == m_uninstalledTransforms.end()) {
        return TransformDescription();
    }

    return m_uninstalledTransforms[id];
}

bool
TransformFactory::haveAnyUninstalledTransforms(bool waitForCheckToComplete)
{
    if (waitForCheckToComplete) {
        populateUninstalledTransforms();
    } else {
        if (!m_uninstalledTransformsMutex.tryLock()) {
            return false;
        }
        if (!m_uninstalledTransformsPopulated) {
            m_uninstalledTransformsMutex.unlock();
            return false;
        }
        m_uninstalledTransformsMutex.unlock();
    }

    return !m_uninstalledTransforms.empty();
}

TransformFactory::TransformInstallStatus
TransformFactory::getTransformInstallStatus(TransformId id)
{
    populateInstalledTransforms();

    if (m_transforms.find(id) != m_transforms.end()) {
        return TransformInstalled;
    }
    
    if (!m_uninstalledTransformsMutex.tryLock()) {
        // uninstalled transforms are being populated; this may take some time,
        // and they aren't critical
        return TransformUnknown;
    }

    if (!m_uninstalledTransformsPopulated) {
        m_uninstalledTransformsMutex.unlock();
        m_populatingSlowly = false;
        populateUninstalledTransforms();
        m_uninstalledTransformsMutex.lock();
    }

    if (m_uninstalledTransforms.find(id) != m_uninstalledTransforms.end()) {
        m_uninstalledTransformsMutex.unlock();
        return TransformNotInstalled;
    }

    m_uninstalledTransformsMutex.unlock();
    return TransformUnknown;
}
    

std::vector<TransformDescription::Type>
TransformFactory::getTransformTypes()
{
    populateInstalledTransforms();

    std::set<TransformDescription::Type> types;
    for (auto i = m_transforms.begin(); i != m_transforms.end(); ++i) {
        types.insert(i->second.type);
    }

    std::vector<TransformDescription::Type> rv;
    for (auto i = types.begin(); i != types.end(); ++i) {
        rv.push_back(*i);
    }

    return rv;
}

std::vector<QString>
TransformFactory::getTransformCategories(TransformDescription::Type transformType)
{
    populateInstalledTransforms();

    std::set<QString, std::function<bool(QString, QString)>>
        categories(TransformDescription::compareUserStrings);
    
    for (auto i = m_transforms.begin(); i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            categories.insert(i->second.category);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (auto i = categories.begin(); i != categories.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

std::vector<QString>
TransformFactory::getTransformMakers(TransformDescription::Type transformType)
{
    populateInstalledTransforms();

    std::set<QString, std::function<bool(QString, QString)>>
        makers(TransformDescription::compareUserStrings);

    for (auto i = m_transforms.begin(); i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            makers.insert(i->second.maker);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (auto i = makers.begin(); i != makers.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

QString
TransformFactory::getTransformTypeName(TransformDescription::Type type) const
{
    switch (type) {
    case TransformDescription::Analysis: return tr("Analysis");
    case TransformDescription::Effects: return tr("Effects");
    case TransformDescription::EffectsData: return tr("Effects Data");
    case TransformDescription::Generator: return tr("Generator");
    case TransformDescription::UnknownType: return tr("Other");
    }
    return tr("Other");
}

bool
TransformFactory::havePopulatedInstalledTransforms()
{
    return m_installedTransformsPopulated;
}

bool
TransformFactory::havePopulatedUninstalledTransforms()
{
    return m_uninstalledTransformsPopulated;
}

void
TransformFactory::populateInstalledTransforms()
{
    {
        MutexLocker locker(&m_installedTransformsMutex,
                           "TransformFactory::populateInstalledTransforms");
        if (m_installedTransformsPopulated) {
            return;
        }

        PluginScan::getInstance()->scan();
        
        TransformDescriptionMap transforms;

        bool wantFeatureExtraction = false;
        bool wantRealTime = false;

        if (m_transformTypeRestriction.empty()) {
            wantFeatureExtraction = true;
            wantRealTime = true;
        } else {
            if (m_transformTypeRestriction.find(Transform::FeatureExtraction) !=
                m_transformTypeRestriction.end()) {
                wantFeatureExtraction = true;
            }
            if (m_transformTypeRestriction.find(Transform::RealTimeEffect) !=
                m_transformTypeRestriction.end()) {
                wantRealTime = true;
            }
        }

        if (wantFeatureExtraction) {
            populateFeatureExtractionPlugins(transforms);
            if (m_exiting) return;
        }

        if (wantRealTime) {
            populateRealTimePlugins(transforms);
            if (m_exiting) return;
        }

        // disambiguate plugins with similar names

        std::map<QString, int> names;
        std::map<QString, QString> pluginSources;
        std::map<QString, QString> pluginMakers;

        for (TransformDescriptionMap::iterator i = transforms.begin();
             i != transforms.end(); ++i) {

            TransformDescription desc = i->second;

            QString td = desc.name;
            QString tn = td.section(": ", 0, 0);
            QString pn = desc.identifier.section(":", 1, 1);

            if (pluginSources.find(tn) != pluginSources.end()) {
                if (pluginSources[tn] != pn && pluginMakers[tn] != desc.maker) {
                    ++names[tn];
                }
            } else {
                ++names[tn];
                pluginSources[tn] = pn;
                pluginMakers[tn] = desc.maker;
            }
        }

        std::map<QString, int> counts;
        m_transforms.clear();

        for (TransformDescriptionMap::iterator i = transforms.begin();
             i != transforms.end(); ++i) {

            TransformDescription desc = i->second;
            QString identifier = desc.identifier;
            QString maker = desc.maker;

            QString td = desc.name;
            QString tn = td.section(": ", 0, 0);
            QString to = td.section(": ", 1);

            if (names[tn] > 1) {
                maker.replace(QRegularExpression(tr(" [\\(<].*$")), "");
                tn = QString("%1 [%2]").arg(tn).arg(maker);
            }

            if (to != "") {
                desc.name = QString("%1: %2").arg(tn).arg(to);
            } else {
                desc.name = tn;
            }

            m_transforms[identifier] = desc;
        }            

        m_installedTransformsPopulated = true;
    }
    
#ifdef DEBUG_TRANSFORM_FACTORY
    SVCERR << "populateInstalledTransforms exiting" << endl;
#endif

    emit installedTransformsPopulated();
}

void
TransformFactory::populateFeatureExtractionPlugins(TransformDescriptionMap &transforms)
{
    FeatureExtractionPluginFactory *factory =
        FeatureExtractionPluginFactory::instance();

    QString errorMessage;
    std::vector<QString> plugs = factory->getPluginIdentifiers(errorMessage);
    if (errorMessage != "") {
        m_errorString = tr("Failed to list Vamp plugins: %1").arg(errorMessage);
    }
    
    if (m_exiting) return;

    for (int i = 0; in_range_for(plugs, i); ++i) {

        QString pluginId = plugs[i];

        piper_vamp::PluginStaticData psd = factory->getPluginStaticData(pluginId);

        if (psd.pluginKey == "") {
            SVCERR << "WARNING: TransformFactory::populateFeatureExtractionPlugins: No plugin static data available for instance " << pluginId << endl;
            continue;
        }

        QString pluginName = QString::fromStdString(psd.basic.name);
        QString category = factory->getPluginCategory(pluginId);
        
        const auto &basicOutputs = psd.basicOutputInfo;

        for (const auto &o: basicOutputs) {

            QString outputName = QString::fromStdString(o.name);

            QString transformId = QString("%1:%2")
                .arg(pluginId).arg(QString::fromStdString(o.identifier));

            QString userName;
            QString friendlyName;
//!!! return to this            QString units = outputs[j].unit.c_str();
            QString description = QString::fromStdString(psd.basic.description);
            QString maker = QString::fromStdString(psd.maker);
            if (maker == "") maker = tr("<unknown maker>");

            QString longDescription = description;

            if (longDescription == "") {
                if (basicOutputs.size() == 1) {
                    longDescription = tr("Extract features using \"%1\" plugin (from %2)")
                        .arg(pluginName).arg(maker);
                } else {
                    longDescription = tr("Extract features using \"%1\" output of \"%2\" plugin (from %3)")
                        .arg(outputName).arg(pluginName).arg(maker);
                }
            } else {
                if (basicOutputs.size() == 1) {
                    longDescription = tr("%1 using \"%2\" plugin (from %3)")
                        .arg(longDescription).arg(pluginName).arg(maker);
                } else {
                    longDescription = tr("%1 using \"%2\" output of \"%3\" plugin (from %4)")
                        .arg(longDescription).arg(outputName).arg(pluginName).arg(maker);
                }
            }                    

            if (basicOutputs.size() == 1) {
                userName = pluginName;
                friendlyName = pluginName;
            } else {
                userName = QString("%1: %2").arg(pluginName).arg(outputName);
                friendlyName = outputName;
            }

            bool configurable = (!psd.programs.empty() ||
                                 !psd.parameters.empty());

#ifdef DEBUG_TRANSFORM_FACTORY
            SVCERR << "Feature extraction plugin transform: " << transformId << " friendly name: " << friendlyName << endl;
#endif

            transforms[transformId] = 
                TransformDescription(TransformDescription::Analysis,
                                     category,
                                     transformId,
                                     userName,
                                     friendlyName,
                                     description,
                                     longDescription,
                                     maker,
//!!!                                     units,
                                     "",
                                     configurable);
        }
    }
}

void
TransformFactory::populateRealTimePlugins(TransformDescriptionMap &transforms)
{
    std::vector<QString> plugs =
        RealTimePluginFactory::getAllPluginIdentifiers();
    if (m_exiting) return;

    static QRegularExpression unitRE("[\\[\\(]([A-Za-z0-9/]+)[\\)\\]]$");

    for (int i = 0; in_range_for(plugs, i); ++i) {
        
        QString pluginId = plugs[i];

        RealTimePluginFactory *factory =
            RealTimePluginFactory::instanceFor(pluginId);

        if (!factory) {
            SVCERR << "WARNING: TransformFactory::populateRealTimePlugins: No real time plugin factory for instance " << pluginId << endl;
            continue;
        }

        RealTimePluginDescriptor descriptor =
            factory->getPluginDescriptor(pluginId);

        if (descriptor.name == "") {
            SVCERR << "WARNING: TransformFactory::populateRealTimePlugins: Failed to query plugin " << pluginId << endl;
            continue;
        }
        
//!!!        if (descriptor.controlOutputPortCount == 0 ||
//            descriptor.audioInputPortCount == 0) continue;

//        cout << "TransformFactory::populateRealTimePlugins: plugin " << pluginId << " has " << descriptor.controlOutputPortCount << " control output ports, " << descriptor.audioOutputPortCount << " audio outputs, " << descriptor.audioInputPortCount << " audio inputs" << endl;
        
        QString pluginName = descriptor.name.c_str();
        QString category = factory->getPluginCategory(pluginId);
        bool configurable = (descriptor.parameterCount > 0);
        QString maker = descriptor.maker.c_str();
        if (maker == "") maker = tr("<unknown maker>");

        if (descriptor.audioInputPortCount > 0) {

            for (int j = 0; j < (int)descriptor.controlOutputPortCount; ++j) {

                QString transformId = QString("%1:%2").arg(pluginId).arg(j);
                QString userName;
                QString units;
                QString portName;

                if (j < (int)descriptor.controlOutputPortNames.size() &&
                    descriptor.controlOutputPortNames[j] != "") {

                    portName = descriptor.controlOutputPortNames[j].c_str();

                    userName = tr("%1: %2")
                        .arg(pluginName)
                        .arg(portName);

                    auto match = unitRE.match(portName);
                    if (match.hasMatch()) {
                        units = match.captured(1);
                    }

                } else if (descriptor.controlOutputPortCount > 1) {

                    userName = tr("%1: Output %2")
                        .arg(pluginName)
                        .arg(j + 1);

                } else {

                    userName = pluginName;
                }

                QString description;

                if (portName != "") {
                    description = tr("Extract \"%1\" data output from \"%2\" effect plugin (from %3)")
                        .arg(portName)
                        .arg(pluginName)
                        .arg(maker);
                } else {
                    description = tr("Extract data output %1 from \"%2\" effect plugin (from %3)")
                        .arg(j + 1)
                        .arg(pluginName)
                        .arg(maker);
                }

                transforms[transformId] = 
                    TransformDescription(TransformDescription::EffectsData,
                                         category,
                                         transformId,
                                         userName,
                                         userName,
                                         "",
                                         description,
                                         maker,
                                         units,
                                         configurable);
            }
        }

        if (!descriptor.isSynth || descriptor.audioInputPortCount > 0) {

            if (descriptor.audioOutputPortCount > 0) {

                QString transformId = QString("%1:A").arg(pluginId);
                TransformDescription::Type type = TransformDescription::Effects;

                QString description = tr("Transform audio signal with \"%1\" effect plugin (from %2)")
                    .arg(pluginName)
                    .arg(maker);

                if (descriptor.audioInputPortCount == 0) {
                    type = TransformDescription::Generator;
                    QString description = tr("Generate audio signal using \"%1\" plugin (from %2)")
                        .arg(pluginName)
                        .arg(maker);
                }

                transforms[transformId] =
                    TransformDescription(type,
                                         category,
                                         transformId,
                                         pluginName,
                                         pluginName,
                                         "",
                                         description,
                                         maker,
                                         "",
                                         configurable);
            }
        }
    }
}

void
TransformFactory::populateUninstalledTransforms()
{
    if (m_exiting) return;

    populateInstalledTransforms();
    if (m_exiting) return;

    {
        MutexLocker locker(&m_uninstalledTransformsMutex,
                           "TransformFactory::populateUninstalledTransforms");
        if (m_uninstalledTransformsPopulated) return;

        PluginRDFIndexer::getInstance()->indexConfiguredURLs();
        if (m_exiting) return;

        PluginRDFIndexer::getInstance()->performConsistencyChecks();
    
        //!!! This will be amazingly slow

        QStringList ids = PluginRDFIndexer::getInstance()->getIndexedPluginIds();
    
        for (QStringList::const_iterator i = ids.begin(); i != ids.end(); ++i) {
        
            PluginRDFDescription desc(*i);

            QString name = desc.getPluginName();
#ifdef DEBUG_TRANSFORM_FACTORY
            if (name == "") {
                SVCERR << "TransformFactory::populateUninstalledTransforms: "
                       << "No name available for plugin " << *i
                       << ", skipping" << endl;
                continue;
            }
#endif

            QString description = desc.getPluginDescription();
            QString maker = desc.getPluginMaker();
            Provider provider = desc.getPluginProvider();

            QStringList oids = desc.getOutputIds();

            for (QStringList::const_iterator j = oids.begin(); j != oids.end(); ++j) {

                TransformId tid = Transform::getIdentifierForPluginOutput(*i, *j);
            
                if (m_transforms.find(tid) != m_transforms.end()) {
#ifdef DEBUG_TRANSFORM_FACTORY
                    SVCERR << "TransformFactory::populateUninstalledTransforms: "
                           << tid << " is installed; adding provider if present, skipping rest" << endl;
#endif
                    if (provider != Provider()) {
                        if (m_transforms[tid].provider == Provider()) {
                            m_transforms[tid].provider = provider;
                        }
                    }
                    continue;
                }

#ifdef DEBUG_TRANSFORM_FACTORY
                SVCERR << "TransformFactory::populateUninstalledTransforms: "
                       << "adding " << tid << endl;
#endif

                QString oname = desc.getOutputName(*j);
                if (oname == "") oname = *j;
            
                TransformDescription td;
                td.type = TransformDescription::Analysis;
                td.category = "";
                td.identifier = tid;

                if (oids.size() == 1) {
                    td.name = name;
                } else if (name != "") {
                    td.name = tr("%1: %2").arg(name).arg(oname);
                }

                QString longDescription = description;
                //!!! basically duplicated from above
                if (longDescription == "") {
                    if (oids.size() == 1) {
                        longDescription = tr("Extract features using \"%1\" plugin (from %2)")
                            .arg(name).arg(maker);
                    } else {
                        longDescription = tr("Extract features using \"%1\" output of \"%2\" plugin (from %3)")
                            .arg(oname).arg(name).arg(maker);
                    }
                } else {
                    if (oids.size() == 1) {
                        longDescription = tr("%1 using \"%2\" plugin (from %3)")
                            .arg(longDescription).arg(name).arg(maker);
                    } else {
                        longDescription = tr("%1 using \"%2\" output of \"%3\" plugin (from %4)")
                            .arg(longDescription).arg(oname).arg(name).arg(maker);
                    }
                }                    

                td.friendlyName = name; //!!!???
                td.description = description;
                td.longDescription = longDescription;
                td.maker = maker;
                td.provider = provider;
                td.units = "";
                td.configurable = false;

                m_uninstalledTransforms[tid] = td;
            }

            if (m_exiting) return;
        }
    
        m_uninstalledTransformsPopulated = true;
    }

#ifdef DEBUG_TRANSFORM_FACTORY
    SVCERR << "populateUninstalledTransforms exiting" << endl;
#endif

    emit uninstalledTransformsPopulated();
}

Transform
TransformFactory::getDefaultTransformFor(TransformId id, sv_samplerate_t rate)
{
    Transform t;
    t.setIdentifier(id);
    if (rate != 0) t.setSampleRate(rate);

    SVDEBUG << "TransformFactory::getDefaultTransformFor: identifier \""
            << id << "\"" << endl;
    
    std::shared_ptr<Vamp::PluginBase> plugin = instantiateDefaultPluginFor(id, rate);

    if (plugin) {
        t.setPluginVersion(QString("%1").arg(plugin->getPluginVersion()));
        setParametersFromPlugin(t, plugin);
        makeContextConsistentWithPlugin(t, plugin);
    }

    return t;
}

std::shared_ptr<Vamp::PluginBase> 
TransformFactory::instantiatePluginFor(const Transform &transform)
{
    SVDEBUG << "TransformFactory::instantiatePluginFor: identifier \""
            << transform.getIdentifier() << "\"" << endl;
    
    std::shared_ptr<Vamp::PluginBase> plugin = instantiateDefaultPluginFor
        (transform.getIdentifier(), transform.getSampleRate());

    if (plugin) {
        setPluginParameters(transform, plugin);
    }

    return plugin;
}

std::shared_ptr<Vamp::PluginBase> 
TransformFactory::instantiateDefaultPluginFor(TransformId identifier,
                                              sv_samplerate_t rate)
{
    populateInstalledTransforms();

    Transform t;
    t.setIdentifier(identifier);
    if (rate == 0) rate = 44100.0;
    QString pluginId = t.getPluginIdentifier();

    std::shared_ptr<Vamp::PluginBase> plugin = nullptr;

    if (t.getType() == Transform::FeatureExtraction) {

        SVDEBUG << "TransformFactory::instantiateDefaultPluginFor: identifier \""
                << identifier << "\" is a feature extraction transform" << endl;
        
        FeatureExtractionPluginFactory *factory =
            FeatureExtractionPluginFactory::instance();

        if (factory) {
            plugin = factory->instantiatePlugin(pluginId, rate);
        }

    } else if (t.getType() == Transform::RealTimeEffect) {

        SVDEBUG << "TransformFactory::instantiateDefaultPluginFor: identifier \""
                << identifier << "\" is a real-time transform" << endl;

        RealTimePluginFactory *factory = 
            RealTimePluginFactory::instanceFor(pluginId);

        if (factory) {
            plugin = factory->instantiatePlugin(pluginId, 0, 0, rate, 1024, 1);
        }

    } else {
        SVDEBUG << "TransformFactory: ERROR: transform id \""
                << identifier << "\" is of unknown type" << endl;
    }

    return plugin;
}

bool
TransformFactory::haveTransform(TransformId identifier)
{
    populateInstalledTransforms();
    return (m_transforms.find(identifier) != m_transforms.end());
}

QString
TransformFactory::getTransformName(TransformId identifier)
{
    populateInstalledTransforms();
    if (m_transforms.find(identifier) != m_transforms.end()) {
        return m_transforms[identifier].name;
    } else return "";
}

QString
TransformFactory::getTransformFriendlyName(TransformId identifier)
{
    populateInstalledTransforms();
    if (m_transforms.find(identifier) != m_transforms.end()) {
        return m_transforms[identifier].friendlyName;
    } else return "";
}

QString
TransformFactory::getTransformUnits(TransformId identifier)
{
    populateInstalledTransforms();
    if (m_transforms.find(identifier) != m_transforms.end()) {
        return m_transforms[identifier].units;
    } else return "";
}

Provider
TransformFactory::getTransformProvider(TransformId identifier)
{
    populateInstalledTransforms();
    if (m_transforms.find(identifier) != m_transforms.end()) {
        return m_transforms[identifier].provider;
    } else return {};
}

Vamp::Plugin::InputDomain
TransformFactory::getTransformInputDomain(TransformId identifier)
{
    populateInstalledTransforms();

    Transform transform;
    transform.setIdentifier(identifier);

    SVDEBUG << "TransformFactory::getTransformInputDomain: identifier \""
            << identifier << "\"" << endl;
    
    if (transform.getType() != Transform::FeatureExtraction) {
        return Vamp::Plugin::TimeDomain;
    }

    std::shared_ptr<Vamp::Plugin> plugin =
        std::dynamic_pointer_cast<Vamp::Plugin>
        (instantiateDefaultPluginFor(identifier, 0));

    if (plugin) {
        Vamp::Plugin::InputDomain d = plugin->getInputDomain();
        return d;
    }

    return Vamp::Plugin::TimeDomain;
}

bool
TransformFactory::isTransformConfigurable(TransformId identifier)
{
    populateInstalledTransforms();

    if (m_transforms.find(identifier) != m_transforms.end()) {
        return m_transforms[identifier].configurable;
    } else return false;
}

bool
TransformFactory::getTransformChannelRange(TransformId identifier,
                                           int &min, int &max)
{
    QString id = identifier.section(':', 0, 2);

    if (RealTimePluginFactory::instanceFor(id)) {

        RealTimePluginDescriptor descriptor = 
            RealTimePluginFactory::instanceFor(id)->
            getPluginDescriptor(id);
        if (descriptor.name == "") {
            return false;
        }

        min = descriptor.audioInputPortCount;
        max = descriptor.audioInputPortCount;

        return true;

    } else {

        auto psd = FeatureExtractionPluginFactory::instance()->
            getPluginStaticData(id);
        if (psd.pluginKey == "") return false;

        min = (int)psd.minChannelCount;
        max = (int)psd.maxChannelCount;

        return true;
    }

    return false;
}

void
TransformFactory::setParametersFromPlugin(Transform &transform,
                                          std::shared_ptr<Vamp::PluginBase> plugin)
{
    Transform::ParameterMap pmap;

    //!!! record plugin & API version

    //!!! check that this is the right plugin!

    Vamp::PluginBase::ParameterList parameters =
        plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        pmap[i->identifier.c_str()] = plugin->getParameter(i->identifier);
//        SVCERR << "TransformFactory::setParametersFromPlugin: parameter "
//                  << i->identifier << " -> value " <<
//            pmap[i->identifier.c_str()] << endl;
    }

    transform.setParameters(pmap);

    if (plugin->getPrograms().empty()) {
        transform.setProgram("");
    } else {
        transform.setProgram(plugin->getCurrentProgram().c_str());
    }

    std::shared_ptr<RealTimePluginInstance> rtpi =
        std::dynamic_pointer_cast<RealTimePluginInstance>(plugin);

    Transform::ConfigurationMap cmap;

    if (rtpi) {

        RealTimePluginInstance::ConfigurationPairMap configurePairs =
            rtpi->getConfigurePairs();

        for (RealTimePluginInstance::ConfigurationPairMap::const_iterator i
                 = configurePairs.begin(); i != configurePairs.end(); ++i) {
            cmap[i->first.c_str()] = i->second.c_str();
        }
    }

    transform.setConfiguration(cmap);
}

void
TransformFactory::setPluginParameters(const Transform &transform,
                                      std::shared_ptr<Vamp::PluginBase> plugin)
{
    //!!! check plugin & API version (see e.g. PluginXml::setParameters)

    //!!! check that this is the right plugin!

    std::shared_ptr<RealTimePluginInstance> rtpi =
        std::dynamic_pointer_cast<RealTimePluginInstance>(plugin);

    if (rtpi) {
        const Transform::ConfigurationMap &cmap = transform.getConfiguration();
        for (Transform::ConfigurationMap::const_iterator i = cmap.begin();
             i != cmap.end(); ++i) {
            rtpi->configure(i->first.toStdString(), i->second.toStdString());
        }
    }

    if (transform.getProgram() != "") {
        plugin->selectProgram(transform.getProgram().toStdString());
    }

    const Transform::ParameterMap &pmap = transform.getParameters();

    Vamp::PluginBase::ParameterList parameters =
        plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        QString key = i->identifier.c_str();
        Transform::ParameterMap::const_iterator pmi = pmap.find(key);
        if (pmi != pmap.end()) {
            plugin->setParameter(i->identifier, pmi->second);
        }
    }
}

void
TransformFactory::makeContextConsistentWithPlugin(Transform &transform,
                                                  std::shared_ptr<Vamp::PluginBase> plugin)
{
    std::shared_ptr<Vamp::Plugin> vp =
        std::dynamic_pointer_cast<Vamp::Plugin>(plugin);

    if (!vp) {
        // time domain input for real-time effects plugin
        if (!transform.getBlockSize()) {
            if (!transform.getStepSize()) transform.setStepSize(1024);
            transform.setBlockSize(transform.getStepSize());
        } else {
            transform.setStepSize(transform.getBlockSize());
        }
    } else {
        Vamp::Plugin::InputDomain domain = vp->getInputDomain();
        if (!transform.getStepSize()) {
            transform.setStepSize((int)vp->getPreferredStepSize());
        }
        if (!transform.getBlockSize()) {
            transform.setBlockSize((int)vp->getPreferredBlockSize());
        }
        if (!transform.getBlockSize()) {
            transform.setBlockSize(1024);
        }
        if (!transform.getStepSize()) {
            if (domain == Vamp::Plugin::FrequencyDomain) {
//                SVCERR << "frequency domain, step = " << blockSize/2 << endl;
                transform.setStepSize(transform.getBlockSize()/2);
            } else {
//                SVCERR << "time domain, step = " << blockSize/2 << endl;
                transform.setStepSize(transform.getBlockSize());
            }
        }
    }
}

QString
TransformFactory::getPluginConfigurationXml(const Transform &t)
{
    QString xml;

    SVDEBUG << "TransformFactory::getPluginConfigurationXml: identifier \""
            << t.getIdentifier() << "\"" << endl;

    auto plugin = instantiateDefaultPluginFor(t.getIdentifier(), 0);
    if (!plugin) {
        SVDEBUG << "TransformFactory::getPluginConfigurationXml: "
                << "Unable to instantiate plugin for transform \""
                << t.getIdentifier() << "\"" << endl;
        return xml;
    }

    setPluginParameters(t, plugin);

    QTextStream out(&xml);
    PluginXml(plugin).toXml(out);

    return xml;
}

void
TransformFactory::setParametersFromPluginConfigurationXml(Transform &t,
                                                          QString xml)
{
    SVDEBUG << "TransformFactory::setParametersFromPluginConfigurationXml: identifier \""
            << t.getIdentifier() << "\"" << endl;

    auto plugin = instantiateDefaultPluginFor(t.getIdentifier(), 0);
    if (!plugin) {
        SVDEBUG << "TransformFactory::setParametersFromPluginConfigurationXml: "
                << "Unable to instantiate plugin for transform \""
                << t.getIdentifier() << "\"" << endl;
        return;
    }

    PluginXml(plugin).setParametersFromXml(xml);
    setParametersFromPlugin(t, plugin);
}

TransformFactory::SearchResults
TransformFactory::search(QString keyword)
{
    QStringList keywords;
    keywords << keyword;
    return search(keywords);
}

TransformFactory::SearchResults
TransformFactory::search(QStringList keywords)
{
    populateInstalledTransforms();

    SearchResults results = searchUnadjusted(keywords);
    
    if (keywords.size() > 1) {

        // If there are any hits for all keywords in a row, put them
        // in (replacing previous hits for the same transforms) but
        // ensure they score more than any of the others

        int maxScore = 0;
        for (auto r: results) {
            if (r.second.score > maxScore) {
                maxScore = r.second.score;
            }
        }

        QStringList oneBigKeyword;
        oneBigKeyword << keywords.join(" ");
        SearchResults oneBigKeywordResults = searchUnadjusted(oneBigKeyword);
        for (auto r: oneBigKeywordResults) {
            results[r.first] = r.second;
            results[r.first].score += maxScore;
        }
    }

    return results;
}

TransformFactory::SearchResults
TransformFactory::searchUnadjusted(QStringList keywords)
{
    SearchResults results;
    TextMatcher matcher;

    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {

        TextMatcher::Match match;

        match.key = i->first;
        
        matcher.test(match, keywords,
                     getTransformTypeName(i->second.type),
                     tr("Plugin type"), 5);

        matcher.test(match, keywords, i->second.category, tr("Category"), 20);
        matcher.test(match, keywords, i->second.identifier, tr("System Identifier"), 6);
        matcher.test(match, keywords, i->second.name, tr("Name"), 30);
        matcher.test(match, keywords, i->second.description, tr("Description"), 20);
        matcher.test(match, keywords, i->second.maker, tr("Maker"), 10);
        matcher.test(match, keywords, i->second.units, tr("Units"), 10);

        if (match.score > 0) results[i->first] = match;
    }

    if (!m_uninstalledTransformsMutex.tryLock()) {
        // uninstalled transforms are being populated; this may take some time,
        // and they aren't critical, but we will speed them up if necessary
        SVDEBUG << "TransformFactory::search: Uninstalled transforms mutex is held, skipping" << endl;
        m_populatingSlowly = false;
        return results;
    }

    if (!m_uninstalledTransformsPopulated) {
        SVDEBUG << "WARNING: TransformFactory::search: Uninstalled transforms are not populated yet" << endl
                << "and are not being populated either -- was the thread not started correctly?" << endl;
        m_uninstalledTransformsMutex.unlock();
        return results;
    }

    m_uninstalledTransformsMutex.unlock();

    for (TransformDescriptionMap::const_iterator i = m_uninstalledTransforms.begin();
         i != m_uninstalledTransforms.end(); ++i) {

        TextMatcher::Match match;

        match.key = i->first;
        
        matcher.test(match, keywords,
                     getTransformTypeName(i->second.type),
                     tr("Plugin type"), 2);

        matcher.test(match, keywords, i->second.category, tr("Category"), 10);
        matcher.test(match, keywords, i->second.identifier, tr("System Identifier"), 3);
        matcher.test(match, keywords, i->second.name, tr("Name"), 15);
        matcher.test(match, keywords, i->second.description, tr("Description"), 10);
        matcher.test(match, keywords, i->second.maker, tr("Maker"), 5);
        matcher.test(match, keywords, i->second.units, tr("Units"), 5);

        if (match.score > 0) results[i->first] = match;
    }

#ifdef DEBUG_TRANSFORM_FACTORY
    SVCERR << "TransformFactory::search: keywords are: " << keywords.join(", ")
           << endl;
    int n = int(results.size()), i = 1;
    SVCERR << "TransformFactory::search: results (" << n << "):" << endl;
    
    for (const auto &r: results) {
        QStringList frags;
        for (const auto &f: r.second.fragments) {
            frags << QString("{\"%1\": \"%2\"}").arg(f.first).arg(f.second);
        }
        SVCERR << "[" << i << "/" << n << "] id " << r.first
               << ": score " << r.second.score
               << ", key " << r.second.key << ", fragments "
               << frags.join(";") << endl;
        ++i;
    }
    SVCERR << endl;
#endif
    
    return results;
}

} // end namespace sv

