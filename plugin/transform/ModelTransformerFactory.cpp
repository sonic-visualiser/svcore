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

#include "plugin/FeatureExtractionPluginFactory.h"
#include "plugin/RealTimePluginFactory.h"
#include "plugin/PluginXml.h"

#include "widgets/PluginParameterDialog.h"

#include "data/model/DenseTimeValueModel.h"

#include "vamp-sdk/PluginHostAdapter.h"

#include "audioio/AudioCallbackPlaySource.h" //!!! shouldn't include here

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

bool
ModelTransformerFactory::getChannelRange(TransformId identifier, Vamp::PluginBase *plugin,
                                         int &minChannels, int &maxChannels)
{
    Vamp::Plugin *vp = 0;
    if ((vp = dynamic_cast<Vamp::Plugin *>(plugin)) ||
        (vp = dynamic_cast<Vamp::PluginHostAdapter *>(plugin))) {
        minChannels = vp->getMinChannelCount();
        maxChannels = vp->getMaxChannelCount();
        return true;
    } else {
        return TransformFactory::getInstance()->
            getTransformChannelRange(identifier, minChannels, maxChannels);
    }
}

Model *
ModelTransformerFactory::getConfigurationForTransformer(TransformId identifier,
                                               const std::vector<Model *> &candidateInputModels,
                                               PluginTransformer::ExecutionContext &context,
                                               QString &configurationXml,
                                               AudioCallbackPlaySource *source,
                                               size_t startFrame,
                                               size_t duration)
{
    if (candidateInputModels.empty()) return 0;

    //!!! This will need revision -- we'll have to have a callback
    //from the dialog for when the candidate input model is changed,
    //as we'll need to reinitialise the channel settings in the dialog
    Model *inputModel = candidateInputModels[0]; //!!! for now
    QStringList candidateModelNames;
    std::map<QString, Model *> modelMap;
    for (size_t i = 0; i < candidateInputModels.size(); ++i) {
        QString modelName = candidateInputModels[i]->objectName();
        QString origModelName = modelName;
        int dupcount = 1;
        while (modelMap.find(modelName) != modelMap.end()) {
            modelName = tr("%1 <%2>").arg(origModelName).arg(++dupcount);
        }
        modelMap[modelName] = candidateInputModels[i];
        candidateModelNames.push_back(modelName);
    }

    QString id = identifier.section(':', 0, 2);
    QString output = identifier.section(':', 3);
    QString outputLabel = "";
    QString outputDescription = "";
    
    bool ok = false;
    configurationXml = m_lastConfigurations[identifier];

//    std::cerr << "last configuration: " << configurationXml.toStdString() << std::endl;

    Vamp::PluginBase *plugin = 0;

    bool frequency = false;
    bool effect = false;
    bool generator = false;

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        std::cerr << "getConfigurationForTransformer: instantiating Vamp plugin" << std::endl;

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel->getSampleRate());

        if (vp) {

            plugin = vp;
            frequency = (vp->getInputDomain() == Vamp::Plugin::FrequencyDomain);

            std::vector<Vamp::Plugin::OutputDescriptor> od =
                vp->getOutputDescriptors();
            if (od.size() > 1) {
                for (size_t i = 0; i < od.size(); ++i) {
                    if (od[i].identifier == output.toStdString()) {
                        outputLabel = od[i].name.c_str();
                        outputDescription = od[i].description.c_str();
                        break;
                    }
                }
            }
        }

    } else if (RealTimePluginFactory::instanceFor(id)) {

        RealTimePluginFactory *factory = RealTimePluginFactory::instanceFor(id);
        const RealTimePluginDescriptor *desc = factory->getPluginDescriptor(id);

        if (desc->audioInputPortCount > 0 && 
            desc->audioOutputPortCount > 0 &&
            !desc->isSynth) {
            effect = true;
        }

        if (desc->audioInputPortCount == 0) {
            generator = true;
        }

        if (output != "A") {
            int outputNo = output.toInt();
            if (outputNo >= 0 && outputNo < int(desc->controlOutputPortCount)) {
                outputLabel = desc->controlOutputPortNames[outputNo].c_str();
            }
        }

        size_t sampleRate = inputModel->getSampleRate();
        size_t blockSize = 1024;
        size_t channels = 1;
        if (effect && source) {
            sampleRate = source->getTargetSampleRate();
            blockSize = source->getTargetBlockSize();
            channels = source->getTargetChannelCount();
        }

        RealTimePluginInstance *rtp = factory->instantiatePlugin
            (id, 0, 0, sampleRate, blockSize, channels);

        plugin = rtp;

        if (effect && source && rtp) {
            source->setAuditioningPlugin(rtp);
        }
    }

    if (plugin) {

        context = PluginTransformer::ExecutionContext(context.channel, plugin);

        if (configurationXml != "") {
            PluginXml(plugin).setParametersFromXml(configurationXml);
        }

        int sourceChannels = 1;
        if (dynamic_cast<DenseTimeValueModel *>(inputModel)) {
            sourceChannels = dynamic_cast<DenseTimeValueModel *>(inputModel)
                ->getChannelCount();
        }

        int minChannels = 1, maxChannels = sourceChannels;
        getChannelRange(identifier, plugin, minChannels, maxChannels);

        int targetChannels = sourceChannels;
        if (!effect) {
            if (sourceChannels < minChannels) targetChannels = minChannels;
            if (sourceChannels > maxChannels) targetChannels = maxChannels;
        }

        int defaultChannel = context.channel;

        PluginParameterDialog *dialog = new PluginParameterDialog(plugin);

        if (candidateModelNames.size() > 1 && !generator) {
            dialog->setCandidateInputModels(candidateModelNames);
        }

        if (startFrame != 0 || duration != 0) {
            dialog->setShowSelectionOnlyOption(true);
        }

        if (targetChannels > 0) {
            dialog->setChannelArrangement(sourceChannels, targetChannels,
                                          defaultChannel);
        }
        
        dialog->setOutputLabel(outputLabel, outputDescription);
        
        dialog->setShowProcessingOptions(true, frequency);

        if (dialog->exec() == QDialog::Accepted) {
            ok = true;
        }

        QString selectedInput = dialog->getInputModel();
        if (selectedInput != "") {
            if (modelMap.find(selectedInput) != modelMap.end()) {
                inputModel = modelMap[selectedInput];
                std::cerr << "Found selected input \"" << selectedInput.toStdString() << "\" in model map, result is " << inputModel << std::endl;
            } else {
                std::cerr << "Failed to find selected input \"" << selectedInput.toStdString() << "\" in model map" << std::endl;
            }
        } else {
            std::cerr << "Selected input empty: \"" << selectedInput.toStdString() << "\"" << std::endl;
        }

        configurationXml = PluginXml(plugin).toXmlString();
        context.channel = dialog->getChannel();
        
        if (startFrame != 0 || duration != 0) {
            if (dialog->getSelectionOnly()) {
                context.startFrame = startFrame;
                context.duration = duration;
            }
        }

        dialog->getProcessingParameters(context.stepSize,
                                        context.blockSize,
                                        context.windowType);

        context.makeConsistentWithPlugin(plugin);

        delete dialog;

        if (effect && source) {
            source->setAuditioningPlugin(0); // will delete our plugin
        } else {
            delete plugin;
        }
    }

    if (ok) m_lastConfigurations[identifier] = configurationXml;

    return ok ? inputModel : 0;
}

PluginTransformer::ExecutionContext
ModelTransformerFactory::getDefaultContextForTransformer(TransformId identifier,
                                                Model *inputModel)
{
    PluginTransformer::ExecutionContext context(-1);

    QString id = identifier.section(':', 0, 2);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel ? inputModel->getSampleRate() : 48000);

        if (vp) {
            context = PluginTransformer::ExecutionContext(-1, vp);
            delete vp;
        }
    }

    return context;
}

ModelTransformer *
ModelTransformerFactory::createTransformer(TransformId identifier, Model *inputModel,
                                  const PluginTransformer::ExecutionContext &context,
                                  QString configurationXml)
{
    ModelTransformer *transformer = 0;

    QString id = identifier.section(':', 0, 2);
    QString output = identifier.section(':', 3);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {
        transformer = new FeatureExtractionModelTransformer
            (inputModel, id, context, configurationXml, output);
    } else if (RealTimePluginFactory::instanceFor(id)) {
        transformer = new RealTimeEffectModelTransformer
            (inputModel, id, context, configurationXml,
             TransformFactory::getInstance()->getTransformUnits(identifier),
             output == "A" ? -1 : output.toInt());
    } else {
        std::cerr << "ModelTransformerFactory::createTransformer: Unknown transform \""
                  << identifier.toStdString() << "\"" << std::endl;
        return transformer;
    }

    if (transformer) transformer->setObjectName(identifier);
    return transformer;
}

Model *
ModelTransformerFactory::transform(TransformId identifier, Model *inputModel,
                            const PluginTransformer::ExecutionContext &context,
                            QString configurationXml)
{
    ModelTransformer *t = createTransformer(identifier, inputModel, context,
                                            configurationXml);

    if (!t) return 0;

    connect(t, SIGNAL(finished()), this, SLOT(transformerFinished()));

    m_runningTransformers.insert(t);

    t->start();
    Model *model = t->detachOutputModel();

    if (model) {
        QString imn = inputModel->objectName();
        QString trn =
            TransformFactory::getInstance()->getTransformFriendlyName
            (identifier);
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

    return model;
}

void
ModelTransformerFactory::transformerFinished()
{
    QObject *s = sender();
    ModelTransformer *transformer = dynamic_cast<ModelTransformer *>(s);
    
    std::cerr << "ModelTransformerFactory::transformerFinished(" << transformer << ")" << std::endl;

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
