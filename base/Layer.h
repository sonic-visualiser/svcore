
/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005
    
    This is experimental software.  Not for distribution.
*/

#ifndef _VIEWER_H_
#define _VIEWER_H_

#include "PropertyContainer.h"

#include <QObject>
#include <QRect>

class ZoomConstraint;
class Model;
class QPainter;
class View;

/**
 * The base class for visual representations of the data found in a
 * Model.  Layers are expected to be able to draw themselves onto a
 * View, and may also be editable.
 */

class Layer : public QObject,
	      public PropertyContainer
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

    virtual QString getPropertyContainerName() const {
	return objectName();
    }

    virtual int getVerticalScaleWidth(QPainter &) const { return 0; }
    virtual void paintVerticalScale(QPainter &, QRect) const { }

    virtual QRect getFeatureDescriptionRect(QPainter &, QPoint) const {
	return QRect(0, 0, 0, 0);
    }
    virtual void paintLocalFeatureDescription(QPainter &, QRect, QPoint) const {
    }

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
     * Return the proportion of background work complete in drawing
     * this view, as a percentage -- in most cases this will be the
     * value returned by pointer from a call to the underlying model's
     * isReady(int *) call.  The widget may choose to show a progress
     * meter if it finds that this returns < 100 at any given moment.
     */
    virtual int getCompletion() const { return 100; }

    virtual void setObjectName(const QString &name);

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

