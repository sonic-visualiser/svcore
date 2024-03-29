/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Serialiser.h"
#include "Debug.h"

#include <iostream>

namespace sv {

QMutex
Serialiser::m_mapMutex;

std::map<QString, QMutex *>
Serialiser::m_mutexMap;

Serialiser::Serialiser(QString id) :
    Serialiser(id, nullptr) { }

Serialiser::Serialiser(QString id, const std::atomic<bool> *cancelled) :
    m_id(id),
    m_cancelled(cancelled),
    m_locked(false)
{
    m_mapMutex.lock();
    
    if (m_mutexMap.find(m_id) == m_mutexMap.end()) {
        m_mutexMap[m_id] = new QMutex;
    }

    // The id mutexes are never deleted, so once we have a reference
    // to the one we need, we can hold on to it while we release the
    // map mutex.  We need to release the map mutex, otherwise if the
    // id mutex is currently held, it will never be released (because
    // the destructor needs to hold the map mutex to release the id
    // mutex).

    QMutex *idMutex = m_mutexMap[m_id];

    m_mapMutex.unlock();

    if (!m_cancelled) {
        idMutex->lock();
        m_locked = true;
    } else {
        // try to lock, polling the cancelled status occasionally
        while (!m_locked && ! *m_cancelled) {
            m_locked = idMutex->tryLock(500);
            if (*m_cancelled) {
                SVCERR << "Serialiser: cancelled!" << endl;
            }
        }
    }
}

Serialiser::~Serialiser()
{
    m_mapMutex.lock();

    if (m_locked) {
        m_mutexMap[m_id]->unlock();
    }

    m_mapMutex.unlock();
}


    
} // end namespace sv

