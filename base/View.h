/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <QFrame>
#include <QProgressBar>

#include "base/ZoomConstraint.h"
#include "base/PropertyContainer.h"
#include "base/ViewManager.h"
#include "base/XmlExportable.h"

class Layer;

#include <map>

/**
 * View is the base class of widgets that display one or more
 * overlaid views of data against a horizontal time scale. 
 *
 * A View may have any number of attached Layers, each of which
 * is expected to have one data Model (although multiple views may
 * share the same model).
 *
 * A View may be panned in time and zoomed, although the
 * mechanisms for doing so (as well as any other operations and
 * properties available) depend on the subclass.
 */

class View : public QFrame,
	     public PropertyContainer,
	     public XmlExportable
{
    Q_OBJECT

public:
    /**
     * Deleting a View deletes all its views.  However, it is
     * also acceptable for the views to be deleted by other code (in
     * which case they will remove themselves from this View
     * automatically), or to be removed explicitly without deleting
     * using removeLayer.
     */
    virtual ~View();

    /**
     * Retrieve the first visible sample frame on the widget.
     * This is a calculated value based on the centre-frame, widget
     * width and zoom level.  The result may be negative.
     */
    virtual long getStartFrame() const;

    /**
     * Set the widget pan based on the given first visible frame.  The
     * frame value may be negative.
     */
    virtual void setStartFrame(long);

    /**
     * Return the centre frame of the visible widget.  This is an
     * exact value that does not depend on the zoom block size.  Other
     * frame values (start, end) are calculated from this based on the
     * zoom and other factors.
     */
    virtual size_t getCentreFrame() const { return m_centreFrame; }

    /**
     * Set the centre frame of the visible widget.
     */
    virtual void setCentreFrame(size_t f) { setCentreFrame(f, true); }

    /**
     * Retrieve the last visible sample frame on the widget.
     * This is a calculated value based on the centre-frame, widget
     * width and zoom level.
     */
    virtual size_t getEndFrame() const;

    /**
     * Return the zoom level, i.e. the number of frames per pixel.
     */
    virtual int getZoomLevel() const { return m_zoomLevel; }

    /**
     * Set the zoom level, i.e. the number of frames per pixel.  The
     * centre frame will be unchanged; the start and end frames will
     * change.
     */
    virtual void setZoomLevel(size_t z);

    /**
     * Zoom in or out.
     */
    virtual void zoom(bool in);

    virtual void addLayer(Layer *v);
    virtual void removeLayer(Layer *v); // does not delete the layer
    virtual int getLayerCount() const { return m_layers.size(); }

    /**
     * Return a layer, counted in stacking order.  That is, layer 0 is
     * the bottom layer and layer "getLayerCount()-1" is the top one.
     */
    virtual Layer *getLayer(int n) { return m_layers[n]; }

    /**
     * Return the layer last selected by the user.  This is normally
     * the top layer, the same as getLayer(getLayerCount()-1).
     * However, if the user has selected the pane itself more recently
     * than any of the layers on it, this function will return 0.  It
     * will also return 0 if there are no layers.
     */
    virtual Layer *getSelectedLayer();

    virtual void setViewManager(ViewManager *m);

    virtual void setFollowGlobalPan(bool f);
    virtual bool getFollowGlobalPan() const { return m_followPan; }

    virtual void setFollowGlobalZoom(bool f);
    virtual bool getFollowGlobalZoom() const { return m_followZoom; }

    virtual void setLightBackground(bool lb) { m_lightBackground = lb; }
    virtual bool hasLightBackground() const { return m_lightBackground; }

    virtual bool shouldIlluminateLocalFeatures(const Layer *, QPoint &) {
	return false;
    }

    enum PlaybackFollowMode {
	PlaybackScrollContinuous,
	PlaybackScrollPage,
	PlaybackIgnore
    };
    virtual void setPlaybackFollow(PlaybackFollowMode m);
    virtual PlaybackFollowMode getPlaybackFollow() const { return m_followPlay; }

    virtual PropertyList getProperties() const;
    virtual PropertyType getPropertyType(const PropertyName &) const;
    virtual int getPropertyRangeAndValue(const PropertyName &,
					   int *min, int *max) const;
    virtual QString getPropertyValueLabel(const PropertyName &,
					  int value) const;
    virtual void setProperty(const PropertyName &, int value);

    virtual size_t getPropertyContainerCount() const;
    virtual const PropertyContainer *getPropertyContainer(size_t i) const;
    virtual PropertyContainer *getPropertyContainer(size_t i);

    virtual QString getPropertyContainerName() const {
	return objectName();
    }

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const;

signals:
    void propertyContainerAdded(PropertyContainer *pc);
    void propertyContainerRemoved(PropertyContainer *pc);
    void propertyContainerPropertyChanged(PropertyContainer *pc);
    void propertyContainerNameChanged(PropertyContainer *pc);

    void centreFrameChanged(void *, unsigned long, bool);
    void zoomLevelChanged(void *, unsigned long, bool);

public slots:
    virtual void modelChanged();
    virtual void modelChanged(size_t startFrame, size_t endFrame);
    virtual void modelCompletionChanged();
    virtual void modelReplaced();
    virtual void layerParametersChanged();
    virtual void layerNameChanged();

    virtual void viewManagerCentreFrameChanged(void *, unsigned long, bool);
    virtual void viewManagerPlaybackFrameChanged(unsigned long);
    virtual void viewManagerZoomLevelChanged(void *, unsigned long, bool);

    virtual void propertyContainerSelected(PropertyContainer *pc);

    virtual void toolModeChanged();

protected:
    View(QWidget *, bool showProgress);
    virtual void paintEvent(QPaintEvent *e);

    typedef std::vector<Layer *> LayerList;

    size_t getModelsStartFrame() const;
    size_t getModelsEndFrame() const;
    int getModelsSampleRate() const;
    bool areLayersScrollable() const;
    LayerList getScrollableBackLayers(bool &changed) const;
    LayerList getNonScrollableFrontLayers(bool &changed) const;
    size_t getZoomConstraintBlockSize(size_t blockSize,
				      ZoomConstraint::RoundingDirection dir =
				      ZoomConstraint::RoundNearest) const;

    void setCentreFrame(size_t f, bool e);

    void checkProgress(void *object);

    size_t              m_centreFrame;
    int                 m_zoomLevel;
    bool                m_newModel;
    bool                m_followPan;
    bool                m_followZoom;
    PlaybackFollowMode  m_followPlay;
    size_t              m_playPointerFrame;
    bool                m_lightBackground;
    bool                m_showProgress;

    QPixmap            *m_cache;
    size_t              m_cacheCentreFrame;
    int                 m_cacheZoomLevel;

    bool                m_deleting;

    LayerList           m_layers; // I don't own these, but see dtor note above
    bool                m_haveSelectedLayer;

    // caches for use in getScrollableBackLayers, getNonScrollableFrontLayers
    mutable LayerList m_lastScrollableBackLayers;
    mutable LayerList m_lastNonScrollableBackLayers;

    class LayerProgressBar : public QProgressBar {
    public:
	LayerProgressBar(QWidget *parent) : QProgressBar(parent) { }
	virtual QString text() const { return m_text; }
	virtual void setText(QString text) { m_text = text; }
    protected:
	QString m_text;
    };

    typedef std::map<Layer *, LayerProgressBar *> ProgressMap;
    ProgressMap m_progressBars; // I own the ProgressBars

    ViewManager *m_manager; // I don't own this
};

#endif

