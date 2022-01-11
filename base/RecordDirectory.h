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

#ifndef SV_RECORD_DIRECTORY_H
#define SV_RECORD_DIRECTORY_H

#include <QString>

/**
 * Report the intended target location for recorded audio files.
 */
class RecordDirectory
{
public:
    /**
     * Return the directory in which a recorded file should be saved.
     * This may vary depending on the current date and time, and so
     * should be queried afresh for each recording. The directory will
     * also be created if it does not yet exist.
     *
     * Returns an empty string if the record directory did not exist
     * and could not be created.
     */
    static QString getRecordDirectory();

    /**
     * Return the root "recorded files" directory. If
     * getRecordDirectory() is returning a datestamped directory, then
     * this will be its parent. The directory will also be created if
     * it does not yet exist.
     *
     * Returns an empty string if the record directory did not exist
     * and could not be created.
     */
    static QString getRecordContainerDirectory();

    /**
     * Set a specific directory as the root "recorded files"
     * directory, which will subsequently be returned by
     * getRecordContainerDirectory() and used to construct paths for
     * getRecordDirectory(). Note that the default is to use a
     * standard app-data specific location; if that is acceptable to
     * you, there is no need ever to call this.
     */
    static void setRecordContainerDirectory(QString);
    
    /**
     * Return the directory in which an audio file converted from a
     * data file should be saved. The directory will also be created if
     * it does not yet exist.
     *
     * Returns an empty string if the directory did not exist and
     * could not be created.
     */
    static QString getConvertedAudioDirectory();
};

#endif
