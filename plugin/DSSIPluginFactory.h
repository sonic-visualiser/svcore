/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#ifndef _DSSI_PLUGIN_FACTORY_H_
#define _DSSI_PLUGIN_FACTORY_H_

#define DSSI_API_LEVEL 2

#include "LADSPAPluginFactory.h"
#include "api/dssi.h"

#include <QMutex>

class DSSIPluginInstance;

class DSSIPluginFactory : public LADSPAPluginFactory
{
public:
    virtual ~DSSIPluginFactory();

    virtual void enumeratePlugins(std::vector<QString> &list);

    virtual RealTimePluginInstance *instantiatePlugin(QString identifier,
						      int clientId,
						      int position,
						      unsigned int sampleRate,
						      unsigned int blockSize,
						      unsigned int channels);

protected:
    DSSIPluginFactory();
    friend class RealTimePluginFactory;

    virtual std::vector<QString> getPluginPath();

#ifdef HAVE_LIBLRDF
    virtual std::vector<QString> getLRDFPath(QString &baseUri);
#endif

    virtual void discoverPlugins(QString soName);

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);
    virtual const DSSI_Descriptor *getDSSIDescriptor(QString identifier);

    DSSI_Host_Descriptor m_hostDescriptor;
};

#endif

