/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
    Based on trivial_sampler from the DSSI distribution
    (by Chris Cannam, public domain).
*/

#include "SamplePlayer.h"
#include "system/System.h"

#include "../api/dssi.h"

#include <cmath>
#include <cstdlib>

#include <QMutexLocker>
#include <QDir>
#include <QFileInfo>

#include <bqaudiostream/AudioReadStreamFactory.h>
#include <bqaudiostream/AudioReadStream.h>

#include <bqresample/Resampler.h>

#include <iostream>
#include <vector>

using std::vector;

//#define DEBUG_SAMPLE_PLAYER 1

const char *const
SamplePlayer::portNames[PortCount] =
{
    "Output",
    "Tuned (on/off)",
    "Base Pitch (MIDI)",
    "Tuning of A (Hz)",
    "Sustain (on/off)",
    "Release time (s)"
};

const LADSPA_PortDescriptor 
SamplePlayer::ports[PortCount] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL
};

const LADSPA_PortRangeHint 
SamplePlayer::hints[PortCount] =
{
    { 0, 0, 0 },
    { LADSPA_HINT_DEFAULT_MAXIMUM | LADSPA_HINT_INTEGER |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0, 1 },
    { LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_INTEGER |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0, 120 },
    { LADSPA_HINT_DEFAULT_440 | LADSPA_HINT_LOGARITHMIC |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 400, 499 },
    { LADSPA_HINT_DEFAULT_MINIMUM | LADSPA_HINT_INTEGER |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0, 1 },
    { LADSPA_HINT_DEFAULT_MINIMUM | LADSPA_HINT_LOGARITHMIC |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0.001f, 2.0f }
};

const LADSPA_Properties
SamplePlayer::properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

const LADSPA_Descriptor 
SamplePlayer::ladspaDescriptor =
{
    0, // "Unique" ID
    "sample_player", // Label
    properties,
    "Library Sample Player", // Name
    "Chris Cannam", // Maker
    "GPL", // Copyright
    PortCount,
    ports,
    portNames,
    hints,
    nullptr, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    nullptr, // Run adding
    nullptr, // Set run adding gain
    deactivate,
    cleanup
};

const DSSI_Descriptor 
SamplePlayer::dssiDescriptor =
{
    2, // DSSI API version
    &ladspaDescriptor,
    configure,
    getProgram,
    selectProgram,
    getMidiController,
    runSynth,
    nullptr, // Run synth adding
    nullptr, // Run multiple synths
    nullptr, // Run multiple synths adding
    receiveHostDescriptor
};

const DSSI_Host_Descriptor *
SamplePlayer::hostDescriptor = nullptr;


const DSSI_Descriptor *
SamplePlayer::getDescriptor(unsigned long index)
{
    if (index == 0) return &dssiDescriptor;
    return nullptr;
}

SamplePlayer::SamplePlayer(int sampleRate) :
    m_output(nullptr),
    m_retune(nullptr),
    m_basePitch(nullptr),
    m_concertA(nullptr),
    m_sustain(nullptr),
    m_release(nullptr),
    m_sampleData(nullptr),
    m_sampleCount(0),
    m_sampleRate(sampleRate),
    m_sampleNo(0),
    m_sampleDir("samples"),
    m_sampleSearchComplete(false),
    m_pendingProgramChange(-1)
{
}

SamplePlayer::~SamplePlayer()
{
    delete[] m_sampleData;
}
    
LADSPA_Handle
SamplePlayer::instantiate(const LADSPA_Descriptor *, unsigned long rate)
{
    if (!hostDescriptor || !hostDescriptor->request_non_rt_thread) {
        SVDEBUG << "SamplePlayer::instantiate: Host does not provide request_non_rt_thread, not instantiating" << endl;
        return nullptr;
    }

    SamplePlayer *player = new SamplePlayer(int(rate));
        // std::cerr << "Instantiated sample player " << std::endl;

    if (hostDescriptor->request_non_rt_thread(player, workThreadCallback)) {
        SVDEBUG << "SamplePlayer::instantiate: Host rejected request_non_rt_thread call, not instantiating" << endl;
        delete player;
        return nullptr;
    }

    return player;
}

void
SamplePlayer::connectPort(LADSPA_Handle handle,
                          unsigned long port, LADSPA_Data *location)
{
    SamplePlayer *player = (SamplePlayer *)handle;

    float **ports[PortCount] = {
        &player->m_output,
        &player->m_retune,
        &player->m_basePitch,
        &player->m_concertA,
        &player->m_sustain,
        &player->m_release
    };

    *ports[port] = (float *)location;
}

void
SamplePlayer::activate(LADSPA_Handle handle)
{
    SamplePlayer *player = (SamplePlayer *)handle;
    QMutexLocker locker(&player->m_mutex);

    player->m_sampleNo = 0;

    for (size_t i = 0; i < Polyphony; ++i) {
        player->m_ons[i] = -1;
        player->m_offs[i] = -1;
        player->m_velocities[i] = 0;
    }
}

void
SamplePlayer::run(LADSPA_Handle handle, unsigned long samples)
{
    runSynth(handle, samples, nullptr, 0);
}

void
SamplePlayer::deactivate(LADSPA_Handle handle)
{
    activate(handle); // both functions just reset the plugin
}

void
SamplePlayer::cleanup(LADSPA_Handle handle)
{
    delete (SamplePlayer *)handle;
}

char *
SamplePlayer::configure(LADSPA_Handle handle, const char *key, const char *value)
{
    if (key && !strcmp(key, "sampledir")) {

        SamplePlayer *player = (SamplePlayer *)handle;

        QMutexLocker locker(&player->m_mutex);

        if (QFileInfo(value).exists() &&
            QFileInfo(value).isDir()) {

            player->m_sampleDir = value;

            if (player->m_sampleSearchComplete) {
                player->m_sampleSearchComplete = false;
                player->searchSamples();
            }

            return nullptr;

        } else {
            size_t len = strlen(value) + 80;
            char *buffer = (char *)malloc(len);
            snprintf(buffer, len, "Sample directory \"%s\" does not exist, leaving unchanged", value);
            return buffer;
        }
    }

    return strdup("Unknown configure key");
}

const DSSI_Program_Descriptor *
SamplePlayer::getProgram(LADSPA_Handle handle, unsigned long program)
{
    SamplePlayer *player = (SamplePlayer *)handle;

    if (!player->m_sampleSearchComplete) {
        QMutexLocker locker(&player->m_mutex);
        if (!player->m_sampleSearchComplete) {
            player->searchSamples();
        }
    }
    if (program >= player->m_samples.size()) return nullptr;

    static DSSI_Program_Descriptor descriptor;
    static char name[60];

    strncpy(name, player->m_samples[program].first.toLocal8Bit().data(), 59);
    name[59] = '\0';

    descriptor.Bank = 0;
    descriptor.Program = program;
    descriptor.Name = name;

    return &descriptor;
}

void
SamplePlayer::selectProgram(LADSPA_Handle handle,
                            unsigned long,
                            unsigned long program)
{
    SamplePlayer *player = (SamplePlayer *)handle;
    player->m_pendingProgramChange = (int)program;
}

int
SamplePlayer::getMidiController(LADSPA_Handle, unsigned long port)
{
    int controllers[PortCount] = {
        DSSI_NONE,
        DSSI_CC(12),
        DSSI_CC(13),
        DSSI_CC(64),
        DSSI_CC(72)
    };

    return controllers[port];
}

void
SamplePlayer::runSynth(LADSPA_Handle handle, unsigned long samples,
                       snd_seq_event_t *events, unsigned long eventCount)
{
    SamplePlayer *player = (SamplePlayer *)handle;

    player->runImpl(samples, events, eventCount);
}

void
SamplePlayer::receiveHostDescriptor(const DSSI_Host_Descriptor *descriptor)
{
    hostDescriptor = descriptor;
}

void
SamplePlayer::workThreadCallback(LADSPA_Handle handle)
{
    SamplePlayer *player = (SamplePlayer *)handle;

    if (player->m_pendingProgramChange >= 0) {

#ifdef DEBUG_SAMPLE_PLAYER
        SVDEBUG << "SamplePlayer::workThreadCallback: pending program change " << player->m_pendingProgramChange << endl;
#endif

        player->m_mutex.lock();

        int program = player->m_pendingProgramChange;
        player->m_pendingProgramChange = -1;

        if (!player->m_sampleSearchComplete) {
            player->searchSamples();
        }
        
        if (program < int(player->m_samples.size())) {
            QString path = player->m_samples[program].second;
            QString programName = player->m_samples[program].first;
            if (programName != player->m_program) {
                player->m_program = programName;
                player->m_mutex.unlock();
                player->loadSampleData(path);
            } else {
                player->m_mutex.unlock();
            }
        }
    }

    if (!player->m_sampleSearchComplete) {

        QMutexLocker locker(&player->m_mutex);

        if (!player->m_sampleSearchComplete) {
            player->searchSamples();
        }
    }
}

void
SamplePlayer::searchSamples()
{
    if (m_sampleSearchComplete) return;

    m_samples.clear();

#ifdef DEBUG_SAMPLE_PLAYER
    SVDEBUG << "SamplePlayer::searchSamples: Directory is \""
              << m_sampleDir << "\"" << endl;
#endif

    QDir dir(m_sampleDir, "*.wav");
    
    for (unsigned int i = 0; i < dir.count(); ++i) {
        QFileInfo file(dir.filePath(dir[i]));
        if (file.isReadable()) {
            m_samples.push_back(std::pair<QString, QString>
                                (file.baseName(), file.filePath()));
#ifdef DEBUG_SAMPLE_PLAYER
            SVCERR << "Found: " << dir[i] << endl;
#endif
        }
    }
    
    m_sampleSearchComplete = true;
}

void
SamplePlayer::loadSampleData(QString path)
{
    breakfastquay::AudioReadStream *stream = nullptr;
    try {
        stream = breakfastquay::AudioReadStreamFactory::createReadStream
            (path.toStdString());
    } catch (const std::exception &e) {
        SVCERR << "SamplePlayer::loadSampleData: ERROR: " << e.what() << endl;
        return;
    } 

    auto channels = stream->getChannelCount();
    auto rate = stream->getSampleRate();
    auto frames = stream->getEstimatedFrameCount();
        
    if (!stream->isSeekable() || frames == 0) {
        SVCERR << "SamplePlayer::loadSampleData: ERROR: Audio file \""
               << path << "\" must be of a format with known length"
               << endl;
        delete stream;
        return;
    }

    // Allow some extra at the end in case of resampler imprecision
    size_t padding = 1000;
    
    vector<float> interleaved((frames + padding) * channels, 0.f);
    auto obtained = stream->getInterleavedFrames(frames, interleaved.data());
    delete stream;
    
    if (obtained < frames) {
        SVCERR << "SamplePlayer::loadSampleData: ERROR: Too few frames read from \""
               << path << "\" (expected " << frames << ", got " << obtained
               << ")" << endl;
        return;
    }

    vector<float> resampled;
    size_t targetFrames = frames;

    if (rate != m_sampleRate) {
        
        double ratio = (double)m_sampleRate / (double)rate;
        targetFrames = (size_t)(round(double(frames) * ratio));
        resampled = vector<float>(targetFrames * channels, 0.f);

        breakfastquay::Resampler::Parameters params;
        params.quality = breakfastquay::Resampler::Best;
        params.dynamism = breakfastquay::Resampler::RatioMostlyFixed;
        params.ratioChange = breakfastquay::Resampler::SuddenRatioChange;

        breakfastquay::Resampler resampler(params, channels);
        int obtained = resampler.resampleInterleaved
            (resampled.data(), targetFrames,
             interleaved.data(), frames + padding,
             ratio, true);
        if (obtained != targetFrames) {
            SVDEBUG << "SamplePlayer::loadSampleData: WARNING: Expected "
                    << targetFrames << " frames from resampler (input frames = "
                    << frames << ", padding = " << padding << ", ratio = "
                    << ratio << "), obtained " << obtained << endl;
        }
    } else {
        resampled = interleaved;
    }

    /* mixdown, adding an extra sample for linear interpolation */
    float *tmpSamples = new float[targetFrames + 1];
    
    for (int i = 0; i < targetFrames; ++i) {
        tmpSamples[i] = 0.0f;
        for (int j = 0; j < channels; ++j) {
            tmpSamples[i] += resampled[i * channels + j];
        }
    }
    tmpSamples[targetFrames] = 0.0f;

    QMutexLocker locker(&m_mutex);

    float *old = m_sampleData;
    m_sampleData = tmpSamples;
    m_sampleCount = targetFrames;

    for (int i = 0; i < Polyphony; ++i) {
        m_ons[i] = -1;
        m_offs[i] = -1;
        m_velocities[i] = 0;
    }

    delete[] old;

    printf("%s: loaded %s (%ld samples from original %ld channels resampled from %ld frames at %ld Hz)\n", "sampler", path.toStdString().data(), (long)targetFrames, (long)channels, (long)frames, (long)rate);
}

void
SamplePlayer::runImpl(unsigned long sampleCount,
                      snd_seq_event_t *events,
                      unsigned long eventCount)
{
    unsigned long pos;
    unsigned long count;
    unsigned long event_pos;
    int i;

    memset(m_output, 0, sampleCount * sizeof(float));

    if (!m_mutex.tryLock()) return;

    if (!m_sampleData || !m_sampleCount) {
        m_sampleNo += sampleCount;
        m_mutex.unlock();
        return;
    }

    for (pos = 0, event_pos = 0; pos < sampleCount; ) {

        while (event_pos < eventCount
               && pos >= events[event_pos].time.tick) {

            if (events[event_pos].type == SND_SEQ_EVENT_NOTEON) {
#ifdef DEBUG_SAMPLE_PLAYER
                SVCERR << "SamplePlayer: found NOTEON at time " 
                          << events[event_pos].time.tick << endl;
#endif
                snd_seq_ev_note_t n = events[event_pos].data.note;
                if (n.velocity > 0) {
                    m_ons[n.note] =
                        m_sampleNo + events[event_pos].time.tick;
                    m_offs[n.note] = -1;
                    m_velocities[n.note] = n.velocity;
                } else {
                    if (!m_sustain || (*m_sustain < 0.001)) {
                        m_offs[n.note] = 
                            m_sampleNo + events[event_pos].time.tick;
                    }
                }
            } else if (events[event_pos].type == SND_SEQ_EVENT_NOTEOFF &&
                       (!m_sustain || (*m_sustain < 0.001))) {
#ifdef DEBUG_SAMPLE_PLAYER
                SVCERR << "SamplePlayer: found NOTEOFF at time " 
                          << events[event_pos].time.tick << endl;
#endif
                snd_seq_ev_note_t n = events[event_pos].data.note;
                m_offs[n.note] = 
                    m_sampleNo + events[event_pos].time.tick;
            }

            ++event_pos;
        }

        count = sampleCount - pos;
        if (event_pos < eventCount &&
            events[event_pos].time.tick < sampleCount) {
            count = events[event_pos].time.tick - pos;
        }

        int notecount = 0;

        for (i = 0; i < Polyphony; ++i) {
            if (m_ons[i] >= 0) {
                ++notecount;
                addSample(i, pos, count);
            }
        }

#ifdef DEBUG_SAMPLE_PLAYER
        SVCERR << "SamplePlayer: have " << notecount << " note(s) sounding currently" << endl;
#else
        (void)notecount;
#endif

        pos += count;
    }

    m_sampleNo += sampleCount;
    m_mutex.unlock();
}

void
SamplePlayer::addSample(int n, unsigned long pos, unsigned long count)
{
    float ratio = 1.f;
    float gain = 1.f;
    unsigned long i, s;

    if (m_retune && *m_retune) {
        if (m_concertA) {
            ratio *= *m_concertA / 440.f;
        }
        if (m_basePitch && float(n) != *m_basePitch) {
            ratio *= powf(1.059463094f, float(n) - *m_basePitch);
        }
    }

    if (long(pos + m_sampleNo) < m_ons[n]) return;

    gain = (float)m_velocities[n] / 127.0f;

    for (i = 0, s = pos + m_sampleNo - m_ons[n];
         i < count;
         ++i, ++s) {

        float         lgain = gain;
        float         rs = float(s) * ratio;
        unsigned long rsi = lrintf(floorf(rs));

        if (rsi >= m_sampleCount) {
#ifdef DEBUG_SAMPLE_PLAYER
            SVCERR << "Note " << n << " has run out of samples (were " << m_sampleCount << " available at ratio " << ratio << "), ending" << endl;
#endif
            m_ons[n] = -1;
            break;
        }

        if (m_offs[n] >= 0 &&
            long(pos + i + m_sampleNo) > m_offs[n]) {

            unsigned long dist =
                pos + i + m_sampleNo - m_offs[n];

            unsigned long releaseFrames = 200;
            if (m_release) {
                releaseFrames = long(*m_release * float(m_sampleRate) + 0.0001f);
            }

            if (dist > releaseFrames) {
#ifdef DEBUG_SAMPLE_PLAYER
                SVCERR << "Note " << n << " has expired its release time (" << releaseFrames << " frames), ending" << endl;
#endif
                m_ons[n] = -1;
                break;
            } else {
                lgain = lgain * (float)(releaseFrames - dist) /
                    (float)releaseFrames;
            }
        }
        
        float sample = m_sampleData[rsi] +
            ((m_sampleData[rsi + 1] -
              m_sampleData[rsi]) *
             (rs - (float)rsi));

        m_output[pos + i] += lgain * sample;
    }
}
