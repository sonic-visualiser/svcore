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

#include "NonRTThread.h"

#ifndef _WIN32
#include <pthread.h>
#endif

NonRTThread::NonRTThread(QObject *parent) :
    QThread(parent)
{
    setStackSize(512 * 1024);
}

void
NonRTThread::start()
{
    QThread::start();

#ifndef _WIN32
    struct sched_param param;
    ::memset(&param, 0, sizeof(param));

    if (::pthread_setschedparam(pthread_self(), SCHED_OTHER, &param)) {
        ::perror("pthread_setschedparam to SCHED_OTHER failed");
    }
#endif
}

