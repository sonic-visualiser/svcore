/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_PLAY_PARAMETER_REPOSITORY_H
#define SV_PLAY_PARAMETER_REPOSITORY_H

#include "PlayParameters.h"
#include "Command.h"

class Playable;

#include <map>

#include <QObject>
#include <QString>

class PlayParameterRepository : public QObject
{
    Q_OBJECT

public:
    static PlayParameterRepository *getInstance();

    virtual ~PlayParameterRepository();

    /**
     * Register a playable.
     * 
     * The id must be of an object that is registered with the ById
     * store and that can be dynamic_cast to Playable.
     */
    void addPlayable(int playableId);

    /**
     * Unregister a playable.
     * 
     * The id must be of an object that is registered with the ById
     * store and that can be dynamic_cast to Playable.
     */
    void removePlayable(int playableId);

    /**
     * Copy the play parameters from one playable to another.
     * 
     * The ids must be of objects that are registered with the ById
     * store and that can be dynamic_cast to Playable.
     */
    void copyParameters(int fromId, int toId);

    /**
     * Retrieve the play parameters for a playable.
     * 
     * The id must be of an object that is registered with the ById
     * store and that can be dynamic_cast to Playable.
     */
    PlayParameters *getPlayParameters(int playableId);

    void clear();

    class EditCommand : public Command
    {
    public:
        EditCommand(PlayParameters *params);
        void setPlayMuted(bool);
        void setPlayAudible(bool);
        void setPlayPan(float);
        void setPlayGain(float);
        void setPlayClipId(QString);
        void execute() override;
        void unexecute() override;
        QString getName() const override;

    protected:
        PlayParameters *m_params;
        PlayParameters m_from;
        PlayParameters m_to;
    };

signals:
    void playParametersChanged(PlayParameters *);
    void playClipIdChanged(int playableId, QString);

protected slots:
    void playParametersChanged();
    void playClipIdChanged(QString);

protected:
    typedef std::map<int, PlayParameters *> PlayableParameterMap;
    PlayableParameterMap m_playParameters;

    static PlayParameterRepository *m_instance;
};

#endif
