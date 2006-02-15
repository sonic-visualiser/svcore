/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _PLAY_PARAMETERS_H_
#define _PLAY_PARAMETERS_H_

#include <QObject>

class PlayParameters : virtual public QObject
{
    Q_OBJECT

public:
    PlayParameters() : m_playMuted(false), m_playPan(0.0), m_playGain(1.0) { }

    virtual bool isPlayMuted() const { return m_playMuted; }
    virtual void setPlayMuted(bool muted);
    
    virtual float getPlayPan() const { return m_playPan; } // -1.0 -> 1.0
    virtual void setPlayPan(float pan);
    
    virtual float getPlayGain() const { return m_playGain; }
    virtual void setPlayGain(float gain);

signals:
    void playParametersChanged();

protected:
    bool m_playMuted;
    float m_playPan;
    float m_playGain;
};

#endif

    

    
