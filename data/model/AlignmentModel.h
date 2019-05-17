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

#ifndef SV_ALIGNMENT_MODEL_H
#define SV_ALIGNMENT_MODEL_H

#include "Model.h"
#include "PathModel.h"
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
                   SparseTimeValueModel *path);
    ~AlignmentModel();

    bool isOK() const override;

    void setError(QString error) { m_error = error; }
    QString getError() const { return m_error; }

    sv_frame_t getStartFrame() const override;
    sv_frame_t getEndFrame() const override;
    sv_samplerate_t getSampleRate() const override;
    bool isReady(int *completion = 0) const override;
    int getCompletion() const override {
        int c = 0;
        (void)isReady(&c);
        return c;
    }
    const ZoomConstraint *getZoomConstraint() const override;

    QString getTypeName() const override { return tr("Alignment"); }

    const Model *getReferenceModel() const;
    const Model *getAlignedModel() const;

    sv_frame_t toReference(sv_frame_t frame) const;
    sv_frame_t fromReference(sv_frame_t frame) const;

    void setPathFrom(SparseTimeValueModel *rawpath);
    void setPath(PathModel *path);

    void toXml(QTextStream &stream,
                       QString indent = "",
                       QString extraAttributes = "") const override;

    QString toDelimitedDataString(QString, DataExportOptions,
                                  sv_frame_t, sv_frame_t) const override {
        return "";
    }

signals:
    void modelChanged();
    void modelChangedWithin(sv_frame_t startFrame, sv_frame_t endFrame);
    void completionChanged();

protected slots:
    void pathChanged();
    void pathChangedWithin(sv_frame_t startFrame, sv_frame_t endFrame);
    void pathCompletionChanged();

protected:
    Model *m_reference; // I don't own this
    Model *m_aligned; // I don't own this

    SparseTimeValueModel *m_rawPath; // I own this
    mutable PathModel *m_path; // I own this
    mutable PathModel *m_reversePath; // I own this
    bool m_pathBegun;
    bool m_pathComplete;
    QString m_error;

    void constructPath() const;
    void constructReversePath() const;

    sv_frame_t align(PathModel *path, sv_frame_t frame) const;
};

#endif
