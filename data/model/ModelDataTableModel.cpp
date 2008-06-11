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

#include "ModelDataTableModel.h"

#include "TabularModel.h"
#include "Model.h"

#include <algorithm>
#include <iostream>

ModelDataTableModel::ModelDataTableModel(TabularModel *m) :
    m_model(m)
{
    Model *baseModel = dynamic_cast<Model *>(m);

    connect(baseModel, SIGNAL(modelChanged()), this, SLOT(modelChanged()));
    connect(baseModel, SIGNAL(modelChanged(size_t, size_t)),
            this, SLOT(modelChanged(size_t, size_t)));
}

ModelDataTableModel::~ModelDataTableModel()
{
}

QVariant
ModelDataTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    return m_model->getData(getUnsorted(index.row()), index.column(), role);
}

bool
ModelDataTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) return false;
    Command *command = m_model->setData(getUnsorted(index.row()),
                                        index.column(), value, role);
    if (command) {
        emit executeCommand(command);
        return true;
    } else {
        return false;
    }
}

Qt::ItemFlags
ModelDataTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsEditable |
        Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable;
    return flags;
}

QVariant
ModelDataTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_model->getHeading(section);
    }
    return QVariant();
}

QModelIndex
ModelDataTableModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, 0);
}

QModelIndex
ModelDataTableModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int
ModelDataTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_model->getRowCount();
}

int
ModelDataTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_model->getColumnCount();
}

QModelIndex 
ModelDataTableModel::getModelIndexForFrame(size_t frame) const
{
    int row = m_model->getRowForFrame(frame);
    return createIndex(getSorted(row), 0, 0);
}

size_t 
ModelDataTableModel::getFrameForModelIndex(const QModelIndex &index) const
{
    return m_model->getFrameForRow(getUnsorted(index.row()));
}

void
ModelDataTableModel::sort(int column, Qt::SortOrder sortOrder)
{
    std::cerr << "ModelDataTableModel::sort(" << column << ", " << sortOrder
              << ")" << std::endl;
    m_sortColumn = column;
    m_sortOrdering = sortOrder;
    m_sort.clear();
    emit layoutChanged();
}

void
ModelDataTableModel::modelChanged()
{
    m_sort.clear();
    emit layoutChanged();
}

void 
ModelDataTableModel::modelChanged(size_t f0, size_t f1)
{
    //!!! inefficient
    m_sort.clear();
    emit layoutChanged();
}

int
ModelDataTableModel::getSorted(int row)
{
    if (m_model->isColumnTimeValue(m_sortColumn)) {
        if (m_sortOrdering == Qt::AscendingOrder) {
            return row;
        } else {
            return rowCount() - row - 1;
        }
    }

    if (m_sort.empty()) {
        resort();
    }
    if (row < 0 || row >= m_sort.size()) return 0;
    return m_sort[row];
}

int
ModelDataTableModel::getUnsorted(int row)
{
    if (m_model->isColumnTimeValue(m_sortColumn)) {
        if (m_sortOrdering == Qt::AscendingOrder) {
            return row;
        } else {
            return rowCount() - row - 1;
        }
    }
//!!! need the reverse of this
    if (m_sort.empty()) {
        resort();
    }
    if (row < 0 || row >= m_sort.size()) return 0;
    return m_sort[row];
}

void
ModelDataTableModel::resort()
{
    //...
}

