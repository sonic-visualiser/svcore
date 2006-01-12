/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "XmlExportable.h"

QString
XmlExportable::encodeEntities(QString s)
{
    s
	.replace("&", "&amp;")
	.replace("<", "&lt;")
	.replace(">", "&gt;")
	.replace("\"", "&quot;")
	.replace("'", "&apos;");

    return s;
}

QString
XmlExportable::encodeColour(QColor c)
{
    QString r, g, b;

    r.setNum(c.red(), 16);
    if (c.red() < 16) r = "0" + r;

    g.setNum(c.green(), 16);
    if (c.green() < 16) g = "0" + g;

    b.setNum(c.blue(), 16);
    if (c.blue() < 16) b = "0" + b;

    return "#" + r + g + b;
}

