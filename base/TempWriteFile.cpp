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

#include "TempWriteFile.h"

#include "Exceptions.h"

#include <QTemporaryFile>
#include <QDir>
#include <iostream>

namespace sv {

TempWriteFile::TempWriteFile(QString target) :
    m_target(target)
{
    QTemporaryFile temp(m_target + ".");
    temp.setAutoRemove(false);
    temp.open(); // creates the file and opens it atomically
    if (temp.error()) {
        SVCERR << "TempWriteFile: Failed to create temporary file in directory of " << m_target << ": " << temp.errorString() << endl;
        throw FileOperationFailed(temp.fileName(), "creation");
    }
    
    m_temp = temp.fileName();
    temp.close(); // does not remove the file
}

TempWriteFile::~TempWriteFile()
{
    if (m_temp != "") {
        QDir dir(QFileInfo(m_temp).dir());
        dir.remove(m_temp);
    }
}

QString
TempWriteFile::getTemporaryFilename()
{
    return m_temp;
}

void
TempWriteFile::moveToTarget()
{
    if (m_temp == "") return;

    {
        QFile targetFile(m_target);
        if (targetFile.exists()) {
            if (!targetFile.remove()) {
                SVCERR << "TempWriteFile: WARNING: Failed to remove existing target file " << m_target << " prior to moving temporary file " << m_temp << " to it" << endl;
            }
        }
    }

    {
        QFile tempFile(m_temp);
        if (!tempFile.rename(m_target)) {
            SVCERR << "TempWriteFile: Failed to rename temporary file " << m_temp << " to target " << m_target << endl;
            throw FileOperationFailed(m_temp, "rename");
        }
    }

#ifndef Q_OS_WIN
    {
        // QTemporaryFile creates files with rw------- permissions. We
        // want rw-r--r-- minus umask. (Actually we want rw-rw-rw-
        // minus umask, but I lack the confidence to apply a change
        // that explicitly requests WriteGroup|WriteOther even though
        // I know the umask should take care of it!)
        
        QFile targetFile(m_target);
        if (!targetFile.setPermissions(QFile::ReadUser |
                                       QFile::WriteUser |
                                       QFile::ReadGroup |
                                       QFile::ReadOther)) {
            SVDEBUG << "TempWriteFile: WARNING: Failed to update file permissions after save" << endl;
        }
    }
#endif
    
    m_temp = "";
}
    
} // end namespace sv

