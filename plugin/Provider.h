/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_PROVIDER_H
#define SV_PROVIDER_H

#include <QString>
#include <QObject>

#include <set>

namespace sv {

struct Provider
{
    QString infoUrl;
    QString downloadUrl;

    enum DownloadType {
        DownloadSourceCode,
        DownloadWindows,
        DownloadMac,
        DownloadLinux32,
        DownloadLinux64,
        DownloadOther
    };
    std::set<DownloadType> downloadTypes;

    std::map<QString, QString> foundInPacks; // pack name -> pack url

    bool hasSourceDownload() const {
        return downloadTypes.find(DownloadSourceCode) != downloadTypes.end();
    }

    bool hasDownloadForThisPlatform() const {
#ifdef Q_OS_WIN32
        return downloadTypes.find(DownloadWindows) != downloadTypes.end();
#endif
#ifdef Q_OS_MAC
        return downloadTypes.find(DownloadMac) != downloadTypes.end();
#endif
#ifdef Q_OS_LINUX
        if (sizeof(void *) == 8) {
            return downloadTypes.find(DownloadLinux64) != downloadTypes.end();
        } else {
            return downloadTypes.find(DownloadLinux32) != downloadTypes.end();
        }
#endif
        return false;
    }

    static QString thisPlatformName() {
#ifdef Q_OS_WIN32
        return QObject::tr("Windows");
#endif
#ifdef Q_OS_MAC
        return QObject::tr("Mac");
#endif
#ifdef Q_OS_LINUX
        if (sizeof(void *) == 8) {
            return QObject::tr("64-bit Linux");
        } else {
            return QObject::tr("32-bit Linux");
        }
#endif
        return "(unknown)";
    }

    bool operator==(const Provider &other) {
        return
            other.infoUrl == infoUrl &&
            other.downloadUrl == downloadUrl &&
            other.downloadTypes == downloadTypes &&
            other.foundInPacks == foundInPacks;
    }
    bool operator!=(const Provider &other) {
        return !operator==(other);
    }
};

} // end namespace sv

#endif
