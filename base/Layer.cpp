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
#include "PlayParameterRepository.h"

Layer::Layer(View *w) :
    m_dormant(false)
{
    m_view = w;

    // Subclass must call this:
//    w->addLayer(this);
}

Layer::~Layer()
{
    m_view->removeLayer(this);
}

QString
Layer::getPropertyContainerIconName() const
{
    return LayerFactory::instance()->getLayerIconName
	(LayerFactory::instance()->getLayerType(this));
}

void
Layer::setObjectName(const QString &name)
{
    QObject::setObjectName(name);
    emit layerNameChanged();
}

int
Layer::getXForFrame(long frame) const
{
    if (m_view) return m_view->getXForFrame(frame);
    else return 0;
}

long
Layer::getFrameForX(int x) const
{
    if (m_view) return m_view->getFrameForX(x);
    else return 0;
}

QString
Layer::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;
    
    s += indent;

    s += QString("<layer id=\"%2\" type=\"%1\" name=\"%3\" model=\"%4\" %5/>\n")
	.arg(LayerFactory::instance()->getLayerTypeName
	     (LayerFactory::instance()->getLayerType(this)))
	.arg(getObjectExportId(this))
	.arg(objectName())
	.arg(getObjectExportId(getModel()))
	.arg(extraAttributes);

    return s;
}

PlayParameters *
Layer::getPlayParameters() 
{
    std::cerr << "Layer (" << this << ")::getPlayParameters: model is "<< getModel() << std::endl;
    const Model *model = getModel();
    if (model) {
	return PlayParameterRepository::instance()->getPlayParameters(model);
    }
    return 0;
}

void
Layer::showLayer(bool show)
{
    setLayerDormant(!show);
    emit layerParametersChanged();
}


#ifdef INCLUDE_MOCFILES
#include "Layer.moc.cpp"
#endif

