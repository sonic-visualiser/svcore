/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.

    Sonic Annotator
    A utility for batch feature extraction from audio files.

    Mark Levy, Chris Sutton and Chris Cannam, Queen Mary, University of London.
    Copyright 2007-2008 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FileFeatureWriter.h"

#include "base/Exceptions.h"

#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDir>

using namespace std;
using namespace Vamp;

FileFeatureWriter::FileFeatureWriter(int support,
                                     QString extension) :
    m_prevstream(0),
    m_support(support),
    m_extension(extension),
    m_manyFiles(false),
    m_stdout(false),
    m_append(false),
    m_force(false)
{
    if (!(m_support & SupportOneFilePerTrack)) {
        if (m_support & SupportOneFilePerTrackTransform) {
            m_manyFiles = true;
        } else if (m_support & SupportOneFileTotal) {
            m_singleFileName = QString("output.%1").arg(m_extension);
        } else {
            cerr << "FileFeatureWriter::FileFeatureWriter: ERROR: Invalid support specification " << support << endl;
        }
    }
}

FileFeatureWriter::~FileFeatureWriter()
{
    while (!m_streams.empty()) {
        m_streams.begin()->second->flush();
        delete m_streams.begin()->second;
        m_streams.erase(m_streams.begin());
    }
    while (!m_files.empty()) {
        delete m_files.begin()->second;
        m_files.erase(m_files.begin());
    }
}

FileFeatureWriter::ParameterList
FileFeatureWriter::getSupportedParameters() const
{
    ParameterList pl;
    Parameter p;

    p.name = "basedir";
    p.description = "Base output directory path.  (The default is the same directory as the input file.)";
    p.hasArg = true;
    pl.push_back(p);

    if (m_support & SupportOneFilePerTrackTransform &&
        m_support & SupportOneFilePerTrack) {
        p.name = "many-files";
        p.description = "Create a separate output file for every combination of input file and transform.  The output file names will be based on the input file names.  (The default is to create one output file per input audio file, and write all transform results for that input into it.)";
        p.hasArg = false;
        pl.push_back(p);
    }

    if (m_support & SupportOneFileTotal) {
        if (m_support & ~SupportOneFileTotal) { // not only option
            p.name = "one-file";
            p.description = "Write all transform results for all input files into the single named output file.";
            p.hasArg = true;
            pl.push_back(p);
        }
        p.name = "stdout";
        p.description = "Write all transform results directly to standard output.";
        p.hasArg = false;
        pl.push_back(p);
    }

    p.name = "force";
    p.description = "If an output file already exists, overwrite it.";
    p.hasArg = false;
    pl.push_back(p);

    p.name = "append";
    p.description = "If an output file already exists, append data to it.";
    p.hasArg = false;
    pl.push_back(p);

    return pl;
}

void
FileFeatureWriter::setParameters(map<string, string> &params)
{
    for (map<string, string>::iterator i = params.begin();
         i != params.end(); ++i) {
        if (i->first == "basedir") {
            m_baseDir = i->second.c_str();
        } else if (i->first == "many-files") {
            if (m_support & SupportOneFilePerTrackTransform &&
                m_support & SupportOneFilePerTrack) {
                if (m_singleFileName != "") {
                    cerr << "FileFeatureWriter::setParameters: WARNING: Both one-file and many-files parameters provided, ignoring many-files" << endl;
                } else {
                    m_manyFiles = true;
                }
            }
        } else if (i->first == "one-file") {
            if (m_support & SupportOneFileTotal) {
                if (m_support & ~SupportOneFileTotal) { // not only option
                    if (m_manyFiles) {
                        cerr << "FileFeatureWriter::setParameters: WARNING: Both many-files and one-file parameters provided, ignoring one-file" << endl;
                    } else {
                        m_singleFileName = i->second.c_str();
                    }
                }
            }
        } else if (i->first == "stdout") {
            if (m_support & SupportOneFileTotal) {
                if (m_singleFileName != "") {
                    cerr << "FileFeatureWriter::setParameters: WARNING: Both stdout and one-file provided, ignoring stdout" << endl;
                } else {
                    m_stdout = true;
                }
            }
        } else if (i->first == "append") {
            m_append = true;
        } else if (i->first == "force") {
            m_force = true;
        }
    }
}

QString FileFeatureWriter::getOutputFilename(QString trackId,
                                             TransformId transformId)
{
    if (m_singleFileName != "") {
        if (QFileInfo(m_singleFileName).exists() && !(m_force || m_append)) {
            cerr << "FileFeatureWriter: ERROR: Specified output file \"" << m_singleFileName.toStdString() << "\" exists and neither force nor append flag is specified -- not overwriting" << endl;
            return "";
        }
        return m_singleFileName;
    }

    if (m_stdout) return "";
    
    QUrl url(trackId);
    QString scheme = url.scheme().toLower();
    bool local = (scheme == "" || scheme == "file" || scheme.length() == 1);

    QString dirname, basename;
    QString infilename = url.toLocalFile();
    if (infilename == "") {
        infilename = url.path();
    }
    basename = QFileInfo(infilename).baseName();
    if (scheme.length() == 1) {
        infilename = scheme + ":" + infilename; // DOS drive!
    }

    cerr << "trackId = " << trackId.toStdString() << ", url = " << url.toString().toStdString() << ", infilename = "
         << infilename.toStdString() << ", basename = " << basename.toStdString() << endl;


    if (m_baseDir != "") dirname = QFileInfo(m_baseDir).absoluteFilePath();
    else if (local) dirname = QFileInfo(infilename).absolutePath();
    else dirname = QDir::currentPath();

    QString filename;

    if (m_manyFiles && transformId != "") {
        filename = QString("%1_%2.%3").arg(basename).arg(transformId).arg(m_extension);
    } else {
        filename = QString("%1.%2").arg(basename).arg(m_extension);
    }

    filename.replace(':', '_'); // ':' not permitted in Windows

    filename = QDir(dirname).filePath(filename);

    if (QFileInfo(filename).exists() && !(m_force || m_append)) {
        cerr << "FileFeatureWriter: ERROR: Output file \"" << filename.toStdString() << "\" exists (for input file or URL \"" << trackId.toStdString() << "\" and transform \"" << transformId.toStdString() << "\") and neither force nor append is specified -- not overwriting" << endl;
        return "";
    }
    
    return filename;
}


QFile *FileFeatureWriter::getOutputFile(QString trackId,
                                        TransformId transformId)
{
    pair<QString, TransformId> key;

    if (m_singleFileName != "") {
        key = pair<QString, TransformId>("", "");
    } else if (m_manyFiles) {
        key = pair<QString, TransformId>(trackId, transformId);
    } else {
        key = pair<QString, TransformId>(trackId, "");
    }

    if (m_files.find(key) == m_files.end()) {

        QString filename = getOutputFilename(trackId, transformId);

        if (filename == "") { // stdout
            return 0;
        }

        cerr << "FileFeatureWriter: NOTE: Using output filename \""
             << filename.toStdString() << "\"" << endl;

        QFile *file = new QFile(filename);
        QIODevice::OpenMode mode = (QIODevice::WriteOnly);
        if (m_append) mode |= QIODevice::Append;
                       
        if (!file->open(mode)) {
            cerr << "FileFeatureWriter: ERROR: Failed to open output file \"" << filename.toStdString()
                 << "\" for writing" << endl;
            delete file;
            m_files[key] = 0;
            throw FailedToOpenFile(filename);
        }

        m_files[key] = file;
    }

    return m_files[key];
}


QTextStream *FileFeatureWriter::getOutputStream(QString trackId,
                                               TransformId transformId)
{
    QFile *file = getOutputFile(trackId, transformId);
    if (!file && !m_stdout) {
        return 0;
    }

    if (m_streams.find(file) == m_streams.end()) {
        if (m_stdout) {
            m_streams[file] = new QTextStream(stdout);
        } else {
            m_streams[file] = new QTextStream(file);
        }
    }

    QTextStream *stream = m_streams[file];

    if (m_prevstream && stream != m_prevstream) {
        m_prevstream->flush();
    }
    m_prevstream = stream;

    return stream;
}
            

void
FileFeatureWriter::flush()
{
    if (m_prevstream) {
        m_prevstream->flush();
    }
}

