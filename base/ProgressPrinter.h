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

#ifndef SV_PROGRESS_PRINTER_H
#define SV_PROGRESS_PRINTER_H

#include "ProgressReporter.h"

namespace sv {

class ProgressPrinter : public ProgressReporter
{
    Q_OBJECT
    
public:
    ProgressPrinter(QString message, QObject *parent = 0);
    virtual ~ProgressPrinter();
    
    bool isDefinite() const override;
    void setDefinite(bool definite) override;

    bool wasCancelled() const override { return false; } // no mechanism

public slots:
    void setMessage(QString) override;
    void setProgress(int) override;
    virtual void done();

protected:
    QString m_prefix;
    int m_lastProgress;
    bool m_definite;
};

} // end namespace sv

#endif
