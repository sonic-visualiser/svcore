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
    // within the application.  The description is intended to be
    // human readable.  In principle it doesn't have to be unique, but
    // the factory will add suffixes to ensure that it is, all the
    // same (just to avoid user confusion).

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

    /**
     * Full description of a transform, suitable for putting on a menu.
     */
    QString getTransformDescription(TransformName name);

    /**
     * Brief but friendly description of a transform, suitable for use
     * as the name of the output layer.
     */
    QString getTransformFriendlyName(TransformName name);

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
