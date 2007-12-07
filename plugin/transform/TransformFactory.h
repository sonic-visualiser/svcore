/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _TRANSFORM_FACTORY_H_
#define _TRANSFORM_FACTORY_H_

#include "TransformDescription.h"

#include <vamp-sdk/Plugin.h>

#include <map>
#include <set>

class TransformFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformFactory();

    static TransformFactory *getInstance();

    TransformList getAllTransformDescriptions();
    TransformDescription getTransformDescription(TransformId id);

    std::vector<QString> getAllTransformTypes();
    std::vector<QString> getTransformCategories(QString transformType);
    std::vector<QString> getTransformMakers(QString transformType);

    /**
     * Return true if the given transform is known.
     */
    bool haveTransform(TransformId identifier);

    /**
     * A single transform ID can lead to many possible Transforms,
     * with different parameters and execution context settings.
     * Return the default one for the given transform.
     */
    Transform getDefaultTransformFor(TransformId identifier, size_t rate = 0);

    /**
     * Full name of a transform, suitable for putting on a menu.
     */
    QString getTransformName(TransformId identifier);

    /**
     * Brief but friendly name of a transform, suitable for use
     * as the name of the output layer.
     */
    QString getTransformFriendlyName(TransformId identifier);

    QString getTransformUnits(TransformId identifier);

    Vamp::Plugin::InputDomain getTransformInputDomain(TransformId identifier);

    /**
     * Return true if the transform has any configurable parameters,
     * i.e. if getConfigurationForTransform can ever return a non-trivial
     * (not equivalent to empty) configuration string.
     */
    bool isTransformConfigurable(TransformId identifier);

    /**
     * If the transform has a prescribed number or range of channel
     * inputs, return true and set minChannels and maxChannels to the
     * minimum and maximum number of channel inputs the transform can
     * accept.  Return false if it doesn't care.
     */
    bool getTransformChannelRange(TransformId identifier,
                                  int &minChannels, int &maxChannels);

    /**
     * Set the plugin parameters, program and configuration strings on
     * the given Transform object from the given plugin instance.
     * Note that no check is made whether the plugin is actually the
     * "correct" one for the transform.
     */
    void setParametersFromPlugin(Transform &transform, Vamp::PluginBase *plugin);

    /**
     * Set the parameters, program and configuration strings on the
     * given plugin from the given Transform object.
     */
    void setPluginParameters(const Transform &transform, Vamp::PluginBase *plugin);
    
    /**
     * If the given Transform object has no processing step and block
     * sizes set, set them to appropriate defaults for the given
     * plugin.
     */
    void makeContextConsistentWithPlugin(Transform &transform, Vamp::PluginBase *plugin); 

    /**
     * Retrieve a <plugin ... /> XML fragment that describes the
     * plugin parameters, program and configuration data for the given
     * transform.
     *
     * This function is provided for backward compatibility only.  Use
     * Transform::toXml where compatibility with PluginXml
     * descriptions of transforms is not required.
     */
    QString getPluginConfigurationXml(const Transform &transform);

    /**
     * Set the plugin parameters, program and configuration strings on
     * the given Transform object from the given <plugin ... /> XML
     * fragment.
     *
     * This function is provided for backward compatibility only.  Use
     * Transform(QString) where compatibility with PluginXml
     * descriptions of transforms is not required.
     */
    void setParametersFromPluginConfigurationXml(Transform &transform,
                                                 QString xml);

protected:
    typedef std::map<TransformId, TransformDescription> TransformDescriptionMap;
    TransformDescriptionMap m_transforms;

    void populateTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    Vamp::PluginBase *instantiatePluginFor(TransformId id, size_t rate);
    Vamp::Plugin *downcastVampPlugin(Vamp::PluginBase *);

    static TransformFactory *m_instance;
};


#endif
