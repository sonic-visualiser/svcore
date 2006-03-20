/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _SAMPLE_PLAYER_H_
#define _SAMPLE_PLAYER_H_

#define DSSI_API_LEVEL 2

#include <ladspa.h>
#include <dssi.h>
#include <seq_event.h>

#include <QMutex>
#include <QString>
#include <vector>

class SamplePlayer
{
public:
    static const DSSI_Descriptor *getDescriptor(unsigned long index);

private:
    SamplePlayer(int sampleRate);
    ~SamplePlayer();

    enum {
	OutputPort    = 0,
	RetunePort    = 1,
	BasePitchPort = 2,
	SustainPort   = 3,
	ReleasePort   = 4,
	PortCount     = 5
    };

    enum {
	Polyphony = 128
    };

    static const char *const portNames[PortCount];
    static const LADSPA_PortDescriptor ports[PortCount];
    static const LADSPA_PortRangeHint hints[PortCount];
    static const LADSPA_Properties properties;
    static const LADSPA_Descriptor ladspaDescriptor;
    static const DSSI_Descriptor dssiDescriptor;
    static const DSSI_Host_Descriptor *hostDescriptor;

    static LADSPA_Handle instantiate(const LADSPA_Descriptor *, unsigned long);
    static void connectPort(LADSPA_Handle, unsigned long, LADSPA_Data *);
    static void activate(LADSPA_Handle);
    static void run(LADSPA_Handle, unsigned long);
    static void deactivate(LADSPA_Handle);
    static void cleanup(LADSPA_Handle);
    static const DSSI_Program_Descriptor *getProgram(LADSPA_Handle, unsigned long);
    static void selectProgram(LADSPA_Handle, unsigned long, unsigned long);
    static int getMidiController(LADSPA_Handle, unsigned long);
    static void runSynth(LADSPA_Handle, unsigned long,
			 snd_seq_event_t *, unsigned long);
    static void receiveHostDescriptor(const DSSI_Host_Descriptor *descriptor);
    static void workThreadCallback(LADSPA_Handle);

    void searchSamples();
    void loadSampleData(QString path);
    void runImpl(unsigned long, snd_seq_event_t *, unsigned long);
    void addSample(int, unsigned long, unsigned long);

    float *m_output;
    float *m_retune;
    float *m_basePitch;
    float *m_sustain;
    float *m_release;

    float *m_sampleData;
    size_t m_sampleCount;
    int m_sampleRate;

    long m_ons[Polyphony];
    long m_offs[Polyphony];
    int m_velocities[Polyphony];
    long m_sampleNo;

    QString m_program;
    std::vector<std::pair<QString, QString> > m_samples; // program name, path
    bool m_sampleSearchComplete;
    int m_pendingProgramChange;

    QMutex m_mutex;
};


#endif
