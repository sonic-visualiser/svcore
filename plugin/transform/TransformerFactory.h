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

#ifndef _TRANSFORMER_FACTORY_H_
#define _TRANSFORMER_FACTORY_H_

#include "TransformDescription.h"

#include "Transformer.h"
#include "PluginTransformer.h"

#include <map>
#include <set>

namespace Vamp { class PluginBase; }

class AudioCallbackPlaySource;

//!!! split into TransformFactory (information about available
// transforms, create default Transform for each transform ID etc) and
// TransformerFactory (create Transformers to apply transforms)

class TransformerFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformerFactory();

    static TransformerFactory *getInstance();

    TransformList getAllTransforms();

    std::vector<QString> getAllTransformerTypes();

    std::vector<QString> getTransformerCategories(QString transformType);
    std::vector<QString> getTransformerMakers(QString transformType);

    /**
     * Get a configuration XML string for the given transform (by
     * asking the user, most likely).  Returns the selected input
     * model if the transform is acceptable, 0 if the operation should
     * be cancelled.  Audio callback play source may be used to
     * audition effects plugins, if provided.
     */
    Model *getConfigurationForTransformer(TransformId identifier,
                                        const std::vector<Model *> &candidateInputModels,
                                        PluginTransformer::ExecutionContext &context,
                                        QString &configurationXml,
                                        AudioCallbackPlaySource *source = 0,
                                        size_t startFrame = 0,
                                        size_t duration = 0);

    /**
     * Get the default execution context for the given transform
     * and input model (if known).
     */
    PluginTransformer::ExecutionContext getDefaultContextForTransformer(TransformId identifier,
                                                                    Model *inputModel = 0);

    /**
     * Return the output model resulting from applying the named
     * transform to the given input model.  The transform may still be
     * working in the background when the model is returned; check the
     * output model's isReady completion status for more details.
     *
     * If the transform is unknown or the input model is not an
     * appropriate type for the given transform, or if some other
     * problem occurs, return 0.
     * 
     * The returned model is owned by the caller and must be deleted
     * when no longer needed.
     */
    Model *transform(TransformId identifier, Model *inputModel,
                     const PluginTransformer::ExecutionContext &context,
                     QString configurationXml = "");

    /**
     * Return true if the given transform is known.
     */
    bool haveTransformer(TransformId identifier);

    /**
     * Full name of a transform, suitable for putting on a menu.
     */
    QString getTransformerName(TransformId identifier);

    /**
     * Brief but friendly name of a transform, suitable for use
     * as the name of the output layer.
     */
    QString getTransformerFriendlyName(TransformId identifier);

    QString getTransformerUnits(TransformId identifier);

    /**
     * Return true if the transform has any configurable parameters,
     * i.e. if getConfigurationForTransformer can ever return a non-trivial
     * (not equivalent to empty) configuration string.
     */
    bool isTransformerConfigurable(TransformId identifier);

    /**
     * If the transform has a prescribed number or range of channel
     * inputs, return true and set minChannels and maxChannels to the
     * minimum and maximum number of channel inputs the transform can
     * accept.  Return false if it doesn't care.
     */
    bool getTransformerChannelRange(TransformId identifier,
                                  int &minChannels, int &maxChannels);
	
protected slots:
    void transformFinished();

    void modelAboutToBeDeleted(Model *);

protected:
    Transformer *createTransformer(TransformId identifier, Model *inputModel,
                               const PluginTransformer::ExecutionContext &context,
                               QString configurationXml);

    struct TransformIdent
    {
        TransformId identifier;
        QString configurationXml;
    };

    typedef std::map<TransformId, QString> TransformerConfigurationMap;
    TransformerConfigurationMap m_lastConfigurations;

    typedef std::map<TransformId, TransformDescription> TransformDescriptionMap;
    TransformDescriptionMap m_transforms;

    typedef std::set<Transformer *> TransformerSet;
    TransformerSet m_runningTransformers;

    void populateTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    bool getChannelRange(TransformId identifier,
                         Vamp::PluginBase *plugin, int &min, int &max);

    static TransformerFactory *m_instance;
};


#endif
