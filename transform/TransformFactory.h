/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
   
    This is experimental software.  Not for distribution.
*/

#ifndef _TRANSFORM_FACTORY_H_
#define _TRANSFORM_FACTORY_H_

#include "Transform.h"

#include <map>

class TransformFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformFactory();

    static TransformFactory *instance();

    // The name is intended to be computer-referencable, and unique
    // within the application.  The description should be
    // human-readable, and does not have to be unique.

    struct TransformDesc {
	TransformDesc(TransformName _name, QString _description = "") :
	    name(_name), description(_description) { }
	TransformName name;
	QString description;
    };
    typedef std::vector<TransformDesc> TransformList;

    TransformList getAllTransforms();

    /**
     * Return the output model resulting from applying the named
     * transform to the given input model.  The transform may still be
     * working in the background when the model is returned; check the
     * output model's isReady completion status for more details.
     *
     * If the transform is unknown or the input model is not an
     * appropriate type for the given transform, or if some other
     * problem occurs, return 0.
     * 
     * The returned model is owned by the caller and must be deleted
     * when no longer needed.
     */
    Model *transform(TransformName name, Model *inputModel);

    QString getTransformDescription(TransformName name);

    //!!! Need some way to indicate that the input model has changed /
    //been deleted so as not to blow up backgrounded transform!  -- Or
    //indeed, if the output model has been deleted -- could equally
    //well happen!

    //!!! Need transform category!
	
protected slots:
    void transformFinished();

protected:
    Transform *createTransform(TransformName name, Model *inputModel);
    Transform *createTransform(TransformName name, Model *inputModel,
			       bool start);

    typedef std::map<TransformName, QString> TransformMap;
    TransformMap m_transforms;
    void populateTransforms();

    static TransformFactory *m_instance;
};


#endif
