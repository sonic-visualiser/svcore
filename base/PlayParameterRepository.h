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
    static PlayParameterRepository *getInstance();

    virtual ~PlayParameterRepository();

    void addModel(const Model *model);
    void removeModel(const Model *model);

    PlayParameters *getPlayParameters(const Model *model) const;

    void clear();

signals:
    void playParametersChanged(PlayParameters *);
    void playPluginIdChanged(const Model *, QString);
    void playPluginConfigurationChanged(const Model *, QString);

protected slots:
    void playParametersChanged();
    void playPluginIdChanged(QString);
    void playPluginConfigurationChanged(QString);

protected:
    typedef std::map<const Model *, PlayParameters *> ModelParameterMap;
    ModelParameterMap m_playParameters;

    static PlayParameterRepository *m_instance;
};

#endif
