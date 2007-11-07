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

#include <map>
#include <set>

namespace Vamp { class PluginBase; }

class TransformFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformFactory();

    static TransformFactory *getInstance();

    TransformList getAllTransforms();

    std::vector<QString> getAllTransformTypes();

    std::vector<QString> getTransformCategories(QString transformType);
    std::vector<QString> getTransformMakers(QString transformType);

    /**
     * Return true if the given transform is known.
     */
    bool haveTransform(TransformId identifier);

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
     * If the given Transform object has no processing step and block
     * sizes set, set them to appropriate defaults for the given
     * plugin.
     */
    void makeContextConsistentWithPlugin(Transform &transform, Vamp::PluginBase *plugin); 

    /**
     * A single transform ID can lead to many possible Transforms,
     * with different parameters and execution context settings.
     * Return the default one for the given transform.
     */
    Transform getDefaultTransformFor(TransformId identifier, size_t rate = 0);

protected:
    typedef std::map<TransformId, TransformDescription> TransformDescriptionMap;
    TransformDescriptionMap m_transforms;

    void populateTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    static TransformFactory *m_instance;
};


#endif
