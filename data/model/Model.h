/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <QObject>

#include "base/XmlExportable.h"

typedef std::vector<float> SampleBlock;

class ZoomConstraint;

/** 
 * Model is the base class for all data models that represent any sort
 * of data on a time scale based on an audio frame rate.
 */

class Model : public QObject,
	      public XmlExportable
{
    Q_OBJECT

public:
    virtual ~Model();

    /**
     * Return true if the model was constructed successfully.  Classes
     * that refer to the model should always test this before use.
     */
    virtual bool isOK() const = 0;

    /**
     * Return the first audio frame spanned by the model.
     */
    virtual size_t getStartFrame() const = 0;

    /**
     * Return the last audio frame spanned by the model.
     */
    virtual size_t getEndFrame() const = 0;

    /**
     * Return the frame rate in frames per second.
     */
    virtual size_t getSampleRate() const = 0;

    /**
     * Return the frame rate of the underlying material, if the model
     * itself has already been resampled.
     */
    virtual size_t getNativeRate() const { return getSampleRate(); }

    /**
     * Return a copy of this model.
     *
     * If the model is not editable, this may be effectively a shallow
     * copy.  If the model is editable, however, this operation must
     * properly copy all of the model's editable data.
     *
     * In general this operation is not useful for non-editable dense
     * models such as waveforms, because there may be no efficient
     * copy operation implemented -- for such models it is better not
     * to copy at all.
     *
     * Caller owns the returned value.
     */
    virtual Model *clone() const = 0;
    
    /**
     * Return true if the model has finished loading or calculating
     * all its data, for a model that is capable of calculating in a
     * background thread.  The default implementation is appropriate
     * for a thread that does not background any work but carries out
     * all its calculation from the constructor or accessors.
     *
     * If "completion" is non-NULL, this function should return
     * through it an estimated percentage value showing how far
     * through the background operation it thinks it is (for progress
     * reporting).  If it has no way to calculate progress, it may
     * return the special value COMPLETION_UNKNOWN.
     */
    virtual bool isReady(int *completion = 0) const {
	bool ok = isOK();
	if (completion) *completion = (ok ? 100 : 0);
	return ok;
    }
    static const int COMPLETION_UNKNOWN;

    /**
     * If this model imposes a zoom constraint, i.e. some limit to the
     * set of resolutions at which its data can meaningfully be
     * displayed, then return it.
     */
    virtual const ZoomConstraint *getZoomConstraint() const {
        return 0;
    }

    /**
     * If this model was derived from another, return the model it was
     * derived from.  The assumption is that the source model's
     * alignment will also apply to this model, unless some other
     * property indicates otherwise.
     */
    virtual Model *getSourceModel() const {
        return m_sourceModel;
    }

    /**
     * Set the source model for this model.
     */
     //!!! No way to handle source model deletion &c yet
    virtual void setSourceModel(Model *model) {
        m_sourceModel = model;
    }

    virtual void toXml(QTextStream &stream,
                       QString indent = "",
                       QString extraAttributes = "") const;

    virtual QString toDelimitedDataString(QString) const { return ""; }

signals:
    /**
     * Emitted when a model has been edited (or more data retrieved
     * from cache, in the case of a cached model that generates slowly)
     */
    void modelChanged();

    /**
     * Emitted when a model has been edited (or more data retrieved
     * from cache, in the case of a cached model that generates slowly)
     */
    void modelChanged(size_t startFrame, size_t endFrame);

    /**
     * Emitted when some internal processing has advanced a stage, but
     * the model has not changed externally.  Views should respond by
     * updating any progress meters or other monitoring, but not
     * refreshing the actual view.
     */
    void completionChanged();

protected:
    Model() : m_sourceModel(0) { }

    // Not provided.
    Model(const Model &);
    Model &operator=(const Model &); 

    Model *m_sourceModel;
};

#endif
