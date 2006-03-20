/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _XML_EXPORTABLE_H_
#define _XML_EXPORTABLE_H_

#include <QString>
#include <QColor>

class XmlExportable
{
public:
    virtual ~XmlExportable() { }

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const = 0;

    static QString encodeEntities(QString);

    static QString encodeColour(QColor);

    static int getObjectExportId(const void *); // not thread-safe
};

#endif
