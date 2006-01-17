/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "Layer.h"
#include "View.h"

#include <iostream>

#include "layer/LayerFactory.h" //!!! shouldn't be including this here -- does that suggest we need to move this into layer/ ?

Layer::Layer(View *w)
{
    m_view = w;

    // Subclass must call this:
//    w->addLayer(this);
}

Layer::~Layer()
{
    m_view->removeLayer(this);
}

void
Layer::setObjectName(const QString &name)
{
    QObject::setObjectName(name);
    emit layerNameChanged();
}

QString
Layer::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    
    s += indent;

    s += QString("<layer id=\"%2\" type=\"%1\" name=\"%3\" model=\"%4\" %5>\n")
	.arg(LayerFactory::instance()->getLayerTypeName
	     (LayerFactory::instance()->getLayerType(this)))
	.arg(getObjectExportId(this))
	.arg(objectName())
	.arg(getObjectExportId(getModel()))
	.arg(extraAttributes);

    PropertyList properties = getProperties();

    for (PropertyList::const_iterator i = properties.begin();
	 i != properties.end(); ++i) {

	int pv = getPropertyRangeAndValue(*i, 0, 0);
	s += indent + "";
	s += QString("<property name=\"%1\" value=\"%2\"/>\n").arg(*i).arg(pv);
    }

    s += indent + "</layer>\n";

    return s;
}

#ifdef INCLUDE_MOCFILES
#include "Layer.moc.cpp"
#endif

