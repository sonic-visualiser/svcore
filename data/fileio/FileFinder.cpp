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

#include "FileFinder.h"
#include "RemoteFile.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>


FileFinder::FileFinder(QString location, QString lastKnownLocation) :
    m_location(location),
    m_lastKnownLocation(lastKnownLocation),
    m_lastLocatedLocation("")
{
}

FileFinder::~FileFinder()
{
}

QString
FileFinder::getLocation()
{
    if (QFileInfo(m_location).exists()) return m_location;

    if (QMessageBox::question(0,
                              QMessageBox::tr("Failed to open file"),
                              QMessageBox::tr("Audio file \"%1\" could not be opened.\nLocate it?").arg(m_location),
//!!!                                  QMessageBox::tr("File \"%1\" could not be opened.\nLocate it?").arg(location),
                              QMessageBox::Ok,
                              QMessageBox::Cancel) == QMessageBox::Ok) {

        //!!! This uses QFileDialog::getOpenFileName, while other
        //files are located using specially built file dialogs in
        //MainWindow::getOpenFileName -- pull out MainWindow
        //functions into another class?
        QString path = QFileDialog::getOpenFileName
            (0,
             QFileDialog::tr("Locate file \"%1\"").arg(QFileInfo(m_location).fileName()), m_location,
             QFileDialog::tr("All files (*.*)"));
/*!!!
                 QFileDialog::tr("Audio files (%1)\nAll files (*.*)")
                 .arg(AudioFileReaderFactory::getKnownExtensions()));
*/

        if (path != "") {
            return path;
        }
    }

    return "";
}


