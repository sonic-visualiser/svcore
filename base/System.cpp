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

#include "System.h"

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#else
#ifndef _WIN32
#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif
#endif

#include <iostream>

#ifdef _WIN32

extern "C" {

void gettimeofday(struct timeval *tv, void *tz)
{
    union { 
	long long ns100;  
	FILETIME ft; 
    } now; 
    
    GetSystemTimeAsFileTime(&now.ft); 
    tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL); 
    tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL); 
}

}

#endif

ProcessStatus
GetProcessStatus(int pid)
{
#ifdef __APPLE__

    // See
    // http://tuvix.apple.com/documentation/Darwin/Reference/ManPages/man3/sysctl.3.html
    // http://developer.apple.com/qa/qa2001/qa1123.html

    int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0, 0 };
    name[3] = pid;

    int err;
    size_t length = 0;

    if (sysctl(name, 4, 0, &length, 0, 0)) {
        perror("GetProcessStatus: sysctl failed");
        return UnknownProcessStatus;
    }

    if (length > 0) return ProcessRunning;
    else return ProcessNotRunning;

#elsif _WIN32

    return UnknownProcessStatus;

#else

    char filename[50];
    struct stat statbuf;

    // Looking up the pid in /proc is worth a try on any POSIX system,
    // I guess -- it'll always compile and it won't return false
    // negatives if we do this first check:

    sprintf(filename, "/proc/%d", (int)getpid());

    int err = stat(filename, &statbuf);

    if (err || !S_ISDIR(statbuf.st_mode)) {
        // If we can't even use it to tell whether we're running or
        // not, then clearly /proc is no use on this system.
        return UnknownProcessStatus;
    }

    sprintf(filename, "/proc/%d", (int)pid);

    err = stat(filename, &statbuf);

    if (!err) {
        if (S_ISDIR(statbuf.st_mode)) {
            return ProcessRunning;
        } else {
            return UnknownProcessStatus;
        }
    } else if (errno == ENOENT) {
        return ProcessNotRunning;
    } else {
        perror("stat failed");
        return UnknownProcessStatus;
    }

#endif
}

