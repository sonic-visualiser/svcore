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
#include <map>

#include <QRegExp>
#include <QMutexLocker>

using std::vector;
using std::set;
using std::map;

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
                                                      const vector<Model *> &candidateInputModels,
                                                      Model *defaultInputModel,
                                                      AudioPlaySource *source,
                                                      sv_frame_t startFrame,
                                                      sv_frame_t duration,
                                                      UserConfigurator *configurator)
{
    QMutexLocker locker(&m_mutex);
    
    ModelTransformer::Input input(nullptr);

    if (candidateInputModels.empty()) return input;

    //!!! This will need revision -- we'll have to have a callback
    //from the dialog for when the candidate input model is changed,
    //as we'll need to reinitialise the channel settings in the dialog
    Model *inputModel = candidateInputModels[0];
    QStringList candidateModelNames;
    QString defaultModelName;
    QMap<QString, Model *> modelMap;
    for (int i = 0; i < (int)candidateInputModels.size(); ++i) {
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

    SVDEBUG << "ModelTransformer: last configuration for identifier " << transform.getIdentifier() << ": " << configurationXml << endl;

    Vamp::PluginBase *plugin = nullptr;

    if (RealTimePluginFactory::instanceFor(id)) {

        SVDEBUG << "ModelTransformerFactory::getConfigurationForTransform: instantiating real-time plugin" << endl;
        
        RealTimePluginFactory *factory = RealTimePluginFactory::instanceFor(id);

        sv_samplerate_t sampleRate = inputModel->getSampleRate();
        int blockSize = 1024;
        int channels = 1;
        if (source) {
            sampleRate = source->getSourceSampleRate();
            blockSize = source->getTargetBlockSize();
            channels = source->getTargetChannelCount();
        }

        RealTimePluginInstance *rtp = factory->instantiatePlugin
            (id, 0, 0, sampleRate, blockSize, channels);

        plugin = rtp;

    } else {

        SVDEBUG << "ModelTransformerFactory::getConfigurationForTransform: instantiating Vamp plugin" << endl;

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instance()->instantiatePlugin
            (id, float(inputModel->getSampleRate()));

        plugin = vp;
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

        SVDEBUG << "ModelTransformerFactory::getConfigurationForTransform: got configuration, deleting plugin" << endl;
        
        delete plugin;
    }

    if (ok) {
        m_lastConfigurations[transform.getIdentifier()] = configurationXml;
        input.setModel(inputModel);
    }

    return input;
}

ModelTransformer *
ModelTransformerFactory::createTransformer(const Transforms &transforms,
                                           const ModelTransformer::Input &input)
{
    ModelTransformer *transformer = nullptr;

    QString id = transforms[0].getPluginIdentifier();

    if (RealTimePluginFactory::instanceFor(id)) {

        transformer =
            new RealTimeEffectModelTransformer(input, transforms[0]);

    } else {

        transformer =
            new FeatureExtractionModelTransformer(input, transforms);
    }

    if (transformer) transformer->setObjectName(transforms[0].getIdentifier());
    return transformer;
}

Model *
ModelTransformerFactory::transform(const Transform &transform,
                                   const ModelTransformer::Input &input,
                                   QString &message,
                                   AdditionalModelHandler *handler) 
{
    SVDEBUG << "ModelTransformerFactory::transform: Constructing transformer with input model " << input.getModel() << endl;

    Transforms transforms;
    transforms.push_back(transform);
    vector<Model *> mm = transformMultiple(transforms, input, message, handler);
    if (mm.empty()) return nullptr;
    else return mm[0];
}

vector<Model *>
ModelTransformerFactory::transformMultiple(const Transforms &transforms,
                                           const ModelTransformer::Input &input,
                                           QString &message,
                                           AdditionalModelHandler *handler) 
{
    SVDEBUG << "ModelTransformerFactory::transformMultiple: Constructing transformer with input model " << input.getModel() << endl;
    
    QMutexLocker locker(&m_mutex);
    
    ModelTransformer *t = createTransformer(transforms, input);
    if (!t) return vector<Model *>();

    if (handler) {
        m_handlers[t] = handler;
    }

    m_runningTransformers.insert(t);

    connect(t, SIGNAL(finished()), this, SLOT(transformerFinished()));

    t->start();
    vector<Model *> models = t->detachOutputModels();

    if (!models.empty()) {
        QString imn = input.getModel()->objectName();
        QString trn =
            TransformFactory::getInstance()->getTransformFriendlyName
            (transforms[0].getIdentifier());
        for (int i = 0; i < (int)models.size(); ++i) {
            if (imn != "") {
                if (trn != "") {
                    models[i]->setObjectName(tr("%1: %2").arg(imn).arg(trn));
                } else {
                    models[i]->setObjectName(imn);
                }
            } else if (trn != "") {
                models[i]->setObjectName(trn);
            }
        }
    } else {
        t->wait();
    }

    message = t->getMessage();

    return models;
}

void
ModelTransformerFactory::transformerFinished()
{
    QObject *s = sender();
    ModelTransformer *transformer = dynamic_cast<ModelTransformer *>(s);
    
//    SVDEBUG << "ModelTransformerFactory::transformerFinished(" << transformer << ")" << endl;

    if (!transformer) {
        cerr << "WARNING: ModelTransformerFactory::transformerFinished: sender is not a transformer" << endl;
        return;
    }

    m_mutex.lock();
    
    if (m_runningTransformers.find(transformer) == m_runningTransformers.end()) {
        cerr << "WARNING: ModelTransformerFactory::transformerFinished(" 
                  << transformer
                  << "): I have no record of this transformer running!"
                  << endl;
    }

    m_runningTransformers.erase(transformer);

    map<AdditionalModelHandler *, vector<Model *>> toNotifyOfMore;
    vector<AdditionalModelHandler *> toNotifyOfNoMore;
    
    if (m_handlers.find(transformer) != m_handlers.end()) {
        if (transformer->willHaveAdditionalOutputModels()) {
            vector<Model *> mm = transformer->detachAdditionalOutputModels();
            toNotifyOfMore[m_handlers[transformer]] = mm;
        } else {
            toNotifyOfNoMore.push_back(m_handlers[transformer]);
        }
        m_handlers.erase(transformer);
    }

    m_mutex.unlock();

    // These calls have to be made without the mutex held, as they may
    // ultimately call back on us (e.g. we have one baroque situation
    // where this could trigger a command to create a layer, which
    // triggers the command history to clip the stack, which deletes a
    // spare old model, which calls back on our modelAboutToBeDeleted)
    
    for (const auto &i: toNotifyOfMore) {
        i.first->moreModelsAvailable(i.second);
    }
    for (AdditionalModelHandler *handler: toNotifyOfNoMore) {
        handler->noMoreModelsAvailable();
    }
    
    if (transformer->isAbandoned()) {
        if (transformer->getMessage() != "") {
            emit transformFailed("", transformer->getMessage());
        }
    }
    
    transformer->wait(); // unnecessary but reassuring
    delete transformer;
}

void
ModelTransformerFactory::modelAboutToBeDeleted(Model *m)
{
    TransformerSet affected;

    {
        QMutexLocker locker(&m_mutex);
    
        for (TransformerSet::iterator i = m_runningTransformers.begin();
             i != m_runningTransformers.end(); ++i) {

            ModelTransformer *t = *i;

            if (t->getInputModel() == m) {
                affected.insert(t);
            } else {
                vector<Model *> mm = t->getOutputModels();
                for (int i = 0; i < (int)mm.size(); ++i) {
                    if (mm[i] == m) affected.insert(t);
                }
            }
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

bool
ModelTransformerFactory::haveRunningTransformers() const
{
    QMutexLocker locker(&m_mutex);
    
    return (!m_runningTransformers.empty());
}
