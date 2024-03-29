/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2009 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_FILE_FINDER_H
#define SV_FILE_FINDER_H

#include <QString>

namespace sv {

class FileFinder 
{
public:
    virtual ~FileFinder() { }
    
    enum FileType {
        SessionFile,
        AudioFile,
        LayerFile,
        LayerFileNoMidi,
        SessionOrAudioFile,
        ImageFile,
        SVGFile,
        AnyFile,
        CSVFile,
        LayerFileNonSV,
        LayerFileNoMidiNonSV,
    };

    virtual QString getOpenFileName(FileType type,
                                    QString fallbackLocation = "") = 0;

    virtual QStringList getOpenFileNames(FileType type,
                                         QString fallbackLocation = "") = 0;

    virtual QString getSaveFileName(FileType type,
                                    QString fallbackLocation = "") = 0;

    virtual void registerLastOpenedFilePath(FileType type,
                                            QString path) = 0;

    virtual QString find(FileType type,
                         QString location,
                         QString lastKnownLocation = "") = 0;

    static FileFinder *getInstance() {
        FFContainer *container = FFContainer::getInstance();
        return container->getFileFinder();
    }

protected:
    class FFContainer {
    public:
        static FFContainer *getInstance() {
            static FFContainer instance;
            return &instance;
        }
        void setFileFinder(FileFinder *ff) { m_ff = ff; }
        FileFinder *getFileFinder() const { return m_ff; }
    private:
        FFContainer() : m_ff(nullptr) { }
        FileFinder *m_ff;
    };

    static void registerFileFinder(FileFinder *ff) {
        FFContainer *container = FFContainer::getInstance();
        container->setFileFinder(ff);
    }
};

} // end namespace sv

#endif

    
