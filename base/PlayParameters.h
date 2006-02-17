/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _PLAY_PARAMETERS_H_
#define _PLAY_PARAMETERS_H_

#include <QObject>

class PlayParameters : public QObject
{
    Q_OBJECT

public:
    PlayParameters() : m_playMuted(false), m_playPan(0.0), m_playGain(1.0) { }

    virtual bool isPlayMuted() const { return m_playMuted; }
    virtual float getPlayPan() const { return m_playPan; } // -1.0 -> 1.0
    virtual float getPlayGain() const { return m_playGain; }

signals:
    void playParametersChanged();
    void playMutedChanged(bool);
    void playPanChanged(float);
    void playGainChanged(float);

public slots:
    virtual void setPlayMuted(bool muted);
    virtual void setPlayAudible(bool nonMuted);
    virtual void setPlayPan(float pan);
    virtual void setPlayGain(float gain);

protected:
    bool m_playMuted;
    float m_playPan;
    float m_playGain;
};

#endif

    

    
