/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#include "PluginIdentifier.h"
#include <iostream>

QString
PluginIdentifier::createIdentifier(QString type,
				   QString soName,
				   QString label)
{
    QString identifier = type + ":" + soName + ":" + label;
    return identifier;
}

void
PluginIdentifier::parseIdentifier(QString identifier,
				  QString &type,
				  QString &soName,
				  QString &label)
{
    type = identifier.section(':', 0, 0);
    soName = identifier.section(':', 1, 1);
    label = identifier.section(':', 2);
}

bool
PluginIdentifier::areIdentifiersSimilar(QString id1, QString id2)
{
    QString type1, type2, soName1, soName2, label1, label2;

    parseIdentifier(id1, type1, soName1, label1);
    parseIdentifier(id2, type2, soName2, label2);

    if (type1 != type2 || label1 != label2) return false;

    bool similar = (soName1.section('/', -1).section('.', 0, 0) ==
		    soName2.section('/', -1).section('.', 0, 0));

    return similar;
}

QString
PluginIdentifier::BUILTIN_PLUGIN_SONAME = "_builtin";

QString
PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY = "__QMUL__:__RESERVED__:ProjectDirectoryKey";

