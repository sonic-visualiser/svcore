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

#ifndef SV_SPARSE_ONE_DIMENSIONAL_MODEL_H
#define SV_SPARSE_ONE_DIMENSIONAL_MODEL_H

#include "SparseModel.h"
#include "base/NoteData.h"
#include "base/NoteExportable.h"
#include "base/PlayParameterRepository.h"
#include "base/RealTime.h"

#include <QStringList>

struct OneDimensionalPoint
{
public:
    OneDimensionalPoint(sv_frame_t _frame) : frame(_frame) { }
    OneDimensionalPoint(sv_frame_t _frame, QString _label) : frame(_frame), label(_label) { }

    int getDimensions() const { return 1; }
    
    sv_frame_t frame;
    QString label;

    QString getLabel() const { return label; }

    void toXml(QTextStream &stream,
               QString indent = "",
               QString extraAttributes = "") const
    {
        stream << QString("%1<point frame=\"%2\" label=\"%3\" %4/>\n")
            .arg(indent).arg(frame).arg(XmlExportable::encodeEntities(label))
            .arg(extraAttributes);
    }

    QString toDelimitedDataString(QString delimiter, DataExportOptions, sv_samplerate_t sampleRate) const
    {
        QStringList list;
        list << RealTime::frame2RealTime(frame, sampleRate).toString().c_str();
        if (label != "") list << label;
        return list.join(delimiter);
    }

    struct Comparator {
        bool operator()(const OneDimensionalPoint &p1,
                        const OneDimensionalPoint &p2) const {
            if (p1.frame != p2.frame) return p1.frame < p2.frame;
            return p1.label < p2.label;
        }
    };
    
    struct OrderComparator {
        bool operator()(const OneDimensionalPoint &p1,
                        const OneDimensionalPoint &p2) const {
            return p1.frame < p2.frame;
        }
    };

    bool operator==(const OneDimensionalPoint &p) const {
        return (frame == p.frame && label == p.label);
    }
};


class SparseOneDimensionalModel : public SparseModel<OneDimensionalPoint>,
                                  public NoteExportable
{
    Q_OBJECT
    
public:
    SparseOneDimensionalModel(sv_samplerate_t sampleRate, int resolution,
                              bool notifyOnAdd = true) :
        SparseModel<OneDimensionalPoint>(sampleRate, resolution, notifyOnAdd)
    {
        PlayParameterRepository::getInstance()->addPlayable(this);
    }

    virtual ~SparseOneDimensionalModel()
    {
        PlayParameterRepository::getInstance()->removePlayable(this);
    }

    bool canPlay() const override { return true; }

    QString getDefaultPlayClipId() const override
    {
        return "tap";
    }

    int getIndexOf(const Point &point)
    {
        // slow
        int i = 0;
        Point::Comparator comparator;
        for (PointList::const_iterator j = m_points.begin();
             j != m_points.end(); ++j, ++i) {
            if (!comparator(*j, point) && !comparator(point, *j)) return i;
        }
        return -1;
    }

    QString getTypeName() const override { return tr("Sparse 1-D"); }

    /**
     * TabularModel methods.  
     */
    
    int getColumnCount() const override
    {
        return 3;
    }

    QString getHeading(int column) const override
    {
        switch (column) {
        case 0: return tr("Time");
        case 1: return tr("Frame");
        case 2: return tr("Label");
        default: return tr("Unknown");
        }
    }

    QVariant getData(int row, int column, int role) const override
    {
        if (column < 2) {
            return SparseModel<OneDimensionalPoint>::getData
                (row, column, role);
        }

        PointListConstIterator i = getPointListIteratorForRow(row);
        if (i == m_points.end()) return QVariant();

        switch (column) {
        case 2: return i->label;
        default: return QVariant();
        }
    }

    Command *getSetDataCommand(int row, int column, const QVariant &value, int role) override
    {
        if (column < 2) {
            return SparseModel<OneDimensionalPoint>::getSetDataCommand
                (row, column, value, role);
        }

        if (role != Qt::EditRole) return 0;
        PointListConstIterator i = getPointListIteratorForRow(row);
        if (i == m_points.end()) return 0;
        EditCommand *command = new EditCommand(this, tr("Edit Data"));

        Point point(*i);
        command->deletePoint(point);

        switch (column) {
        case 2: point.label = value.toString(); break;
        }

        command->addPoint(point);
        return command->finish();
    }


    bool isColumnTimeValue(int column) const override
    {
        return (column < 2); 
    }

    SortType getSortType(int column) const override
    {
        if (column == 2) return SortAlphabetical;
        return SortNumeric;
    }

    /**
     * NoteExportable methods.
     */

    NoteList getNotes() const override {
        return getNotesStartingWithin(getStartFrame(),
                                      getEndFrame() - getStartFrame());
    }

    NoteList getNotesActiveAt(sv_frame_t frame) const override {
        return getNotesStartingWithin(frame, 1);
    }

    NoteList getNotesStartingWithin(sv_frame_t startFrame,
                                    sv_frame_t duration) const override {
        
        PointList points = getPoints(startFrame, startFrame + duration);
        NoteList notes;

        for (PointList::iterator pli =
                 points.begin(); pli != points.end(); ++pli) {

            notes.push_back
                (NoteData(pli->frame,
                          sv_frame_t(getSampleRate() / 6), // arbitrary short duration
                          64,   // default pitch
                          100)); // default velocity
        }

        return notes;
    }
};

#endif


    
