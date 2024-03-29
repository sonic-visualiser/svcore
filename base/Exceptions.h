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

#ifndef SV_EXCEPTIONS_H
#define SV_EXCEPTIONS_H

#include <exception>

#include <QString>

#include "Debug.h"

namespace sv {

class FileNotFound : virtual public std::exception
{
public:
    FileNotFound(QString file) throw();
    virtual ~FileNotFound() throw() { }
    const char *what() const throw() override;
    
protected:
    QString m_file;
};

class FailedToOpenFile : virtual public std::exception
{
public:
    FailedToOpenFile(QString file) throw();
    virtual ~FailedToOpenFile() throw() { }
    const char *what() const throw() override;
    
protected:
    QString m_file;
};

class DirectoryCreationFailed : virtual public std::exception
{
public:
    DirectoryCreationFailed(QString directory) throw();
    virtual ~DirectoryCreationFailed() throw() { }
    const char *what() const throw() override;
    
protected:
    QString m_directory;
};

class FileReadFailed : virtual public std::exception
{
public:
    FileReadFailed(QString file) throw();
    virtual ~FileReadFailed() throw() { }
    const char *what() const throw() override;

protected:
    QString m_file;
};

class FileOperationFailed : virtual public std::exception
{
public:
    FileOperationFailed(QString file, QString operation) throw();
    virtual ~FileOperationFailed() throw() { }
    const char *what() const throw() override;

protected:
    QString m_file;
    QString m_operation;
};

class InsufficientDiscSpace : virtual public std::exception
{
public:
    InsufficientDiscSpace(QString directory,
                          size_t required, size_t available) throw();
    InsufficientDiscSpace(QString directory) throw();
    virtual ~InsufficientDiscSpace() throw() { }
    const char *what() const throw() override;

    QString getDirectory() const { return m_directory; }
    size_t getRequired() const { return m_required; }
    size_t getAvailable() const { return m_available; }

protected:
    QString m_directory;
    size_t m_required;
    size_t m_available;
};

class AllocationFailed : virtual public std::exception
{
public:
    AllocationFailed(QString purpose) throw();
    virtual ~AllocationFailed() throw() { }
    const char *what() const throw() override;

protected:
    QString m_purpose;
};

} // end namespace sv

#endif
