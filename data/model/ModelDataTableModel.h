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

#ifndef _MODEL_DATA_TABLE_MODEL_H_
#define _MODEL_DATA_TABLE_MODEL_H_

#include <QAbstractItemModel>

#include "Model.h"

class ModelDataTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ModelDataTableModel(Model *m);
    virtual ~ModelDataTableModel();

    QVariant data(const QModelIndex &index, int role) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    static bool canHandleModelType(Model *);

protected slots:
    void modelChanged();
    void modelChanged(size_t, size_t);

protected:
    // We need to have some sort of map between row and time in sample
    // frames.  I guess this will do for now.

    std::vector<size_t> m_rows; // contains sample frame

    Model *m_model;

    void rebuildRowVector();
    template <typename PointType> void rebuildRowVectorSparse();
    template <typename PointType> QVariant dataSparse(int row, int col) const;
};

#endif
