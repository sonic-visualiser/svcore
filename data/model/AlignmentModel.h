/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ALIGNMENT_MODEL_H_
#define _ALIGNMENT_MODEL_H_

#include "Model.h"
#include "SparseModel.h"
#include "base/RealTime.h"

#include <QString>
#include <QStringList>

class SparseTimeValueModel;

class AlignmentModel : public Model
{
    Q_OBJECT

public:
    AlignmentModel(Model *reference,
                   Model *aligned,
                   Model *inputModel, // probably an AggregateWaveModel; I take ownership
                   SparseTimeValueModel *path); // I take ownership
    ~AlignmentModel();

    virtual bool isOK() const;
    virtual size_t getStartFrame() const;
    virtual size_t getEndFrame() const;
    virtual size_t getSampleRate() const;
    virtual Model *clone() const;
    virtual bool isReady(int *completion = 0) const;
    virtual const ZoomConstraint *getZoomConstraint() const;

    QString getTypeName() const { return tr("Alignment"); }

    const Model *getReferenceModel() const;
    const Model *getAlignedModel() const;

    size_t toReference(size_t frame) const;
    size_t fromReference(size_t frame) const;

signals:
    void modelChanged();
    void modelChanged(size_t startFrame, size_t endFrame);
    void completionChanged();

protected slots:
    void pathChanged();
    void pathChanged(size_t startFrame, size_t endFrame);
    void pathCompletionChanged();

protected:
    Model *m_reference; // I don't own this
    Model *m_aligned; // I don't own this

    Model *m_inputModel; // I own this

    struct PathPoint
    {
        PathPoint(long _frame) : frame(_frame), mapframe(_frame) { }
        PathPoint(long _frame, long _mapframe) :
            frame(_frame), mapframe(_mapframe) { }

        int getDimensions() const { return 2; }

        long frame;
        long mapframe;

        QString getLabel() const { return ""; }

        void toXml(QTextStream &stream, QString indent = "",
                   QString extraAttributes = "") const {
            stream << QString("%1<point frame=\"%2\" mapframe=\"%3\" %4/>\n")
                .arg(indent).arg(frame).arg(mapframe).arg(extraAttributes);
        }
        
        QString toDelimitedDataString(QString delimiter,
                                      size_t sampleRate) const {
            QStringList list;
            list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
            list << QString("%1").arg(mapframe);
            return list.join(delimiter);
        }

        struct Comparator {
            bool operator()(const PathPoint &p1, const PathPoint &p2) const {
                if (p1.frame != p2.frame) return p1.frame < p2.frame;
                return p1.mapframe < p2.mapframe;
            }
        };
    
        struct OrderComparator {
            bool operator()(const PathPoint &p1, const PathPoint &p2) const {
                return p1.frame < p2.frame;
            }
        };
    };

    class PathModel : public SparseModel<PathPoint>
    {
    public:
        PathModel(size_t sampleRate, size_t resolution, bool notify = true) :
            SparseModel<PathPoint>(sampleRate, resolution, notify) { }
    };

    SparseTimeValueModel *m_rawPath; // I own this
    mutable PathModel *m_path; // I own this
    mutable PathModel *m_reversePath; // I own this
    bool m_pathBegun;
    bool m_pathComplete;

    void constructPath() const;
    void constructReversePath() const;

    size_t align(PathModel *path, size_t frame) const;
};

#endif
