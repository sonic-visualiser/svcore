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

#ifndef _WIN32
#include <signal.h>
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
#ifdef _WIN32
    DWORD handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!handle) {
        return ProcessNotRunning;
    } else {
        CloseHandle(handle);
        return ProcessRunning;
    }
#else
    if (kill(getpid(), 0) == 0) {
        if (kill(pid, 0) == 0) {
            return ProcessRunning;
        } else {
            return ProcessNotRunning;
        }
    } else {
        return UnknownProcessStatus;
    }
#endif
}

