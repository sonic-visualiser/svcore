/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _PLAY_PARAMETER_REPOSITORY_H_
#define _PLAY_PARAMETER_REPOSITORY_H_

class PlayParameters;
class Model;

#include <map>

class PlayParameterRepository
{
public:
    static PlayParameterRepository *instance();

    virtual ~PlayParameterRepository();

//!!! No way to remove a model!
    PlayParameters *getPlayParameters(const Model *model);

    void clear();

protected:
    std::map<const Model *, PlayParameters *> m_playParameters;

    static PlayParameterRepository *m_instance;
};

#endif
