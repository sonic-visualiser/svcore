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

#ifndef _MATRIX_FILE_CACHE_H_
#define _MATRIX_FILE_CACHE_H_

#include "base/ResizeableBitset.h"

#include "FileReadThread.h"

#include <sys/types.h>
#include <QString>
#include <QMutex>
#include <map>

class MatrixFile : public QObject
{
    Q_OBJECT

public:
    enum Mode { ReadOnly, ReadWrite };

    /**
     * Construct a MatrixFile object reading from and/or writing to
     * the matrix file with the given base name in the application's
     * temporary directory.
     *
     * If mode is ReadOnly, the file must exist and be readable.
     *
     * If mode is ReadWrite and the file does not exist, it will be
     * created.  If mode is ReadWrite and the file does exist, the
     * existing file will be used and the mode will be reset to
     * ReadOnly.  Call getMode() to check whether this has occurred
     * after construction.
     *
     * cellSize specifies the size in bytes of the object type stored
     * in the matrix.  For example, use cellSize = sizeof(float) for a
     * matrix of floats.  The MatrixFile object doesn't care about the
     * objects themselves, it just deals with raw data of a given size.
     *
     * If eagerCache is true, blocks from the file will be cached for
     * read.  If eagerCache is false, only columns that have been set
     * by calling setColumnAt on this MatrixFile (i.e. columns for
     * which haveSetColumnAt returns true) will be cached.
     */
    MatrixFile(QString fileBase, Mode mode, size_t cellSize, bool eagerCache);
    virtual ~MatrixFile();

    Mode getMode() const { return m_mode; }

    size_t getWidth() const { return m_width; }
    size_t getHeight() const { return m_height; }
    size_t getCellSize() const { return m_cellSize; }
    
    void resize(size_t width, size_t height);
    void reset();

    bool haveSetColumnAt(size_t x) const { return m_columnBitset->get(x); }
    void getColumnAt(size_t x, void *data); // may throw FileReadFailed
    void setColumnAt(size_t x, const void *data);

    void suspend();

protected:
    int     m_fd;
    Mode    m_mode;
    int     m_flags;
    mode_t  m_fmode;
    size_t  m_cellSize;
    size_t  m_width;
    size_t  m_height;
    size_t  m_headerSize;
    QString m_fileName;
    size_t  m_defaultCacheWidth;
    size_t  m_prevX;

    struct Cache {
        size_t x;
        size_t width;
        char *data;
    };

    Cache m_cache;
    bool  m_eagerCache;

    bool getFromCache(size_t x, size_t ystart, size_t ycount, void *data);
    void primeCache(size_t x, bool left);

    void resume();

    bool seekTo(size_t x, size_t y);

    static FileReadThread *m_readThread;
    int m_requestToken;

    size_t m_requestingX;
    size_t m_requestingWidth;
    char *m_spareData;

    static std::map<QString, int> m_refcount;
    static QMutex m_refcountMutex;
    QMutex m_fdMutex;
    QMutex m_cacheMutex;

    typedef std::map<QString, ResizeableBitset *> ResizeableBitsetMap;
    static ResizeableBitsetMap m_columnBitsets;
    static QMutex m_columnBitsetWriteMutex;
    ResizeableBitset *m_columnBitset;
};

#endif

