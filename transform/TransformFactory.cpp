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

#include "vamp-sdk/Plugin.h"
#include "vamp-sdk/PluginHostAdapter.h"
#include "vamp-sdk/hostext/PluginWrapper.h"

#include <iostream>
#include <set>

#include <QRegExp>
#include <QTextStream>

TransformFactory *
TransformFactory::m_instance = new TransformFactory;

TransformFactory *
TransformFactory::getInstance()
{
    return m_instance;
}

TransformFactory::~TransformFactory()
{
}

TransformList
TransformFactory::getAllTransformDescriptions()
{
    if (m_transforms.empty()) populateTransforms();

    std::set<TransformDescription> dset;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
//        std::cerr << "inserting transform into set: id = " << i->second.identifier.toStdString() << std::endl;
	dset.insert(i->second);
    }

    TransformList list;
    for (std::set<TransformDescription>::const_iterator i = dset.begin();
	 i != dset.end(); ++i) {
//        std::cerr << "inserting transform into list: id = " << i->identifier.toStdString() << std::endl;
	list.push_back(*i);
    }

    return list;
}

TransformDescription
TransformFactory::getTransformDescription(TransformId id)
{
    if (m_transforms.empty()) populateTransforms();

    if (m_transforms.find(id) == m_transforms.end()) {
        return TransformDescription();
    }

    return m_transforms[id];
}

std::vector<QString>
TransformFactory::getAllTransformTypes()
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> types;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
        types.insert(i->second.type);
    }

    std::vector<QString> rv;
    for (std::set<QString>::iterator i = types.begin(); i != types.end(); ++i) {
        rv.push_back(*i);
    }

    return rv;
}

std::vector<QString>
TransformFactory::getTransformCategories(QString transformType)
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> categories;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            categories.insert(i->second.category);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (std::set<QString>::iterator i = categories.begin(); 
         i != categories.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

std::vector<QString>
TransformFactory::getTransformMakers(QString transformType)
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> makers;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            makers.insert(i->second.maker);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (std::set<QString>::iterator i = makers.begin(); 
         i != makers.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

void
TransformFactory::populateTransforms()
{
    TransformDescriptionMap transforms;

    populateFeatureExtractionPlugins(transforms);
    populateRealTimePlugins(transforms);

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
            maker.replace(QRegExp(tr(" [\\(<].*$")), "");
	    tn = QString("%1 [%2]").arg(tn).arg(maker);
	}

        if (to != "") {
            desc.name = QString("%1: %2").arg(tn).arg(to);
        } else {
            desc.name = tn;
        }

	m_transforms[identifier] = desc;
    }	    
}

void
TransformFactory::populateFeatureExtractionPlugins(TransformDescriptionMap &transforms)
{
    std::vector<QString> plugs =
	FeatureExtractionPluginFactory::getAllPluginIdentifiers();

    for (size_t i = 0; i < plugs.size(); ++i) {

	QString pluginId = plugs[i];

	FeatureExtractionPluginFactory *factory =
	    FeatureExtractionPluginFactory::instanceFor(pluginId);

	if (!factory) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: No feature extraction plugin factory for instance " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}

	Vamp::Plugin *plugin = 
	    factory->instantiatePlugin(pluginId, 44100);

	if (!plugin) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: Failed to instantiate plugin " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}
		
	QString pluginName = plugin->getName().c_str();
        QString category = factory->getPluginCategory(pluginId);

	Vamp::Plugin::OutputList outputs =
	    plugin->getOutputDescriptors();

	for (size_t j = 0; j < outputs.size(); ++j) {

	    QString transformId = QString("%1:%2")
		    .arg(pluginId).arg(outputs[j].identifier.c_str());

	    QString userName;
            QString friendlyName;
            QString units = outputs[j].unit.c_str();
            QString description = plugin->getDescription().c_str();
            QString maker = plugin->getMaker().c_str();
            if (maker == "") maker = tr("<unknown maker>");

            if (description == "") {
                if (outputs.size() == 1) {
                    description = tr("Extract features using \"%1\" plugin (from %2)")
                        .arg(pluginName).arg(maker);
                } else {
                    description = tr("Extract features using \"%1\" output of \"%2\" plugin (from %3)")
                        .arg(outputs[j].name.c_str()).arg(pluginName).arg(maker);
                }
            } else {
                if (outputs.size() == 1) {
                    description = tr("%1 using \"%2\" plugin (from %3)")
                        .arg(description).arg(pluginName).arg(maker);
                } else {
                    description = tr("%1 using \"%2\" output of \"%3\" plugin (from %4)")
                        .arg(description).arg(outputs[j].name.c_str()).arg(pluginName).arg(maker);
                }
            }                    

	    if (outputs.size() == 1) {
		userName = pluginName;
                friendlyName = pluginName;
	    } else {
		userName = QString("%1: %2")
		    .arg(pluginName)
		    .arg(outputs[j].name.c_str());
                friendlyName = outputs[j].name.c_str();
	    }

            bool configurable = (!plugin->getPrograms().empty() ||
                                 !plugin->getParameterDescriptors().empty());

//            std::cerr << "Feature extraction plugin transform: " << transformId.toStdString() << " friendly name: " << friendlyName.toStdString() << std::endl;

	    transforms[transformId] = 
                TransformDescription(tr("Analysis"),
                                     category,
                                     transformId,
                                     userName,
                                     friendlyName,
                                     description,
                                     maker,
                                     units,
                                     configurable);
	}

        delete plugin;
    }
}

void
TransformFactory::populateRealTimePlugins(TransformDescriptionMap &transforms)
{
    std::vector<QString> plugs =
	RealTimePluginFactory::getAllPluginIdentifiers();

    static QRegExp unitRE("[\\[\\(]([A-Za-z0-9/]+)[\\)\\]]$");

    for (size_t i = 0; i < plugs.size(); ++i) {
        
	QString pluginId = plugs[i];

        RealTimePluginFactory *factory =
            RealTimePluginFactory::instanceFor(pluginId);

	if (!factory) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: No real time plugin factory for instance " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}

        const RealTimePluginDescriptor *descriptor =
            factory->getPluginDescriptor(pluginId);

        if (!descriptor) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: Failed to query plugin " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}
	
//!!!        if (descriptor->controlOutputPortCount == 0 ||
//            descriptor->audioInputPortCount == 0) continue;

//        std::cout << "TransformFactory::populateRealTimePlugins: plugin " << pluginId.toStdString() << " has " << descriptor->controlOutputPortCount << " control output ports, " << descriptor->audioOutputPortCount << " audio outputs, " << descriptor->audioInputPortCount << " audio inputs" << std::endl;
	
	QString pluginName = descriptor->name.c_str();
        QString category = factory->getPluginCategory(pluginId);
        bool configurable = (descriptor->parameterCount > 0);
        QString maker = descriptor->maker.c_str();
        if (maker == "") maker = tr("<unknown maker>");

        if (descriptor->audioInputPortCount > 0) {

            for (size_t j = 0; j < descriptor->controlOutputPortCount; ++j) {

                QString transformId = QString("%1:%2").arg(pluginId).arg(j);
                QString userName;
                QString units;
                QString portName;

                if (j < descriptor->controlOutputPortNames.size() &&
                    descriptor->controlOutputPortNames[j] != "") {

                    portName = descriptor->controlOutputPortNames[j].c_str();

                    userName = tr("%1: %2")
                        .arg(pluginName)
                        .arg(portName);

                    if (unitRE.indexIn(portName) >= 0) {
                        units = unitRE.cap(1);
                    }

                } else if (descriptor->controlOutputPortCount > 1) {

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
                    TransformDescription(tr("Effects Data"),
                                         category,
                                         transformId,
                                         userName,
                                         userName,
                                         description,
                                         maker,
                                         units,
                                         configurable);
            }
        }

        if (!descriptor->isSynth || descriptor->audioInputPortCount > 0) {

            if (descriptor->audioOutputPortCount > 0) {

                QString transformId = QString("%1:A").arg(pluginId);
                QString type = tr("Effects");

                QString description = tr("Transform audio signal with \"%1\" effect plugin (from %2)")
                    .arg(pluginName)
                    .arg(maker);

                if (descriptor->audioInputPortCount == 0) {
                    type = tr("Generators");
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
                                         description,
                                         maker,
                                         "",
                                         configurable);
            }
        }
    }
}


Transform
TransformFactory::getDefaultTransformFor(TransformId id, size_t rate)
{
    Transform t;
    t.setIdentifier(id);
    if (rate != 0) t.setSampleRate(rate);

    Vamp::PluginBase *plugin = instantiateDefaultPluginFor(id, rate);

    if (plugin) {
        t.setPluginVersion(QString("%1").arg(plugin->getPluginVersion()));
        setParametersFromPlugin(t, plugin);
        makeContextConsistentWithPlugin(t, plugin);
        delete plugin;
    }

    return t;
}

Vamp::PluginBase *
TransformFactory::instantiatePluginFor(const Transform &transform)
{
    Vamp::PluginBase *plugin = instantiateDefaultPluginFor
        (transform.getIdentifier(), transform.getSampleRate());
    if (plugin) {
        setPluginParameters(transform, plugin);
    }
    return plugin;
}

Vamp::PluginBase *
TransformFactory::instantiateDefaultPluginFor(TransformId identifier, size_t rate)
{
    Transform t;
    t.setIdentifier(identifier);
    if (rate == 0) rate = 44100;
    QString pluginId = t.getPluginIdentifier();

    Vamp::PluginBase *plugin = 0;

    if (t.getType() == Transform::FeatureExtraction) {

        FeatureExtractionPluginFactory *factory = 
            FeatureExtractionPluginFactory::instanceFor(pluginId);

        plugin = factory->instantiatePlugin(pluginId, rate);

    } else {

        RealTimePluginFactory *factory = 
            RealTimePluginFactory::instanceFor(pluginId);
            
        plugin = factory->instantiatePlugin(pluginId, 0, 0, rate, 1024, 1);
    }

    return plugin;
}

Vamp::Plugin *
TransformFactory::downcastVampPlugin(Vamp::PluginBase *plugin)
{
    Vamp::Plugin *vp = dynamic_cast<Vamp::Plugin *>(plugin);
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::Plugin" << std::endl;
        vp = dynamic_cast<Vamp::PluginHostAdapter *>(plugin); //!!! why?
}
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::PluginHostAdapter" << std::endl;
        vp = dynamic_cast<Vamp::HostExt::PluginWrapper *>(plugin); //!!! no, I mean really why?
    }
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::HostExt::PluginWrapper" << std::endl;
    }
    return vp;
}

bool
TransformFactory::haveTransform(TransformId identifier)
{
    if (m_transforms.empty()) populateTransforms();
    return (m_transforms.find(identifier) != m_transforms.end());
}

QString
TransformFactory::getTransformName(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].name;
    } else return "";
}

QString
TransformFactory::getTransformFriendlyName(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].friendlyName;
    } else return "";
}

QString
TransformFactory::getTransformUnits(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].units;
    } else return "";
}

Vamp::Plugin::InputDomain
TransformFactory::getTransformInputDomain(TransformId identifier)
{
    Transform transform;
    transform.setIdentifier(identifier);

    if (transform.getType() != Transform::FeatureExtraction) {
        return Vamp::Plugin::TimeDomain;
    }

    Vamp::Plugin *plugin =
        downcastVampPlugin(instantiateDefaultPluginFor(identifier, 0));

    if (plugin) {
        Vamp::Plugin::InputDomain d = plugin->getInputDomain();
        delete plugin;
        return d;
    }

    return Vamp::Plugin::TimeDomain;
}

bool
TransformFactory::isTransformConfigurable(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].configurable;
    } else return false;
}

bool
TransformFactory::getTransformChannelRange(TransformId identifier,
                                           int &min, int &max)
{
    QString id = identifier.section(':', 0, 2);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *plugin = 
            FeatureExtractionPluginFactory::instanceFor(id)->
            instantiatePlugin(id, 44100);
        if (!plugin) return false;

        min = plugin->getMinChannelCount();
        max = plugin->getMaxChannelCount();
        delete plugin;

        return true;

    } else if (RealTimePluginFactory::instanceFor(id)) {

        // don't need to instantiate

        const RealTimePluginDescriptor *descriptor = 
            RealTimePluginFactory::instanceFor(id)->
            getPluginDescriptor(id);
        if (!descriptor) return false;

        min = descriptor->audioInputPortCount;
        max = descriptor->audioInputPortCount;

        return true;
    }

    return false;
}

void
TransformFactory::setParametersFromPlugin(Transform &transform,
                                          Vamp::PluginBase *plugin)
{
    Transform::ParameterMap pmap;

    //!!! record plugin & API version

    //!!! check that this is the right plugin!

    Vamp::PluginBase::ParameterList parameters =
        plugin->getParameterDescriptors();

    for (Vamp::PluginBase::ParameterList::const_iterator i = parameters.begin();
         i != parameters.end(); ++i) {
        pmap[i->identifier.c_str()] = plugin->getParameter(i->identifier);
    }

    transform.setParameters(pmap);

    if (plugin->getPrograms().empty()) {
        transform.setProgram("");
    } else {
        transform.setProgram(plugin->getCurrentProgram().c_str());
    }

    RealTimePluginInstance *rtpi =
        dynamic_cast<RealTimePluginInstance *>(plugin);

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
                                      Vamp::PluginBase *plugin)
{
    //!!! check plugin & API version (see e.g. PluginXml::setParameters)

    //!!! check that this is the right plugin!

    RealTimePluginInstance *rtpi =
        dynamic_cast<RealTimePluginInstance *>(plugin);

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
                                                  Vamp::PluginBase *plugin)
{
    const Vamp::Plugin *vp = downcastVampPlugin(plugin);

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
            transform.setStepSize(vp->getPreferredStepSize());
        }
        if (!transform.getBlockSize()) {
            transform.setBlockSize(vp->getPreferredBlockSize());
        }
        if (!transform.getBlockSize()) {
            transform.setBlockSize(1024);
        }
        if (!transform.getStepSize()) {
            if (domain == Vamp::Plugin::FrequencyDomain) {
//                std::cerr << "frequency domain, step = " << blockSize/2 << std::endl;
                transform.setStepSize(transform.getBlockSize()/2);
            } else {
//                std::cerr << "time domain, step = " << blockSize/2 << std::endl;
                transform.setStepSize(transform.getBlockSize());
            }
        }
    }
}

QString
TransformFactory::getPluginConfigurationXml(const Transform &t)
{
    QString xml;

    Vamp::PluginBase *plugin = instantiateDefaultPluginFor
        (t.getIdentifier(), 0);
    if (!plugin) {
        std::cerr << "TransformFactory::getPluginConfigurationXml: "
                  << "Unable to instantiate plugin for transform \""
                  << t.getIdentifier().toStdString() << "\"" << std::endl;
        return xml;
    }

    setPluginParameters(t, plugin);

    QTextStream out(&xml);
    PluginXml(plugin).toXml(out);
    delete plugin;

    return xml;
}

void
TransformFactory::setParametersFromPluginConfigurationXml(Transform &t,
                                                          QString xml)
{
    Vamp::PluginBase *plugin = instantiateDefaultPluginFor
        (t.getIdentifier(), 0);
    if (!plugin) {
        std::cerr << "TransformFactory::setParametersFromPluginConfigurationXml: "
                  << "Unable to instantiate plugin for transform \""
                  << t.getIdentifier().toStdString() << "\"" << std::endl;
        return;
    }

    PluginXml(plugin).setParametersFromXml(xml);
    setParametersFromPlugin(t, plugin);
    delete plugin;
}

