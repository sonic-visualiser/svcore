/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

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

#include <QObject>

class PlayParameterRepository : public QObject
{
    Q_OBJECT

public:
    static PlayParameterRepository *instance();

    virtual ~PlayParameterRepository();

    void addModel(const Model *model);
    void removeModel(const Model *model);

    PlayParameters *getPlayParameters(const Model *model) const;

    void clear();

signals:
    void playParametersChanged(PlayParameters *);

protected slots:
    void playParametersChanged();

protected:
    std::map<const Model *, PlayParameters *> m_playParameters;

    static PlayParameterRepository *m_instance;
};

#endif
