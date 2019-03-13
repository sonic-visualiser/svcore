/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_TABULAR_MODEL_H
#define SV_TABULAR_MODEL_H

#include <QVariant>
#include <QString>

#include "base/RealTime.h"

class Command;

/**
 * TabularModel is an abstract base class for models that support
 * direct access to data in a tabular form.  A model that implements
 * TabularModel may be displayed and, perhaps, edited in a data
 * spreadsheet window.
 *
 * This is very like a cut-down QAbstractItemModel.  It assumes a
 * relationship between row number and frame time.
 */

class TabularModel
{
public:
    virtual ~TabularModel() { }

    virtual int getRowCount() const = 0;
    virtual int getColumnCount() const = 0;

    virtual QString getHeading(int column) const = 0;

    enum { SortRole = Qt::UserRole };
    enum SortType { SortNumeric, SortAlphabetical };

    virtual QVariant getData(int row, int column, int role) const = 0;
    virtual bool isColumnTimeValue(int col) const = 0;
    virtual SortType getSortType(int col) const = 0;

    virtual sv_frame_t getFrameForRow(int row) const = 0;
    virtual int getRowForFrame(sv_frame_t frame) const = 0;

    virtual bool isEditable() const { return false; }
    virtual Command *getSetDataCommand(int /* row */, int /* column */, const QVariant &, int /* role */) { return 0; }
    virtual Command *getInsertRowCommand(int /* beforeRow */) { return 0; }
    virtual Command *getRemoveRowCommand(int /* row */) { return 0; }

    QVariant adaptFrameForRole(sv_frame_t frame,
                               sv_samplerate_t rate,
                               int role) const {
        if (role == SortRole) return int(frame);
        RealTime rt = RealTime::frame2RealTime(frame, rate);
        if (role == Qt::EditRole) return rt.toString().c_str();
        else return rt.toText().c_str();
    }

    QVariant adaptValueForRole(float value,
                               QString unit,
                               int role) const {
        if (role == SortRole || role == Qt::EditRole) return value;
        else return QString("%1 %2").arg(value).arg(unit);
    }
};

#endif
