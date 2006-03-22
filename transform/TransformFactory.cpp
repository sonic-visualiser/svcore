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

#include "TransformFactory.h"

#include "FeatureExtractionPluginTransform.h"

#include "plugin/FeatureExtractionPluginFactory.h"

#include "widgets/PluginParameterDialog.h"

#include <iostream>

TransformFactory *
TransformFactory::m_instance = new TransformFactory;

TransformFactory *
TransformFactory::instance()
{
    return m_instance;
}

TransformFactory::~TransformFactory()
{
}

TransformFactory::TransformList
TransformFactory::getAllTransforms()
{
    if (m_transforms.empty()) populateTransforms();

    TransformList list;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
	list.push_back(i->second);
    }

    return list;
}

void
TransformFactory::populateTransforms()
{
    std::vector<QString> fexplugs =
	FeatureExtractionPluginFactory::getAllPluginIdentifiers();

    std::map<QString, QString> makers;

    for (size_t i = 0; i < fexplugs.size(); ++i) {

	QString pluginId = fexplugs[i];

	FeatureExtractionPluginFactory *factory =
	    FeatureExtractionPluginFactory::instanceFor(pluginId);

	if (!factory) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: No feature extraction plugin factory for instance " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}

	FeatureExtractionPlugin *plugin = 
	    factory->instantiatePlugin(pluginId, 48000);

	if (!plugin) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: Failed to instantiate plugin " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}
		
	QString pluginDescription = plugin->getDescription().c_str();
	FeatureExtractionPlugin::OutputList outputs =
	    plugin->getOutputDescriptors();

	for (size_t j = 0; j < outputs.size(); ++j) {

	    QString transformName = QString("%1:%2")
		    .arg(pluginId).arg(outputs[j].name.c_str());

	    QString userDescription;

	    if (outputs.size() == 1) {
		userDescription = pluginDescription;
	    } else {
		userDescription = QString("%1: %2")
		    .arg(pluginDescription)
		    .arg(outputs[j].description.c_str());
	    }

            bool configurable = (!plugin->getPrograms().empty() ||
                                 !plugin->getParameterDescriptors().empty());

	    m_transforms[transformName] = 
                TransformDesc(transformName,
                              userDescription,
                              configurable);
	    
	    makers[transformName] = plugin->getMaker().c_str();
	}
    }

    // disambiguate plugins with similar descriptions

    std::map<QString, int> descriptions;

    for (TransformDescriptionMap::iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {

        TransformDesc desc = i->second;

	++descriptions[desc.description];
	++descriptions[QString("%1 [%2]").arg(desc.description).arg(makers[desc.name])];
    }

    std::map<QString, int> counts;
    TransformDescriptionMap newMap;

    for (TransformDescriptionMap::iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {

        TransformDesc desc = i->second;
	QString name = desc.name, description = desc.description;

	if (descriptions[description] > 1) {
	    description = QString("%1 [%2]").arg(description).arg(makers[name]);
	    if (descriptions[description] > 1) {
		description = QString("%1 <%2>")
		    .arg(description).arg(++counts[description]);
	    }
	}

        desc.description = description;
	newMap[name] = desc;
    }	    
	    
    m_transforms = newMap;
}

QString
TransformFactory::getTransformDescription(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name].description;
    } else return "";
}

QString
TransformFactory::getTransformFriendlyName(TransformName name)
{
    QString description = getTransformDescription(name);

    int i = description.indexOf(':');
    if (i >= 0) {
	return description.remove(0, i + 2);
    } else {
	return description;
    }
}

bool
TransformFactory::getConfigurationForTransform(TransformName name,
                                               Model *inputModel,
                                               QString &configurationXml)
{
    QString id = name.section(':', 0, 2);
    QString output = name.section(':', 3);
    
    bool ok = false;
    configurationXml = m_lastConfigurations[name];

    std::cerr << "last configuration: " << configurationXml.toStdString() << std::endl;

    if (FeatureExtractionPluginFactory::instanceFor(id)) {
        FeatureExtractionPlugin *plugin =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel->getSampleRate());
        if (plugin) {
            if (configurationXml != "") {
                plugin->setParametersFromXml(configurationXml);
            }
            PluginParameterDialog *dialog = new PluginParameterDialog(plugin);
            if (dialog->exec() == QDialog::Accepted) {
                ok = true;
            }
            configurationXml = plugin->toXmlString();
            delete plugin;
        }
    }

    if (ok) m_lastConfigurations[name] = configurationXml;

    return ok;
}

Transform *
TransformFactory::createTransform(TransformName name, Model *inputModel,
				  QString configurationXml, bool start)
{
    Transform *transform = 0;

    // The only transform type we support at the moment is the
    // FeatureExtractionPluginTransform.  In future we may wish to
    // support e.g. RealTimePluginTransform for audio->audio or
    // audio->midi transforms using standard effects plugins.

    QString id = name.section(':', 0, 2);
    QString output = name.section(':', 3);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {
        transform = new FeatureExtractionPluginTransform(inputModel,
                                                         id,
                                                         configurationXml,
                                                         output);
    } else {
        std::cerr << "TransformFactory::createTransform: Unknown transform "
                  << name.toStdString() << std::endl;
    }

    if (start && transform) transform->start();
    transform->setObjectName(name);
    return transform;
}

Model *
TransformFactory::transform(TransformName name, Model *inputModel,
                            QString configurationXml)
{
    Transform *t = createTransform(name, inputModel, configurationXml, false);

    if (!t) return 0;

    connect(t, SIGNAL(finished()), this, SLOT(transformFinished()));

    t->start();
    return t->detachOutputModel();
}

void
TransformFactory::transformFinished()
{
    QObject *s = sender();
    Transform *transform = dynamic_cast<Transform *>(s);
    
    if (!transform) {
	std::cerr << "WARNING: TransformFactory::transformFinished: sender is not a transform" << std::endl;
	return;
    }

    transform->wait(); // unnecessary but reassuring
    delete transform;
}

