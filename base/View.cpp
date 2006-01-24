/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#include "base/View.h"
#include "base/Layer.h"
#include "base/Model.h"
#include "base/ZoomConstraint.h"
#include "base/Profiler.h"

#include "layer/TimeRulerLayer.h" //!!! damn, shouldn't be including that here

#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QApplication>

#include <iostream>

//#define DEBUG_VIEW_WIDGET_PAINT 1

using std::cerr;
using std::endl;

View::View(QWidget *w, bool showProgress) :
    QFrame(w),
    m_centreFrame(0),
    m_zoomLevel(1024),
    m_newModel(true),
    m_followPan(true),
    m_followZoom(true),
    m_followPlay(PlaybackScrollPage),
    m_lightBackground(true),
    m_showProgress(showProgress),
    m_cache(0),
    m_cacheCentreFrame(0),
    m_cacheZoomLevel(1024),
    m_selectionCached(false),
    m_deleting(false),
    m_haveSelectedLayer(false),
    m_manager(0)
{
//    QWidget::setAttribute(Qt::WA_PaintOnScreen);
}

View::~View()
{
    m_deleting = true;

    for (LayerList::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	delete *i;
    }
}

PropertyContainer::PropertyList
View::getProperties() const
{
    PropertyList list;
    list.push_back(tr("Global Scroll"));
    list.push_back(tr("Global Zoom"));
    list.push_back(tr("Follow Playback"));
    return list;
}

PropertyContainer::PropertyType
View::getPropertyType(const PropertyName &name) const
{
    if (name == tr("Global Scroll")) return ToggleProperty;
    if (name == tr("Global Zoom")) return ToggleProperty;
    if (name == tr("Follow Playback")) return ValueProperty;
    return InvalidProperty;
}

int
View::getPropertyRangeAndValue(const PropertyName &name,
				       int *min, int *max) const
{
    if (name == tr("Global Scroll")) return m_followPan;
    if (name == tr("Global Zoom")) return m_followZoom;
    if (name == tr("Follow Playback")) { *min = 0; *max = 2; return int(m_followPlay); }
    return PropertyContainer::getPropertyRangeAndValue(name, min, max);
}

QString
View::getPropertyValueLabel(const PropertyName &name,
				  int value) const
{
    if (name == tr("Follow Playback")) {
	switch (value) {
	default:
	case 0: return tr("Scroll");
	case 1: return tr("Page");
	case 2: return tr("Off");
	}
    }
    return tr("<unknown>");
}

void
View::setProperty(const PropertyName &name, int value)
{
    if (name == tr("Global Scroll")) {
	setFollowGlobalPan(value != 0);
    } else if (name == tr("Global Zoom")) {
	setFollowGlobalZoom(value != 0);
    } else if (name == tr("Follow Playback")) {
	switch (value) {
	default:
	case 0: setPlaybackFollow(PlaybackScrollContinuous); break;
	case 1: setPlaybackFollow(PlaybackScrollPage); break;
	case 2: setPlaybackFollow(PlaybackIgnore); break;
	}
    }
}

size_t
View::getPropertyContainerCount() const
{
    return m_layers.size() + 1; // the 1 is for me
}

const PropertyContainer *
View::getPropertyContainer(size_t i) const
{
    return (const PropertyContainer *)(((View *)this)->
				       getPropertyContainer(i));
}

PropertyContainer *
View::getPropertyContainer(size_t i)
{
    if (i == 0) return this;
    return m_layers[i-1];
}

void
View::propertyContainerSelected(PropertyContainer *pc)
{
    if (pc == this) {
	if (m_haveSelectedLayer) {
	    m_haveSelectedLayer = false;
	    update();
	}
	return;
    }

    delete m_cache;
    m_cache = 0;

    Layer *selectedLayer = 0;

    for (LayerList::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if (*i == pc) {
	    selectedLayer = *i;
	    m_layers.erase(i);
	    break;
	}
    }

    if (selectedLayer) {
	m_haveSelectedLayer = true;
	m_layers.push_back(selectedLayer);
	update();
    } else {
	m_haveSelectedLayer = false;
    }
}

void
View::toolModeChanged()
{
    std::cerr << "View::toolModeChanged(" << m_manager->getToolMode() << ")" << std::endl;
}

long
View::getStartFrame() const
{
    size_t w2 = (width() / 2) * m_zoomLevel;
    size_t frame = m_centreFrame;
    if (frame >= w2) {
	frame -= w2;
	return (frame / m_zoomLevel * m_zoomLevel);
    } else {
	frame = w2 - frame;
	frame = frame / m_zoomLevel * m_zoomLevel;
	return -(long)frame - m_zoomLevel;
    }
}

size_t
View::getEndFrame() const
{
    return getStartFrame() + (width() * m_zoomLevel) - 1;
}

void
View::setStartFrame(long f)
{
    setCentreFrame(f + m_zoomLevel * (width() / 2));
}

void
View::setCentreFrame(size_t f, bool e)
{
    if (m_centreFrame != f) {

	int formerPixel = m_centreFrame / m_zoomLevel;

	m_centreFrame = f;

	int newPixel = m_centreFrame / m_zoomLevel;
	
	if (newPixel != formerPixel) {

#ifdef DEBUG_VIEW_WIDGET_PAINT
	    std::cout << "View(" << this << ")::setCentreFrame: newPixel " << newPixel << ", formerPixel " << formerPixel << std::endl;
#endif
	    update();
	}

	if (e) emit centreFrameChanged(this, f, m_followPan);
    }
}

void
View::setZoomLevel(size_t z)
{
    if (m_zoomLevel != int(z)) {
	m_zoomLevel = z;
	emit zoomLevelChanged(this, z, m_followZoom);
	update();
    }
}

void
View::addLayer(Layer *layer)
{
    delete m_cache;
    m_cache = 0;

    m_layers.push_back(layer);

    m_progressBars[layer] = new LayerProgressBar(this);
    m_progressBars[layer]->setMinimum(0);
    m_progressBars[layer]->setMaximum(100);
    m_progressBars[layer]->setMinimumWidth(80);
    m_progressBars[layer]->hide();

    connect(layer, SIGNAL(layerParametersChanged()),
	    this,    SLOT(layerParametersChanged()));
    connect(layer, SIGNAL(layerNameChanged()),
	    this,    SLOT(layerNameChanged()));
    connect(layer, SIGNAL(modelChanged()),
	    this,    SLOT(modelChanged()));
    connect(layer, SIGNAL(modelCompletionChanged()),
	    this,    SLOT(modelCompletionChanged()));
    connect(layer, SIGNAL(modelChanged(size_t, size_t)),
	    this,    SLOT(modelChanged(size_t, size_t)));
    connect(layer, SIGNAL(modelReplaced()),
	    this,    SLOT(modelReplaced()));

    m_newModel = true;
    update();

    emit propertyContainerAdded(layer);
}

void
View::removeLayer(Layer *layer)
{
    if (m_deleting) {
	return;
    }

    delete m_cache;
    m_cache = 0;

    for (LayerList::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if (*i == layer) {
	    m_layers.erase(i);
	    if (m_progressBars.find(layer) != m_progressBars.end()) {
		delete m_progressBars[layer];
		m_progressBars.erase(layer);
	    }
	    break;
	}
    }
    
    update();

    emit propertyContainerRemoved(layer);
}

Layer *
View::getSelectedLayer()
{
    if (m_haveSelectedLayer && !m_layers.empty()) {
	return getLayer(getLayerCount() - 1);
    } else {
	return 0;
    }
}

void
View::setViewManager(ViewManager *manager)
{
    if (m_manager) {
	m_manager->disconnect(this, SLOT(viewManagerCentreFrameChanged(void *, unsigned long, bool)));
	m_manager->disconnect(this, SLOT(viewManagerZoomLevelChanged(void *, unsigned long, bool)));
	disconnect(m_manager, SIGNAL(centreFrameChanged(void *, unsigned long, bool)));
	disconnect(m_manager, SIGNAL(zoomLevelChanged(void *, unsigned long, bool)));
	disconnect(m_manager, SIGNAL(toolModeChanged()));
	disconnect(m_manager, SIGNAL(selectionChanged()));
	disconnect(m_manager, SIGNAL(inProgressSelectionChanged()));
    }

    m_manager = manager;
    if (m_followPan) setCentreFrame(m_manager->getGlobalCentreFrame(), false);
    if (m_followZoom) setZoomLevel(m_manager->getGlobalZoom());

    connect(m_manager, SIGNAL(centreFrameChanged(void *, unsigned long, bool)),
	    this, SLOT(viewManagerCentreFrameChanged(void *, unsigned long, bool)));
    connect(m_manager, SIGNAL(playbackFrameChanged(unsigned long)),
	    this, SLOT(viewManagerPlaybackFrameChanged(unsigned long)));
    connect(m_manager, SIGNAL(zoomLevelChanged(void *, unsigned long, bool)),
	    this, SLOT(viewManagerZoomLevelChanged(void *, unsigned long, bool)));
    connect(m_manager, SIGNAL(toolModeChanged()),
	    this, SLOT(toolModeChanged()));
    connect(m_manager, SIGNAL(selectionChanged()),
	    this, SLOT(selectionChanged()));
    connect(m_manager, SIGNAL(inProgressSelectionChanged()),
	    this, SLOT(selectionChanged()));

    connect(this, SIGNAL(centreFrameChanged(void *, unsigned long, bool)),
	    m_manager, SIGNAL(centreFrameChanged(void *, unsigned long, bool)));
    connect(this, SIGNAL(zoomLevelChanged(void *, unsigned long, bool)),
	    m_manager, SIGNAL(zoomLevelChanged(void *, unsigned long, bool)));

    toolModeChanged();
}

void
View::setFollowGlobalPan(bool f)
{
    m_followPan = f;
    emit propertyContainerPropertyChanged(this);
}

void
View::setFollowGlobalZoom(bool f)
{
    m_followZoom = f;
    emit propertyContainerPropertyChanged(this);
}

void
View::setPlaybackFollow(PlaybackFollowMode m)
{
    m_followPlay = m;
    emit propertyContainerPropertyChanged(this);
}

void
View::modelChanged()
{
    QObject *obj = sender();

#ifdef DEBUG_VIEW_WIDGET_PAINT
    std::cerr << "View(" << this << ")::modelChanged()" << std::endl;
#endif
    delete m_cache;
    m_cache = 0;

    checkProgress(obj);

    update();
}

void
View::modelChanged(size_t startFrame, size_t endFrame)
{
    QObject *obj = sender();

    long myStartFrame = getStartFrame();
    size_t myEndFrame = getEndFrame();

#ifdef DEBUG_VIEW_WIDGET_PAINT
    std::cerr << "View(" << this << ")::modelChanged(" << startFrame << "," << endFrame << ") [me " << myStartFrame << "," << myEndFrame << "]" << std::endl;
#endif

    if (myStartFrame > 0 && endFrame < size_t(myStartFrame)) {
	checkProgress(obj);
	return;
    }
    if (startFrame > myEndFrame) {
	checkProgress(obj);
	return;
    }

    delete m_cache;
    m_cache = 0;

    if (long(startFrame) < myStartFrame) startFrame = myStartFrame;
    if (endFrame > myEndFrame) endFrame = myEndFrame;

    int x0 = (startFrame - myStartFrame) / m_zoomLevel;
    int x1 = (endFrame - myStartFrame) / m_zoomLevel;
    if (x1 < x0) return;

    checkProgress(obj);

    update(x0, 0, x1 - x0 + 1, height());
}    

void
View::modelCompletionChanged()
{
    QObject *obj = sender();
    checkProgress(obj);
}

void
View::modelReplaced()
{
#ifdef DEBUG_VIEW_WIDGET_PAINT
    std::cerr << "View(" << this << ")::modelReplaced()" << std::endl;
#endif
    delete m_cache;
    m_cache = 0;

    m_newModel = true;
    update();
}

void
View::layerParametersChanged()
{
    Layer *layer = dynamic_cast<Layer *>(sender());

#ifdef DEBUG_VIEW_WIDGET_PAINT
    std::cerr << "View::layerParametersChanged()" << std::endl;
#endif

    delete m_cache;
    m_cache = 0;
    update();

    if (layer) {
	emit propertyContainerPropertyChanged(layer);
    }
}

void
View::layerNameChanged()
{
    Layer *layer = dynamic_cast<Layer *>(sender());
    if (layer) emit propertyContainerNameChanged(layer);
}

void
View::viewManagerCentreFrameChanged(void *p, unsigned long f, bool locked)
{
    if (m_followPan && p != this && locked) {
	if (m_manager && (sender() == m_manager)) {
#ifdef DEBUG_VIEW_WIDGET_PAINT
	    std::cerr << this << ": manager frame changed " << f << " from " << p << std::endl;
#endif
	    setCentreFrame(f);
	    if (p == this) repaint();
	}
    }
}

void
View::viewManagerPlaybackFrameChanged(unsigned long f)
{
    if (m_manager) {
	if (sender() != m_manager) return;
    }

    if (m_playPointerFrame == f) return;
    bool visible = (m_playPointerFrame / m_zoomLevel != f / m_zoomLevel);
    size_t oldPlayPointerFrame = m_playPointerFrame;
    m_playPointerFrame = f;
    if (!visible) return;

    switch (m_followPlay) {

    case PlaybackScrollContinuous:
	if (QApplication::mouseButtons() == Qt::NoButton) {
	    setCentreFrame(f, false);
	}
	break;

    case PlaybackScrollPage:
    { 
	long w = width() * getZoomLevel();
	w -= w/5;
	long sf = (f / w) * w - w/8;
#ifdef DEBUG_VIEW_WIDGET_PAINT
	std::cerr << "PlaybackScrollPage: f = " << f << ", sf = " << sf << ", start frame "
		  << getStartFrame() << std::endl;
#endif
	setCentreFrame(sf + width() * getZoomLevel() / 2, false);
	int xold = (long(oldPlayPointerFrame) - getStartFrame()) / m_zoomLevel;
	int xnew = (long(m_playPointerFrame) - getStartFrame()) / m_zoomLevel;
	update(xold - 1, 0, 3, height());
	update(xnew - 1, 0, 3, height());
	break;
    }

    case PlaybackIgnore:
	if (long(f) >= getStartFrame() && f < getEndFrame()) {
	    update();
	}
	break;
    }
}

void
View::viewManagerZoomLevelChanged(void *p, unsigned long z, bool locked)
{
    if (m_followZoom && p != this && locked) {
	if (m_manager && (sender() == m_manager)) {
	    setZoomLevel(z);
	    if (p == this) repaint();
	}
    }
}

void
View::selectionChanged()
{
    if (m_selectionCached) {
	delete m_cache;
	m_cache = 0;
	m_selectionCached = false;
    }
    update();
}

size_t
View::getModelsStartFrame() const
{
    bool first = true;
    size_t startFrame = 0;

    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {

	if ((*i)->getModel() && (*i)->getModel()->isOK()) {

	    size_t thisStartFrame = (*i)->getModel()->getStartFrame();

	    if (first || thisStartFrame < startFrame) {
		startFrame = thisStartFrame;
	    }
	    first = false;
	}
    }
    return startFrame;
}

size_t
View::getModelsEndFrame() const
{
    bool first = true;
    size_t endFrame = 0;

    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {

	if ((*i)->getModel() && (*i)->getModel()->isOK()) {

	    size_t thisEndFrame = (*i)->getModel()->getEndFrame();

	    if (first || thisEndFrame > endFrame) {
		endFrame = thisEndFrame;
	    }
	    first = false;
	}
    }

    if (first) return getModelsStartFrame();
    return endFrame;
}

int
View::getModelsSampleRate() const
{
    //!!! Just go for the first, for now.  If we were supporting
    // multiple samplerates, we'd probably want to do frame/time
    // conversion in the model

    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if ((*i)->getModel() && (*i)->getModel()->isOK()) {
	    return (*i)->getModel()->getSampleRate();
	}
    }
    return 0;
}

bool
View::areLayersScrollable() const
{
    // True iff all views are scrollable
    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if (!(*i)->isLayerScrollable()) return false;
    }
    return true;
}

View::LayerList
View::getScrollableBackLayers(bool &changed) const
{
    changed = false;

    LayerList scrollables;
    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if ((*i)->isLayerScrollable()) scrollables.push_back(*i);
	else {
	    if (scrollables != m_lastScrollableBackLayers) {
		m_lastScrollableBackLayers = scrollables;
		changed = true;
	    }
	    return scrollables;
	}
    }

    if (scrollables.size() == 1 &&
	dynamic_cast<TimeRulerLayer *>(*scrollables.begin())) {

	// If only the ruler is scrollable, it's not worth the bother
	// -- it probably redraws as quickly as it refreshes from
	// cache
	scrollables.clear();
    }

    if (scrollables != m_lastScrollableBackLayers) {
	m_lastScrollableBackLayers = scrollables;
	changed = true;
    }
    return scrollables;
}

View::LayerList
View::getNonScrollableFrontLayers(bool &changed) const
{
    changed = false;
    LayerList scrollables = getScrollableBackLayers(changed);
    LayerList nonScrollables;

    // Everything in front of the first non-scrollable from the back
    // should also be considered non-scrollable

    size_t count = 0;
    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
	if (count < scrollables.size()) {
	    ++count;
	    continue;
	}
	nonScrollables.push_back(*i);
    }

    if (nonScrollables != m_lastNonScrollableBackLayers) {
	m_lastNonScrollableBackLayers = nonScrollables;
	changed = true;
    }

    return nonScrollables;
}

size_t
View::getZoomConstraintBlockSize(size_t blockSize,
				 ZoomConstraint::RoundingDirection dir)
    const
{
    size_t candidate = blockSize;
    bool haveCandidate = false;

    for (LayerList::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {

	if ((*i)->getZoomConstraint()) {

	    size_t thisBlockSize = 
		(*i)->getZoomConstraint()->getNearestBlockSize
		(blockSize, dir);

	    // Go for the block size that's furthest from the one
	    // passed in.  Most of the time, that's what we want.
	    if (!haveCandidate ||
		(thisBlockSize > blockSize && thisBlockSize > candidate) ||
		(thisBlockSize < blockSize && thisBlockSize < candidate)) {
		candidate = thisBlockSize;
		haveCandidate = true;
	    }
	}
    }

    return candidate;
}

void
View::zoom(bool in)
{
    int newZoomLevel = m_zoomLevel;

    if (in) {
	newZoomLevel = getZoomConstraintBlockSize(newZoomLevel - 1, 
						  ZoomConstraint::RoundDown);
    } else {
	newZoomLevel = getZoomConstraintBlockSize(newZoomLevel + 1,
						  ZoomConstraint::RoundUp);
    }

    if (newZoomLevel != m_zoomLevel) {
	setZoomLevel(newZoomLevel);
    }
}

void
View::checkProgress(void *object)
{
//    std::cerr << "View::checkProgress(" << object << ")" << std::endl;

    if (!m_showProgress) return;

    int ph = height();

    for (ProgressMap::const_iterator i = m_progressBars.begin();
	 i != m_progressBars.end(); ++i) {

	if (i->first == object) {

	    int completion = i->first->getCompletion();

	    if (completion >= 100) {

		i->second->hide();

	    } else {

		i->second->setText(i->first->getPropertyContainerName());
		i->second->setValue(completion);
		i->second->move(0, ph - i->second->height());

		i->second->show();
		i->second->update();

		ph -= i->second->height();
	    }
	} else {
	    if (i->second->isVisible()) {
		ph -= i->second->height();
	    }
	}
    }
}
/*!!!
void
View::identifyLocalFeatures(bool on, int x, int y)
{
    for (LayerList::const_iterator i = m_layers.end(); i != m_layers.begin(); ) {
	--i;
#ifdef DEBUG_VIEW_WIDGET_PAINT
	std::cerr << "View::identifyLocalFeatures: calling on " << *i << std::endl;
#endif
	if ((*i)->identifyLocalFeatures(on, x, y)) break;
    }
}
*/

void
View::paintEvent(QPaintEvent *e)
{
//    Profiler prof("View::paintEvent", true);
//    std::cerr << "View::paintEvent" << std::endl;

    if (m_layers.empty()) {
	QFrame::paintEvent(e);
	return;
    }

    if (m_newModel) {
	m_newModel = false;
    }

    // ensure our constraints are met
    m_zoomLevel = getZoomConstraintBlockSize(m_zoomLevel,
					     ZoomConstraint::RoundUp);

    QPainter paint;
    bool repaintCache = false;
    bool paintedCacheRect = false;

    QRect cacheRect(rect());

    if (e) {
	cacheRect &= e->rect();
#ifdef DEBUG_VIEW_WIDGET_PAINT
	std::cerr << "paint rect " << cacheRect.width() << "x" << cacheRect.height()
		  << ", my rect " << width() << "x" << height() << std::endl;
#endif
    }

    QRect nonCacheRect(cacheRect);

    // If not all layers are scrollable, but some of the back layers
    // are, we should store only those in the cache

    bool layersChanged = false;
    LayerList scrollables = getScrollableBackLayers(layersChanged);
    LayerList nonScrollables = getNonScrollableFrontLayers(layersChanged);
    bool selectionCacheable = nonScrollables.empty();
    bool haveSelections = m_manager && !m_manager->getSelections().empty();
    bool selectionDrawn = false;

#ifdef DEBUG_VIEW_WIDGET_PAINT
    std::cerr << "View(" << this << ")::paintEvent: have " << scrollables.size()
	      << " scrollable back layers and " << nonScrollables.size()
	      << " non-scrollable front layers" << std::endl;
    std::cerr << "haveSelections " << haveSelections << ", selectionCacheable "
	      << selectionCacheable << ", m_selectionCached " << m_selectionCached << std::endl;
#endif

    if (layersChanged || scrollables.empty() ||
	(haveSelections && (selectionCacheable != m_selectionCached))) {
	delete m_cache;
	m_cache = 0;
	m_selectionCached = false;
    }

    if (!scrollables.empty()) {
	if (!m_cache ||
	    m_cacheZoomLevel != m_zoomLevel ||
	    width() != m_cache->width() ||
	    height() != m_cache->height()) {

	    // cache is not valid

	    if (cacheRect.width() < width()/10) {
#ifdef DEBUG_VIEW_WIDGET_PAINT
		std::cerr << "View(" << this << ")::paintEvent: small repaint, not bothering to recreate cache" << std::endl;
#endif
	    } else {
		delete m_cache;
		m_cache = new QPixmap(width(), height());
#ifdef DEBUG_VIEW_WIDGET_PAINT
		std::cerr << "View(" << this << ")::paintEvent: recreated cache" << std::endl;
#endif
		cacheRect = rect();
		repaintCache = true;
	    }

	} else if (m_cacheCentreFrame != m_centreFrame) {

	    long dx = long(m_cacheCentreFrame / m_zoomLevel) -
		long(m_centreFrame / m_zoomLevel);

	    if (dx > -width() && dx < width()) {
#if defined(Q_WS_WIN32) || defined(Q_WS_MAC)
		// Copying a pixmap to itself doesn't work properly on Windows
		// or Mac (it only works when moving in one direction)
		static QPixmap *tmpPixmap = 0;
		if (!tmpPixmap ||
		    tmpPixmap->width() != width() ||
		    tmpPixmap->height() != height()) {
		    delete tmpPixmap;
		    tmpPixmap = new QPixmap(width(), height());
		}
		paint.begin(tmpPixmap);
		paint.drawPixmap(0, 0, *m_cache);
		paint.end();
		paint.begin(m_cache);
		paint.drawPixmap(dx, 0, *tmpPixmap);
		paint.end();
#else
		// But it seems to be fine on X11
		paint.begin(m_cache);
		paint.drawPixmap(dx, 0, *m_cache);
		paint.end();
#endif

		if (dx < 0) {
		    cacheRect = QRect(width() + dx, 0, -dx, height());
		} else {
		    cacheRect = QRect(0, 0, dx, height());
		}
#ifdef DEBUG_VIEW_WIDGET_PAINT
		std::cerr << "View(" << this << ")::paintEvent: scrolled cache by " << dx << std::endl;
#endif
	    } else {
		cacheRect = rect();
#ifdef DEBUG_VIEW_WIDGET_PAINT
		std::cerr << "View(" << this << ")::paintEvent: scrolling too far" << std::endl;
#endif
	    }
	    repaintCache = true;

	} else {
#ifdef DEBUG_VIEW_WIDGET_PAINT
	    std::cerr << "View(" << this << ")::paintEvent: cache is good" << std::endl;
#endif
	    paint.begin(this);
	    paint.drawPixmap(cacheRect, *m_cache, cacheRect);
	    paint.end();
	    QFrame::paintEvent(e);
	    paintedCacheRect = true;
	}

	m_cacheCentreFrame = m_centreFrame;
	m_cacheZoomLevel = m_zoomLevel;
    }

#ifdef DEBUG_VIEW_WIDGET_PAINT
//    std::cerr << "View(" << this << ")::paintEvent: cacheRect " << cacheRect << ", nonCacheRect " << (nonCacheRect | cacheRect) << ", repaintCache " << repaintCache << ", paintedCacheRect " << paintedCacheRect << std::endl;
#endif

    // Scrollable (cacheable) items first

    if (!paintedCacheRect) {

	if (repaintCache) paint.begin(m_cache);
	else paint.begin(this);

	paint.setClipRect(cacheRect);
	
	if (hasLightBackground()) {
	    paint.setPen(Qt::white);
	    paint.setBrush(Qt::white);
	} else {
	    paint.setPen(Qt::black);
	    paint.setBrush(Qt::black);
	}
	paint.drawRect(cacheRect);

	paint.setPen(Qt::black);
	paint.setBrush(Qt::NoBrush);
	
	for (LayerList::iterator i = scrollables.begin(); i != scrollables.end(); ++i) {
	    paint.setRenderHint(QPainter::Antialiasing, false);
	    paint.save();
	    (*i)->paint(paint, cacheRect);
	    paint.restore();
	}

	if (haveSelections && selectionCacheable) {
	    drawSelections(paint);
	    m_selectionCached = repaintCache;
	    selectionDrawn = true;
	}
	
	paint.end();

	if (repaintCache) {
	    cacheRect |= (e ? e->rect() : rect());
	    paint.begin(this);
	    paint.drawPixmap(cacheRect, *m_cache, cacheRect);
	    paint.end();
	}
    }

    // Now non-cacheable items.  We always need to redraw the
    // non-cacheable items across at least the area we drew of the
    // cacheable items.

    nonCacheRect |= cacheRect;

    paint.begin(this);
    paint.setClipRect(nonCacheRect);

    if (scrollables.empty()) {
	if (hasLightBackground()) {
	    paint.setPen(Qt::white);
	    paint.setBrush(Qt::white);
	} else {
	    paint.setPen(Qt::black);
	    paint.setBrush(Qt::black);
	}
	paint.drawRect(nonCacheRect);
    }
	
    paint.setPen(Qt::black);
    paint.setBrush(Qt::NoBrush);
	
    for (LayerList::iterator i = nonScrollables.begin(); i != nonScrollables.end(); ++i) {
	(*i)->paint(paint, nonCacheRect);
    }
	
    paint.end();

    paint.begin(this);
    if (e) paint.setClipRect(e->rect());
    if (!m_selectionCached) {
	drawSelections(paint);
    }
    paint.end();

    if (m_followPlay != PlaybackScrollContinuous) {

	paint.begin(this);

	if (long(m_playPointerFrame) > getStartFrame() &&
	    m_playPointerFrame < getEndFrame()) {

	    int playx = (long(m_playPointerFrame) - getStartFrame()) /
		m_zoomLevel;

	    paint.setPen(Qt::black);
	    paint.drawLine(playx - 1, 0, playx - 1, height() - 1);
	    paint.drawLine(playx + 1, 0, playx + 1, height() - 1);
	    paint.drawPoint(playx, 0);
	    paint.drawPoint(playx, height() - 1);
	    paint.setPen(Qt::white);
	    paint.drawLine(playx, 1, playx, height() - 2);
	}

	paint.end();
    }

    QFrame::paintEvent(e);
}

void
View::drawSelections(QPainter &paint)
{
    ViewManager::SelectionList selections;

    if (m_manager) {
	selections = m_manager->getSelections();
	if (m_manager->haveInProgressSelection()) {
	    bool exclusive;
	    Selection inProgressSelection =
		m_manager->getInProgressSelection(exclusive);
	    if (exclusive) selections.clear();
	    selections.insert(inProgressSelection);
	}
    }

    paint.save();
    paint.setPen(QColor(150, 150, 255));
    paint.setBrush(QColor(150, 150, 255, 80));

    for (ViewManager::SelectionList::iterator i = selections.begin();
	 i != selections.end(); ++i) {

	int p0 = -1, p1 = -1;

	if (int(i->getStartFrame()) >= getStartFrame()) {
	    p0 = (i->getStartFrame() - getStartFrame()) / m_zoomLevel;
	}

	if (int(i->getEndFrame()) >= getStartFrame()) {
	    p1 = (i->getEndFrame() - getStartFrame()) / m_zoomLevel;
	}

	if (p0 == -1 && p1 == -1) continue;

	if (p1 > width()) p1 = width() + 1;

#ifdef DEBUG_VIEW_WIDGET_PAINT
	std::cerr << "View::drawSelections: " << p0 << ",-1 [" << (p1-p0) << "x" << (height()+1) << "]" << std::endl;
#endif

	paint.drawRect(p0, -1, p1 - p0, height() + 1);
    }

    paint.restore();
}

QString
View::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;

    s += indent;

    s += QString("<view "
		 "centre=\"%1\" "
		 "zoom=\"%2\" "
		 "followPan=\"%3\" "
		 "followZoom=\"%4\" "
		 "tracking=\"%5\" "
		 "light=\"%6\" %7>\n")
	.arg(m_centreFrame)
	.arg(m_zoomLevel)
	.arg(m_followPan)
	.arg(m_followZoom)
	.arg(m_followPlay == PlaybackScrollContinuous ? "scroll" :
	     m_followPlay == PlaybackScrollPage ? "page" : "ignore")
	.arg(m_lightBackground)
	.arg(extraAttributes);

    for (size_t i = 0; i < m_layers.size(); ++i) {
	s += m_layers[i]->toXmlString(indent + "  ");
    }

    s += indent + "</view>\n";

    return s;
}


#ifdef INCLUDE_MOCFILES
#include "View.moc.cpp"
#endif

