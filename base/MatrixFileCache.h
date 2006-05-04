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

#include <sys/types.h>
#include <QString>
#include <QMutex>
#include <map>

#include "FileReadThread.h"

class MatrixFileCache : public QObject
{
    Q_OBJECT

public:
    enum Mode { ReadOnly, ReadWrite };

    MatrixFileCache(QString fileBase, Mode mode);
    virtual ~MatrixFileCache();

    size_t getWidth() const;
    size_t getHeight() const;
    
    void resize(size_t width, size_t height);
    void reset();

    float getValueAt(size_t x, size_t y);
    void getColumnAt(size_t x, float *values);

    void setValueAt(size_t x, size_t y, float value);
    void setColumnAt(size_t x, float *values);

protected slots:
    void requestCancelled(int token);

protected:
    int     m_fd;
    Mode    m_mode;
    size_t  m_width;
    size_t  m_height;
    size_t  m_headerSize;
    QString m_fileName;
    size_t  m_defaultCacheWidth;
    size_t  m_prevX;

    struct Cache {
        size_t  x;
        size_t  width;
        float  *data;
    };

    Cache m_cache;

    bool getValuesFromCache(size_t x, size_t ystart, size_t ycount,
                            float *values);

    void primeCache(size_t x, bool left);

    bool seekTo(size_t x, size_t y);

    FileReadThread m_readThread;
    int m_requestToken;
    size_t m_requestingX;
    size_t m_requestingWidth;

    static std::map<QString, int> m_refcount;
    static QMutex m_refcountMutex;
    QMutex m_fdMutex;
    QMutex m_cacheMutex;
};

#endif

