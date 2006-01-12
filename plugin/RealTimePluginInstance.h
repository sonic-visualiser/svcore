/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2005 Chris Cannam.
*/

#ifndef _REALTIME_PLUGIN_INSTANCE_H_
#define _REALTIME_PLUGIN_INSTANCE_H_

#include <QString>
#include <QStringList>
#include <vector>

#include "base/RealTime.h"

class RealTimePluginFactory;
	
/**
 * RealTimePluginInstance is a very trivial interface that an audio
 * process can use to refer to an instance of a plugin without needing
 * to know what type of plugin it is.
 *
 * The audio code calls run() on an instance that has been passed to
 * it, and assumes that the passing code has already initialised the
 * plugin, connected its inputs and outputs and so on, and that there
 * is an understanding in place about the sizes of the buffers in use
 * by the plugin.  All of this depends on the subclass implementation.
 */

// These names are taken from LADSPA, but the values are not
// guaranteed to match

namespace PortType { // ORable
    static const int Input   = 1;
    static const int Output  = 2;
    static const int Control = 4;
    static const int Audio   = 8;
}

namespace PortHint { // ORable
    static const int NoHint  = 0;
    static const int Toggled = 1;
    static const int Integer = 2;
    static const int Logarithmic = 4;
    static const int SampleRate = 8;
}

class RealTimePluginInstance
{
public:
    typedef float sample_t;

    virtual ~RealTimePluginInstance();

    virtual bool isOK() const = 0;

    virtual QString getIdentifier() const = 0;

    /**
     * Run for one block, starting at the given time.  The start time
     * may be of interest to synths etc that may have queued events
     * waiting.  Other plugins can ignore it.
     */
    virtual void run(const RealTime &blockStartTime) = 0;
    
    virtual size_t getBufferSize() const = 0;

    virtual size_t getAudioInputCount() const = 0;
    virtual size_t getAudioOutputCount() const = 0;

    virtual sample_t **getAudioInputBuffers() = 0;
    virtual sample_t **getAudioOutputBuffers() = 0;

    virtual QStringList getPrograms() const { return QStringList(); }
    virtual QString getCurrentProgram() const { return QString(); }
    virtual QString getProgram(int /* bank */, int /* program */) const { return QString(); }
    virtual unsigned long getProgram(QString /* name */) const { return 0; } // bank << 16 + program
    virtual void selectProgram(QString) { }

    virtual unsigned int getParameterCount() const = 0;
    virtual void setParameterValue(unsigned int parameter, float value) = 0;
    virtual float getParameterValue(unsigned int parameter) const = 0;
    virtual float getParameterDefault(unsigned int parameter) const = 0;

    virtual QString configure(QString /* key */, QString /* value */) { return QString(); }

    virtual void sendEvent(const RealTime & /* eventTime */,
			   const void * /* event */) { }

    virtual bool isBypassed() const = 0;
    virtual void setBypassed(bool value) = 0;

    // This should be called after setup, but while not actually playing.
    virtual size_t getLatency() = 0;

    virtual void silence() = 0;
    virtual void discardEvents() { }
    virtual void setIdealChannelCount(size_t channels) = 0; // must also silence(); may also re-instantiate

    void setFactory(RealTimePluginFactory *f) { m_factory = f; } // ew

protected:
    RealTimePluginInstance(RealTimePluginFactory *factory, QString identifier) :
	m_factory(factory), m_identifier(identifier) { }

    RealTimePluginFactory *m_factory;
    QString m_identifier;

    friend class PluginFactory;
};


#endif
