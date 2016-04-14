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

#ifndef PLUGIN_SCAN_H
#define PLUGIN_SCAN_H

#include <QStringList>

#include "../vamp-plugin-load-checker/knownplugins.h" //!!!

class PluginScan : public PluginCandidates::LogCallback
{
public:
    static PluginScan *getInstance();

    void scan();

    bool scanSucceeded() const;
    
    QStringList getCandidateVampLibraries() const;
    QStringList getCandidateLADSPALibraries() const;
    QStringList getCandidateDSSILibraries() const;
    QStringList getCandidateLibrariesFor(KnownPlugins::PluginType) const;

    QString getStartupFailureReport() const;

protected:
    void log(std::string);

private:
    PluginScan();
    ~PluginScan();
    KnownPlugins *m_kp;
    bool m_succeeded;
};

#endif
