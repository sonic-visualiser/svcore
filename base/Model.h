/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <QObject>

#include "XmlExportable.h"

typedef std::vector<float> SampleBlock;

/** 
 * Model is the base class for all data models that represent any sort
 * of data on a time scale based on an audio frame rate.
 */

class Model : virtual public QObject,
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

    virtual QString toXmlString(QString indent = "",
				QString extraAttributes = "") const;

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
    Model() { }

    // Not provided.
    Model(const Model &);
    Model &operator=(const Model &); 
};

#endif
