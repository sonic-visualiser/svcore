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

#include "ProgressPrinter.h"

#include <iostream>

ProgressPrinter::ProgressPrinter(QString prefix, QObject *parent) :
    QObject(parent),
    m_prefix(prefix),
    m_lastProgress(0)
{
}

ProgressPrinter::~ProgressPrinter()
{
    if (m_lastProgress > 0 && m_lastProgress != 100) {
        std::cerr << "\r\n";
    }
    std::cerr << "(progress printer dtor)" << std::endl;
}

void
ProgressPrinter::progress(int progress)
{
    if (progress == m_lastProgress) return;
    if (progress == 100) std::cerr << "\r\n";
    else {
        std::cerr << "\r"
                  << m_prefix.toStdString() 
                  << (m_prefix == "" ? "" : " ")
                  << progress << "%";
    }
    m_lastProgress = progress;
}

