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

#ifndef SV_TRANSFORM_FACTORY_H
#define SV_TRANSFORM_FACTORY_H

#include "TransformDescription.h"

#include "base/TextMatcher.h"

#include <vamp-hostsdk/Plugin.h>

#include <QObject>
#include <QStringList>
#include <QThread>
#include <QMutex>

#include <map>
#include <set>
#include <memory>
#include <atomic>

/**
 * TransformFactory catalogues both "installed" transforms (based on
 * plugins available to load) and "uninstalled" ones (based on RDF or
 * other metadata, possibly found online).
 *
 * You can retrieve a list of properties of either of these, and given
 * a transform ID, the TransformFactory can tell you whether the
 * transform is installed, not yet installed, or unknown. You can also
 * search both transform catalogues by keyword.
 *
 * TransformFactory can also construct the plugin for a transform,
 * transfer parameter bundles back and forth from plugin to transform,
 * and return various information about a transform. (These
 * capabilities are for installed transforms only. If a method name
 * contains "Transform" without qualification, it refers only to
 * installed ones.)
 *
 * The process of populating the transform catalogues can be handled
 * synchronously or asynchronously. There are two optional population
 * threads, for installed and uninstalled transforms, which can be
 * started with startPopulatingInstalledTransforms() and
 * startPopulatingUninstalledTransforms() respectively. Signals
 * installedTransformsPopulated and uninstalledTransformsPopulated are
 * emitted by each when they complete.
 *
 * If a function is called which depends on either installed or
 * uninstalled transforms having been populated, at a time when
 * population is not complete, it will first populate them, either by
 * waiting for the population thread or by populating them
 * synchronously. The exceptions are the search functions, which do
 * not wait for uninstalled transforms to finish populating - they may
 * just return incomplete data in that case.
 */
class TransformFactory : public QObject
{
    Q_OBJECT

public:
    static TransformFactory *getInstance();
    static void deleteInstance(); // only when exiting

public slots:
    /**
     * Start populating the installed transforms in a background
     * thread. Any call that depends on installed transform
     * information will wait for this thread to complete before it
     * acts. Calling this is optional; if you don't call it, installed
     * transforms will be populated the first time information about
     * them is requested.
     */
    void startPopulatingInstalledTransforms();

    /**
     * Start populating metadata about uninstalled transforms in a
     * background thread. Any call that depends specifically on
     * uninstalled transform information will wait for this thread to
     * complete before it acts.
     *
     * Note that the first thing the thread does is sleep until the
     * installed transforms have finished populating - if you don't
     * populate those, this will do nothing!
     */
    void startPopulatingUninstalledTransforms();

public:
    /**
     * Return true if the installed transforms have been populated,
     * i.e. if a call to something like haveTransform() will return
     * without having to do the work of scanning plugins. If this
     * returns false, then population may be in progress or may have
     * not been started at all.
     */
    bool havePopulatedInstalledTransforms();
    
    /**
     * Return true if the uninstalled transforms have finished being
     * populated, i.e. if a call to something like
     * getUninstalledTransformDescriptions() will return without
     * having to do the work of retrieving metadata. If this returns
     * false, then population may be in progress or may have not been
     * started at all.
     */
    bool havePopulatedUninstalledTransforms();
    
    TransformList getInstalledTransformDescriptions();
    TransformDescription getInstalledTransformDescription(TransformId id);
    bool haveAnyInstalledTransforms();

    TransformList getUninstalledTransformDescriptions();
    TransformDescription getUninstalledTransformDescription(TransformId id);
    bool haveAnyUninstalledTransforms(bool waitForCheckToComplete = false);
    
    typedef enum {
        TransformUnknown,
        TransformInstalled,
        TransformNotInstalled
    } TransformInstallStatus;

    TransformInstallStatus getTransformInstallStatus(TransformId id);

    typedef std::map<TransformId, TextMatcher::Match> SearchResults;
    SearchResults search(QString keyword);
    SearchResults search(QStringList keywords);

    // All of the remaining functions act upon installed transforms
    // only.
    
    std::vector<TransformDescription::Type> getTransformTypes();
    std::vector<QString> getTransformCategories(TransformDescription::Type);
    std::vector<QString> getTransformMakers(TransformDescription::Type);
    QString getTransformTypeName(TransformDescription::Type) const;

    /**
     * Return true if the given transform is installed. If this
     * returns false, none of the following methods will return
     * anything useful.
     */
    bool haveTransform(TransformId identifier);

    /**
     * A single transform ID can lead to many possible Transforms,
     * with different parameters and execution context settings.
     * Return the default one for the given transform.
     */
    Transform getDefaultTransformFor(TransformId identifier,
                                     sv_samplerate_t rate = 0);

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

    Provider getTransformProvider(TransformId identifier);

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
     * Load an appropriate plugin for the given transform and set the
     * parameters, program and configuration strings on that plugin
     * from the Transform object.
     *
     * Note that this requires that the transform has a meaningful
     * sample rate set, as that is used as the rate for the plugin.  A
     * Transform can legitimately have rate set at zero (= "use the
     * rate of the input source"), so the caller will need to test for
     * this case.
     *
     * Returns the plugin thus loaded.  This will be a
     * Vamp::PluginBase, but not necessarily a Vamp::Plugin (only if
     * the transform was a feature-extraction type -- call
     * downcastVampPlugin if you only want Vamp::Plugins).  Returns
     * nullptr if no suitable plugin was available.
     *
     * (NB at the time of writing this is only used in Sonic
     * Annotator, which does not use model transform objects. In SV
     * the model transformers load their own plugins.)
     */
    std::shared_ptr<Vamp::PluginBase> instantiatePluginFor(const Transform &transform);

    /**
     * Set the plugin parameters, program and configuration strings on
     * the given Transform object from the given plugin instance.
     * Note that no check is made whether the plugin is actually the
     * "correct" one for the transform.
     */
    void setParametersFromPlugin(Transform &transform, std::shared_ptr<Vamp::PluginBase> plugin);

    /**
     * Set the parameters, program and configuration strings on the
     * given plugin from the given Transform object.
     */
    void setPluginParameters(const Transform &transform, std::shared_ptr<Vamp::PluginBase> plugin);
    
    /**
     * If the given Transform object has no processing step and block
     * sizes set, set them to appropriate defaults for the given
     * plugin.
     */
    void makeContextConsistentWithPlugin(Transform &transform, std::shared_ptr<Vamp::PluginBase> plugin); 

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
    
    QString getStartupFailureReport() const {
        return m_errorString;
    }

signals:
    void installedTransformsPopulated();
    void uninstalledTransformsPopulated();
    
protected:
    typedef std::map<TransformId, TransformDescription> TransformDescriptionMap;

    TransformDescriptionMap m_transforms;
    std::atomic<bool> m_installedTransformsPopulated;

    TransformDescriptionMap m_uninstalledTransforms;
    std::atomic<bool> m_uninstalledTransformsPopulated;

    QString m_errorString;
    
    void populateInstalledTransforms();
    void populateUninstalledTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    std::shared_ptr<Vamp::PluginBase> instantiateDefaultPluginFor(TransformId id, sv_samplerate_t rate);
    QMutex m_installedTransformsMutex;
    QMutex m_uninstalledTransformsMutex;

    class InstalledTransformsPopulateThread : public QThread
    {
    public:
        InstalledTransformsPopulateThread(TransformFactory *factory) :
            m_factory(factory) {
        }
        void run() override;
        TransformFactory *m_factory;
    };
    InstalledTransformsPopulateThread *m_installedThread;

    class UninstalledTransformsPopulateThread : public QThread
    {
    public:
        UninstalledTransformsPopulateThread(TransformFactory *factory) :
            m_factory(factory) {
        }
        void run() override;
        TransformFactory *m_factory;
    };
    UninstalledTransformsPopulateThread *m_uninstalledThread;
    
    bool m_exiting;
    bool m_populatingSlowly;

    SearchResults searchUnadjusted(QStringList keywords);

    static TransformFactory *m_instance;

    TransformFactory();
    virtual ~TransformFactory();
};

#endif
