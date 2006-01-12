/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2005 Chris Cannam.
*/

#ifndef _PLUGIN_IDENTIFIER_H_
#define _PLUGIN_IDENTIFIER_H_

#include <QString>


// A plugin identifier is simply a string; this class provides methods
// to parse it into its constituent bits (plugin type, DLL path and label).

class PluginIdentifier {

public:
 
    static QString createIdentifier(QString type, QString soName, QString label);

    static void parseIdentifier(QString identifier,
				QString &type, QString &soName, QString &label);

    static bool areIdentifiersSimilar(QString id1, QString id2);

    // Fake soName for use with plugins that are actually compiled in
    static QString BUILTIN_PLUGIN_SONAME;

    // Not strictly related to identifiers
    static QString RESERVED_PROJECT_DIRECTORY_KEY;
};

#endif
