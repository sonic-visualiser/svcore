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

#include <QStringList>
#include <QString>

#include <stdint.h>

#ifndef _WIN32
#include <signal.h>
#include <sys/statvfs.h>
#endif

#include <iostream>

#ifdef _WIN32

extern "C" {

void usleep(unsigned long usec)
{
    ::Sleep(usec / 1000);
}

void gettimeofday(struct timeval *tv, void *tz)
{
    union { 
	long long ns100;  
	FILETIME ft; 
    } now; 
    
    ::GetSystemTimeAsFileTime(&now.ft); 
    tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL); 
    tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL); 
}

}

#endif

ProcessStatus
GetProcessStatus(int pid)
{
#ifdef _WIN32
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
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

void
GetRealMemoryMBAvailable(int &available, int &total)
{
    available = -1;
    total = -1;

    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (!meminfo) return;

    char buf[256];
    while (!feof(meminfo)) {
        fgets(buf, 256, meminfo);
        bool isMemFree = (strncmp(buf, "MemFree:", 8) == 0);
        bool isMemTotal = (!isMemFree && (strncmp(buf, "MemTotal:", 9) == 0));
        if (isMemFree || isMemTotal) {
            QString line = QString(buf).trimmed();
            QStringList elements = line.split(' ', QString::SkipEmptyParts);
            QString unit = "kB";
            if (elements.size() > 2) unit = elements[2];
            int size = elements[1].toInt();
//            std::cerr << "have size \"" << size << "\", unit \""
//                      << unit.toStdString() << "\"" << std::endl;
            if (unit.toLower() == "gb") size = size * 1024;
            else if (unit.toLower() == "mb") size = size;
            else if (unit.toLower() == "kb") size = size / 1024;
            else size = size / 1048576;

            if (isMemFree) available = size;
            else total = size;
        }
        if (available != -1 && total != -1) {
            fclose(meminfo);
            return;
        }
    }
    fclose(meminfo);
}

int
GetDiscSpaceMBAvailable(const char *path)
{
#ifdef _WIN32
    __int64 available, total, totalFree;
    if (GetDiskFreeSpaceEx(path, &available, &total, &totalFree)) {
        available /= 1048576;
        if (available > INT_MAX) available = INT_MAX;
        return int(available);
    } else {
        std::cerr << "WARNING: GetDiskFreeSpaceEx failed: error code "
                  << GetLastError() << std::endl;
        return -1;
    }
#else
    struct statvfs buf;
    if (!statvfs(path, &buf)) {
        // do the multiplies and divides in this order to reduce the
        // likelihood of arithmetic overflow
        std::cerr << "statvfs(" << path << ") says available: " << buf.f_bavail << ", block size: " << buf.f_bsize << std::endl;
        uint64_t available = ((buf.f_bavail / 1024) * buf.f_bsize) / 1024;
        if (available > INT_MAX) available = INT_MAX;
        return int(available);
    } else {
        perror("statvfs failed");
        return -1;
    }
#endif
}
    

double mod(double x, double y) { return x - (y * floor(x / y)); }
float modf(float x, float y) { return x - (y * floorf(x / y)); }

double princarg(double a) { return mod(a + M_PI, -2 * M_PI) + M_PI; }
float princargf(float a) { return modf(a + M_PI, -2 * M_PI) + M_PI; }

