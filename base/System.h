/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef _WIN32

#include <windows.h>
#include <malloc.h>

#define MLOCK(a,b)   1
#define MUNLOCK(a,b) 1

#define DLOPEN(a,b)  LoadLibrary((a).toStdWString().c_str())
#define DLSYM(a,b)   GetProcAddress((HINSTANCE)(a),(b))
#define DLCLOSE(a)   FreeLibrary((HINSTANCE)(a))
#define DLERROR()    ""

#define PLUGIN_GLOB  "*.dll"

extern "C" {
void gettimeofday(struct timeval *p, void *tz);
}

#else

#include <sys/mman.h>
#include <dlfcn.h>

#define MLOCK(a,b)   ::mlock((a),(b))
#define MUNLOCK(a,b) ::munlock((a),(b))

#define DLOPEN(a,b)  dlopen((a).toStdString().c_str(),(b))
#define DLSYM(a,b)   dlsym((a),(b))
#define DLCLOSE(a)   dlclose((a))
#define DLERROR()    dlerror()

#define PLUGIN_GLOB  "*.so"

#endif

#endif

