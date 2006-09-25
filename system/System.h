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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef _WIN32

#include <windows.h>
#include <malloc.h>
#include <process.h>

#define MLOCK(a,b)   1
#define MUNLOCK(a,b) 1
#define MUNLOCK_SAMPLEBLOCK(a) 1

#define DLOPEN(a,b)  LoadLibrary((a).toStdWString().c_str())
#define DLSYM(a,b)   GetProcAddress((HINSTANCE)(a),(b))
#define DLCLOSE(a)   FreeLibrary((HINSTANCE)(a))
#define DLERROR()    ""

#define PLUGIN_GLOB  "*.dll"
#define PATH_SEPARATOR ';'
#define DEFAULT_VAMP_PATH "%ProgramFiles%\\Vamp Plugins"

#define getpid _getpid

extern "C" {
void usleep(unsigned long usec);
void gettimeofday(struct timeval *p, void *tz);
}

#else

#include <sys/mman.h>
#include <dlfcn.h>

#define MLOCK(a,b)   ::mlock((a),(b))
#define MUNLOCK(a,b) (::munlock((a),(b)) ? (::perror("munlock failed"), 0) : 0)
#define MUNLOCK_SAMPLEBLOCK(a) do { if (!(a).empty()) { const float &b = *(a).begin(); MUNLOCK(&b, (a).capacity() * sizeof(float)); } } while(0);

#define DLOPEN(a,b)  dlopen((a).toStdString().c_str(),(b))
#define DLSYM(a,b)   dlsym((a),(b))
#define DLCLOSE(a)   dlclose((a))
#define DLERROR()    dlerror()

#define PATH_SEPARATOR ':'

#ifdef __APPLE__

#define PLUGIN_GLOB  "*.dylib"
#define DEFAULT_VAMP_PATH "/Library/Audio/Plug-Ins/Vamp/:$HOME/Library/Audio/Plug-Ins/Vamp"

#else 

#define PLUGIN_GLOB  "*.so"
#define DEFAULT_VAMP_PATH "/usr/local/lib/vamp:/usr/lib/vamp:$HOME/vamp:$HOME/.vamp"

#endif /* __APPLE__ */

#endif /* ! _WIN32 */

enum ProcessStatus { ProcessRunning, ProcessNotRunning, UnknownProcessStatus };
extern ProcessStatus GetProcessStatus(int pid);

// Return a vague approximation to the number of free megabytes of real memory.
// Return -1 if unknown.
extern int GetRealMemoryMBAvailable();

// Return a vague approximation to the number of free megabytes of disc space
// on the partition containing the given path.  Return -1 if unknown.
extern int GetDiscSpaceMBAvailable(const char *path);

#include <cmath>

extern double mod(double x, double y);
extern float modf(float x, float y);

extern double princarg(double a);
extern float princargf(float a);

#endif /* ! _SYSTEM_H_ */


