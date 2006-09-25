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

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QString>

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

int
GetRealMemoryMBAvailable()
{
    // ugh
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QFile::ReadOnly)) {
        std::cerr << "opened meminfo" << std::endl;
        QTextStream in(&meminfo);
        while (!in.atEnd()) {
            QString line = in.readLine(256);
            std::cerr << "read: \"" << line.toStdString() << "\"" << std::endl;
            if (line.startsWith("MemFree:")) {
                QStringList elements = line.split(' ', QString::SkipEmptyParts);
                QString unit = "kB";
                if (elements.size() > 2) unit = elements[2];
                int size = elements[1].toInt();
                std::cerr << "have size \"" << size << "\", unit \""
                          << unit.toStdString() << "\"" << std::endl;
                if (unit.toLower() == "gb") return size * 1024;
                if (unit.toLower() == "mb") return size;
                if (unit.toLower() == "kb") return size / 1024;
                return size / 1048576;
            }
        }
    }
    return -1;
}

int
GetDiscSpaceMBAvailable(const char *path)
{
#ifdef _WIN32
    __int64 available, total, totalFree;
    if (GetDiskFreeSpaceEx(path, &available, &total, &totalFree)) {
        available /= 1048576;
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
        return ((buf.f_bavail / 1024) * buf.f_bsize) / 1024;
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

