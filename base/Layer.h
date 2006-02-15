
/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _LAYER_H_
#define _LAYER_H_

#include "PropertyContainer.h"
#include "XmlExportable.h"

#include <QObject>
#include <QRect>
#include <QXmlAttributes>

class ZoomConstraint;
class Model;
class QPainter;
class View;
class QMouseEvent;

/**
 * The base class for visual representations of the data found in a
 * Model.  Layers are expected to be able to draw themselves onto a
 * View, and may also be editable.
 */

class Layer : public QObject,
	      public PropertyContainer,
	      public XmlExportable
{
    Q_OBJECT

public:
    Layer(View *w);
    virtual ~Layer();

    virtual const Model *getModel() const = 0;
    virtual const ZoomConstraint *getZoomConstraint() const { return 0; }
    virtual void paint(QPainter &, QRect) const = 0;   

    enum VerticalPosition {
	PositionTop, PositionMiddle, PositionBottom
    };
    virtual VerticalPosition getPreferredTimeRulerPosition() const {
	return PositionMiddle;
    }
    virtual VerticalPosition getPreferredFrameCountPosition() const {
	return PositionBottom;
    }

    virtual QString getPropertyContainerIconName() const;

    virtual QString getPropertyContainerName() const {
	return objectName();
    }

    virtual int getVerticalScaleWidth(QPainter &) const { return 0; }
    virtual void paintVerticalScale(QPainter &, QRect) const { }

    virtual QString getFeatureDescription(QPoint &) const {
	return "";
    }

    //!!! We also need a method (like the vertical scale method) for
    //drawing additional scales like a colour scale.  That is, unless
    //all applicable layers can actually do this from
    //paintVerticalScale as well?

    enum SnapType {
	SnapLeft,
	SnapRight,
	SnapNearest,
	SnapNeighbouring
    };

    /**
     * Adjust the given frame to snap to the nearest feature, if
     * possible.
     *
     * If snap is SnapLeft or SnapRight, adjust the frame to match
     * that of the nearest feature in the given direction regardless
     * of how far away it is.  If snap is SnapNearest, adjust the
     * frame to that of the nearest feature in either direction.  If
     * snap is SnapNeighbouring, adjust the frame to that of the
     * nearest feature if it is close, and leave it alone (returning
     * false) otherwise.  SnapNeighbouring should always choose the
     * same feature that would be used in an editing operation through
     * calls to editStart etc.
     *
     * Return true if a suitable feature was found and frame adjusted
     * accordingly.  Return false if no suitable feature was
     * available.  Also return the resolution of the model in this
     * layer in sample frames.
     */
    virtual bool snapToFeatureFrame(int &frame,
				    size_t &resolution,
				    SnapType snap) const {
	resolution = 1;
	return false;
    }

    // Draw and edit modes:
    //
    // Layer needs to get actual mouse events, I guess.  Draw mode is
    // probably the easier.

    virtual void drawStart(QMouseEvent *) { }
    virtual void drawDrag(QMouseEvent *) { }
    virtual void drawEnd(QMouseEvent *) { }

    virtual void editStart(QMouseEvent *) { }
    virtual void editDrag(QMouseEvent *) { }
    virtual void editEnd(QMouseEvent *) { }

    // Text mode:
    //
    // Label nearest feature.  We need to get the feature coordinates
    // and current label from the layer, and then the pane can pop up
    // a little text entry dialog at the right location.  Or we edit
    // in place?  Probably the dialog is easier.

    /**
     * This should return true if the view can safely be scrolled
     * automatically by the widget (simply copying the existing data
     * and then refreshing the exposed area) without altering its
     * meaning.  For the widget as a whole this is usually not
     * possible because of invariant (non-scrolling) material
     * displayed over the top, but the widget may be able to optimise
     * scrolling better if it is known that individual views can be
     * scrolled safely in this way.
     */
    virtual bool isLayerScrollable() const { return true; }

    /**
     * This should return true if the layer completely obscures any
     * underlying layers.  It's used to determine whether the view can
     * safely draw any selection rectangles under the layer instead of
     * over it, in the case where the layer is not scrollable and
     * therefore needs to be redrawn each time (so that the selection
     * rectangle can be cached).
     */
    virtual bool isLayerOpaque() const { return false; }

    /**
     * This should return true if the layer can be edited by the user.
     * If this is the case, the appropriate edit tools may be made
     * available by the application and the layer's drawStart/Drag/End
     * and editStart/Drag/End methods should be implemented.
     */
    virtual bool isLayerEditable() const { return false; }

    /**
     * Return the proportion of background work complete in drawing
     * this view, as a percentage -- in most cases this will be the
     * value returned by pointer from a call to the underlying model's
     * isReady(int *) call.  The widget may choose to show a progress
     * meter if it finds that this returns < 100 at any given moment.
     */
    virtual int getCompletion() const { return 100; }

    virtual void setObjectName(const QString &name);

    /**
     * Return the pixel x-coordinate corresponding to a given sample
     * frame (which may be negative).
     */
    int getXForFrame(long frame) const;

    /**
     * Return the closest frame to the given pixel x-coordinate.
     */
    long getFrameForX(int x) const;

    /**
     * Convert the layer's data (though not those of the model it
     * refers to) into an XML string for file output.  This class
     * implements the basic name/type/model-id output; subclasses will
     * typically call this superclass implementation with extra
     * attributes describing their particular properties.
     */
    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const;

    /**
     * Set the particular properties of a layer (those specific to the
     * subclass) from a set of XML attributes.  This is the effective
     * inverse of the toXmlString method.
     */
    virtual void setProperties(const QXmlAttributes &) = 0;

    /**
     * Indicate that a layer is not currently visible and is not
     * expected to become visible in the near future (for example
     * because the user has explicitly removed or hidden it).  The
     * layer may respond by (for example) freeing any cache memory it
     * is using, until next time its paint method is called.
     */
    virtual void setLayerDormant() { }

    virtual PlayParameters *getPlayParameters() const;

signals:
    void modelChanged();
    void modelCompletionChanged();
    void modelChanged(size_t startFrame, size_t endFrame);
    void modelReplaced();

    void layerParametersChanged();
    void layerNameChanged();

protected:
    View *m_view;
};

#endif

