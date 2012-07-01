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

#include "ModelTransformerFactory.h"

#include "FeatureExtractionModelTransformer.h"
#include "RealTimeEffectModelTransformer.h"

#include "TransformFactory.h"

#include "base/AudioPlaySource.h"

#include "plugin/FeatureExtractionPluginFactory.h"
#include "plugin/RealTimePluginFactory.h"
#include "plugin/PluginXml.h"

#include "data/model/DenseTimeValueModel.h"

#include <vamp-hostsdk/PluginHostAdapter.h>

#include <iostream>
#include <set>

#include <QRegExp>

ModelTransformerFactory *
ModelTransformerFactory::m_instance = new ModelTransformerFactory;

ModelTransformerFactory *
ModelTransformerFactory::getInstance()
{
    return m_instance;
}

ModelTransformerFactory::~ModelTransformerFactory()
{
}

ModelTransformer::Input
ModelTransformerFactory::getConfigurationForTransform(Transform &transform,
                                                      const std::vector<Model *> &candidateInputModels,
                                                      Model *defaultInputModel,
                                                      AudioPlaySource *source,
                                                      size_t startFrame,
                                                      size_t duration,
                                                      UserConfigurator *configurator)
{
    ModelTransformer::Input input(0);

    if (candidateInputModels.empty()) return input;

    //!!! This will need revision -- we'll have to have a callback
    //from the dialog for when the candidate input model is changed,
    //as we'll need to reinitialise the channel settings in the dialog
    Model *inputModel = candidateInputModels[0];
    QStringList candidateModelNames;
    QString defaultModelName;
    QMap<QString, Model *> modelMap;
    for (size_t i = 0; i < candidateInputModels.size(); ++i) {
        QString modelName = candidateInputModels[i]->objectName();
        QString origModelName = modelName;
        int dupcount = 1;
        while (modelMap.contains(modelName)) {
            modelName = tr("%1 <%2>").arg(origModelName).arg(++dupcount);
        }
        modelMap[modelName] = candidateInputModels[i];
        candidateModelNames.push_back(modelName);
        if (candidateInputModels[i] == defaultInputModel) {
            defaultModelName = modelName;
        }
    }

    QString id = transform.getPluginIdentifier();
    
    bool ok = true;
    QString configurationXml = m_lastConfigurations[transform.getIdentifier()];

    std::cerr << "last configuration: " << configurationXml << std::endl;

    Vamp::PluginBase *plugin = 0;

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        std::cerr << "getConfigurationForTransform: instantiating Vamp plugin" << std::endl;

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel->getSampleRate());

        plugin = vp;

    } else if (RealTimePluginFactory::instanceFor(id)) {

        RealTimePluginFactory *factory = RealTimePluginFactory::instanceFor(id);
        const RealTimePluginDescriptor *desc = factory->getPluginDescriptor(id);

        size_t sampleRate = inputModel->getSampleRate();
        size_t blockSize = 1024;
        size_t channels = 1;
        if (source) {
            sampleRate = source->getTargetSampleRate();
            blockSize = source->getTargetBlockSize();
            channels = source->getTargetChannelCount();
        }

        RealTimePluginInstance *rtp = factory->instantiatePlugin
            (id, 0, 0, sampleRate, blockSize, channels);

        plugin = rtp;
    }

    if (plugin) {

        // Ensure block size etc are valid
        TransformFactory::getInstance()->
            makeContextConsistentWithPlugin(transform, plugin);

        // Prepare the plugin with any existing parameters already
        // found in the transform
        TransformFactory::getInstance()->
            setPluginParameters(transform, plugin);
        
        // For this interactive usage, we want to override those with
        // whatever the user chose last time around
        PluginXml(plugin).setParametersFromXml(configurationXml);

        if (configurator) {
            ok = configurator->configure(input, transform, plugin,
                                         inputModel, source,
                                         startFrame, duration,
                                         modelMap,
                                         candidateModelNames,
                                         defaultModelName);
        }
        

        TransformFactory::getInstance()->
            makeContextConsistentWithPlugin(transform, plugin);

        configurationXml = PluginXml(plugin).toXmlString();

        delete plugin;
    }

    if (ok) {
        m_lastConfigurations[transform.getIdentifier()] = configurationXml;
        input.setModel(inputModel);
    }

    return input;
}

ModelTransformer *
ModelTransformerFactory::createTransformer(const Transform &transform,
                                           const ModelTransformer::Input &input)
{
    ModelTransformer *transformer = 0;

    QString id = transform.getPluginIdentifier();

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        transformer =
            new FeatureExtractionModelTransformer(input, transform);

    } else if (RealTimePluginFactory::instanceFor(id)) {

        transformer =
            new RealTimeEffectModelTransformer(input, transform);

    } else {
        SVDEBUG << "ModelTransformerFactory::createTransformer: Unknown transform \""
                  << transform.getIdentifier() << "\"" << endl;
        return transformer;
    }

    if (transformer) transformer->setObjectName(transform.getIdentifier());
    return transformer;
}

Model *
ModelTransformerFactory::transform(const Transform &transform,
                                   const ModelTransformer::Input &input,
                                   QString &message)
{
    SVDEBUG << "ModelTransformerFactory::transform: Constructing transformer with input model " << input.getModel() << endl;

    ModelTransformer *t = createTransformer(transform, input);
    if (!t) return 0;

    connect(t, SIGNAL(finished()), this, SLOT(transformerFinished()));

    m_runningTransformers.insert(t);

    t->start();
    Model *model = t->detachOutputModel();

    if (model) {
        QString imn = input.getModel()->objectName();
        QString trn =
            TransformFactory::getInstance()->getTransformFriendlyName
            (transform.getIdentifier());
        if (imn != "") {
            if (trn != "") {
                model->setObjectName(tr("%1: %2").arg(imn).arg(trn));
            } else {
                model->setObjectName(imn);
            }
        } else if (trn != "") {
            model->setObjectName(trn);
        }
    } else {
        t->wait();
    }

    message = t->getMessage();

    return model;
}

void
ModelTransformerFactory::transformerFinished()
{
    QObject *s = sender();
    ModelTransformer *transformer = dynamic_cast<ModelTransformer *>(s);
    
//    SVDEBUG << "ModelTransformerFactory::transformerFinished(" << transformer << ")" << endl;

    if (!transformer) {
	std::cerr << "WARNING: ModelTransformerFactory::transformerFinished: sender is not a transformer" << std::endl;
	return;
    }

    if (m_runningTransformers.find(transformer) == m_runningTransformers.end()) {
        std::cerr << "WARNING: ModelTransformerFactory::transformerFinished(" 
                  << transformer
                  << "): I have no record of this transformer running!"
                  << std::endl;
    }

    m_runningTransformers.erase(transformer);

    transformer->wait(); // unnecessary but reassuring
    delete transformer;
}

void
ModelTransformerFactory::modelAboutToBeDeleted(Model *m)
{
    TransformerSet affected;

    for (TransformerSet::iterator i = m_runningTransformers.begin();
         i != m_runningTransformers.end(); ++i) {

        ModelTransformer *t = *i;

        if (t->getInputModel() == m || t->getOutputModel() == m) {
            affected.insert(t);
        }
    }

    for (TransformerSet::iterator i = affected.begin();
         i != affected.end(); ++i) {

        ModelTransformer *t = *i;

        t->abandon();

        t->wait(); // this should eventually call back on
                   // transformerFinished, which will remove from
                   // m_runningTransformers and delete.
    }
}

