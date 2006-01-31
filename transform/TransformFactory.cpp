/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
   
    This is experimental software.  Not for distribution.
*/

#include "TransformFactory.h"

#include "BeatDetectTransform.h"
#include "BeatDetectionFunctionTransform.h"
#include "FeatureExtractionPluginTransform.h"

#include "plugin/FeatureExtractionPluginFactory.h"

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
    for (TransformMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
	list.push_back(TransformDesc(i->first, i->second));
    }

    return list;
}

void
TransformFactory::populateTransforms()
{
    //!!!
    std::vector<QString> fexplugs =
	FeatureExtractionPluginFactory::getAllPluginIdentifiers();

    for (size_t i = 0; i < fexplugs.size(); ++i) {

	QString pluginId = fexplugs[i];

	FeatureExtractionPluginFactory *factory =
	    FeatureExtractionPluginFactory::instanceFor(pluginId);

	if (factory) {
	    //!!! well, really we want to be able to query this without having to instantiate

	    FeatureExtractionPlugin *plugin = 
		factory->instantiatePlugin(pluginId, 48000);

	    QString pluginDescription = plugin->getDescription().c_str();

	    if (plugin) {

		FeatureExtractionPlugin::OutputList outputs =
		    plugin->getOutputDescriptors();

		if (outputs.size() == 1) {
		    m_transforms[QString("%1:%2")
				 .arg(pluginId)
				 .arg(outputs[0].name.c_str())]
			= pluginDescription;
		} else {
		    for (size_t j = 0; j < outputs.size(); ++j) {
			m_transforms[QString("%1:%2")
				     .arg(pluginId)
				     .arg(outputs[j].name.c_str())]
			    = QString("%1: %2")
			    .arg(pluginDescription)
			    .arg(outputs[j].description.c_str());
		    }
		}
	    }
	}
    }
}

QString
TransformFactory::getTransformDescription(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name];
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

Transform *
TransformFactory::createTransform(TransformName name, Model *inputModel)
{
    return createTransform(name, inputModel, true);
}

Transform *
TransformFactory::createTransform(TransformName name, Model *inputModel,
				  bool start)
{
    Transform *transform = 0;

    if (name == BeatDetectTransform::getName()) {
	transform = new BeatDetectTransform(inputModel);
    } else if (name == BeatDetectionFunctionTransform::getName()) {
	transform = new BeatDetectionFunctionTransform(inputModel);
    } else {
	QString id = name.section(':', 0, 2);
	QString output = name.section(':', 3);
	if (FeatureExtractionPluginFactory::instanceFor(id)) {
	    transform = new FeatureExtractionPluginTransform(inputModel,
							     id, output);
	} else {
	    std::cerr << "TransformFactory::createTransform: Unknown transform "
		      << name.toStdString() << std::endl;
	}
    }

    if (start && transform) transform->start();
    transform->setObjectName(name);
    return transform;
}

Model *
TransformFactory::transform(TransformName name, Model *inputModel)
{
    Transform *t = createTransform(name, inputModel, false);

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

