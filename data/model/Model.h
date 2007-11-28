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
class AlignmentModel;

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
     * Return the "work title" of the model, if known.
     */
    virtual QString getTitle() const;

    /**
     * Return the "artist" or "maker" of the model, if known.
     */
    virtual QString getMaker() const;

    /**
     * Return the location of the data in this model (e.g. source
     * URL).  This should not normally be returned for editable models
     * that have been edited.
     */
    virtual QString getLocation() const;

    /**
     * Return the type of the model.  For display purposes only.
     */
    virtual QString getTypeName() const = 0;

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
     * property (such as a specific alignment model set on this model)
     * indicates otherwise.
     */
    virtual Model *getSourceModel() const {
        return m_sourceModel;
    }

    /**
     * Set the source model for this model.
     */
    virtual void setSourceModel(Model *model);

    /**
     * Specify an aligment between this model's timeline and that of a
     * reference model.  The alignment model records both the
     * reference and the alignment.  This model takes ownership of the
     * alignment model.
     */
    virtual void setAlignment(AlignmentModel *alignment);

    /**
     * Return the reference model for the current alignment timeline,
     * if any.
     */
    virtual const Model *getAlignmentReference() const;

    /**
     * Return the frame number of the reference model that corresponds
     * to the given frame number in this model.
     */
    virtual size_t alignToReference(size_t frame) const;

    /**
     * Return the frame number in this model that corresponds to the
     * given frame number of the reference model.
     */
    virtual size_t alignFromReference(size_t referenceFrame) const;

    /**
     * Return the completion percentage for the alignment model: 100
     * if there is no alignment model or it has been entirely
     * calculated, or less than 100 if it is still being calculated.
     */
    virtual int getAlignmentCompletion() const;

    virtual void toXml(QTextStream &stream,
                       QString indent = "",
                       QString extraAttributes = "") const;

    virtual QString toDelimitedDataString(QString) const { return ""; }

public slots:
    void aboutToDelete();
    void sourceModelAboutToBeDeleted();

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

    /**
     * Emitted when the completion percentage changes for the
     * calculation of this model's alignment model.
     */
    void alignmentCompletionChanged();

    /**
     * Emitted when something notifies this model (through calling
     * aboutToDelete() that it is about to delete it.  Note that this
     * depends on an external agent such as a Document object or
     * owning model telling the model that it is about to delete it;
     * there is nothing in the model to guarantee that this signal
     * will be emitted before the actual deletion.
     */
    void aboutToBeDeleted();

protected:
    Model() : m_sourceModel(0), m_alignment(0), m_aboutToDelete(false) { }

    // Not provided.
    Model(const Model &);
    Model &operator=(const Model &); 

    Model *m_sourceModel;
    AlignmentModel *m_alignment;
    bool m_aboutToDelete;
};

#endif
