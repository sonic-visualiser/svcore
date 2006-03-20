/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam and Richard Bown.
*/

#ifndef _LADSPAPLUGININSTANCE_H_
#define _LADSPAPLUGININSTANCE_H_

#include <vector>
#include <set>
#include <QString>

#include "api/ladspa.h"
#include "RealTimePluginInstance.h"

// LADSPA plugin instance.  LADSPA is a variable block size API, but
// for one reason and another it's more convenient to use a fixed
// block size in this wrapper.
//
class LADSPAPluginInstance : public RealTimePluginInstance
{
public:
    virtual ~LADSPAPluginInstance();

    virtual bool isOK() const { return m_instanceHandles.size() != 0; }

    int getClientId() const { return m_client; }
    virtual QString getIdentifier() const { return m_identifier; }
    int getPosition() const { return m_position; }

    virtual std::string getName() const;
    virtual std::string getDescription() const;
    virtual std::string getMaker() const;
    virtual int getPluginVersion() const;
    virtual std::string getCopyright() const;

    virtual void run(const RealTime &rt);

    virtual unsigned int getParameterCount() const;
    virtual void setParameterValue(unsigned int parameter, float value);
    virtual float getParameterValue(unsigned int parameter) const;
    virtual float getParameterDefault(unsigned int parameter) const;
    
    virtual ParameterList getParameterDescriptors() const;
    virtual float getParameter(std::string) const;
    virtual void setParameter(std::string, float);

    virtual size_t getBufferSize() const { return m_blockSize; }
    virtual size_t getAudioInputCount() const { return m_instanceCount * m_audioPortsIn.size(); }
    virtual size_t getAudioOutputCount() const { return m_instanceCount * m_audioPortsOut.size(); }
    virtual sample_t **getAudioInputBuffers() { return m_inputBuffers; }
    virtual sample_t **getAudioOutputBuffers() { return m_outputBuffers; }

    virtual bool isBypassed() const { return m_bypassed; }
    virtual void setBypassed(bool bypassed) { m_bypassed = bypassed; }

    virtual size_t getLatency();

    virtual void silence();
    virtual void setIdealChannelCount(size_t channels); // may re-instantiate

protected:
    // To be constructed only by LADSPAPluginFactory
    friend class LADSPAPluginFactory;

    // Constructor that creates the buffers internally
    // 
    LADSPAPluginInstance(RealTimePluginFactory *factory,
			 int client,
			 QString identifier,
                         int position,
			 unsigned long sampleRate,
			 size_t blockSize,
			 int idealChannelCount,
                         const LADSPA_Descriptor* descriptor);

    void init(int idealChannelCount = 0);
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();
    
    int                        m_client;
    int                        m_position;
    std::vector<LADSPA_Handle> m_instanceHandles;
    size_t                     m_instanceCount;
    const LADSPA_Descriptor   *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsIn;
    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsOut;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    size_t                    m_sampleRate;
    float                    *m_latencyPort;
    bool                      m_run;
    
    bool                      m_bypassed;
};

#endif // _LADSPAPLUGININSTANCE_H_

