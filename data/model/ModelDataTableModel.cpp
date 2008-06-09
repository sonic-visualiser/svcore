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

#include "SparseTimeValueModel.h"
#include "SparseOneDimensionalModel.h"
#include "SparseModel.h"

ModelDataTableModel::ModelDataTableModel(Model *m) :
    m_model(m)
{
    connect(m, SIGNAL(modelChanged()), this, SLOT(modelChanged()));
    connect(m, SIGNAL(modelChanged(size_t, size_t)),
            this, SLOT(modelChanged(size_t, size_t)));
    rebuildRowVector();
}

ModelDataTableModel::~ModelDataTableModel()
{
}

QVariant
ModelDataTableModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    bool withUnit = (role == Qt::DisplayRole);

    if (!index.isValid()) return QVariant();

    int row = index.row(), col = index.column();

    if (row < 0 || row >= m_rows.size()) return QVariant();

    if (dynamic_cast<const SparseOneDimensionalModel *>(m_model)) {
        return dataSparse<SparseOneDimensionalModel::Point>(row, col, withUnit);
    } else if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
        return dataSparse<SparseTimeValueModel::Point>(row, col, withUnit);
    }

    return QVariant();
}

template <typename PointType>
QVariant
ModelDataTableModel::dataSparse(int row, int col, bool withUnit) const
{
    size_t frame = m_rows[row];
    
    // This is just garbage.  This would be a reasonable enough way to
    // handle this in a dynamically typed language but it's hopeless
    // in C++.  The design is simply wrong.  We need virtual helper
    // methods in the model itself.

    typedef SparseModel<PointType> ModelType;
    typedef std::multiset<PointType, typename PointType::OrderComparator> 
        PointListType;

    const ModelType *sm = dynamic_cast<const ModelType *>(m_model);
    const PointListType &points = sm->getPoints(frame);

    // it is possible to have more than one point at the same frame

    int indexAtFrame = 0;
    int ri = row;
    while (ri > 0 && m_rows[ri-1] == m_rows[row]) { --ri; ++indexAtFrame; }

    for (typename PointListType::const_iterator i = points.begin();
         i != points.end(); ++i) {

        const PointType *point = &(*i);
        if (point->frame < frame) continue;
        if (point->frame > frame) return QVariant();
        if (indexAtFrame > 0) { --indexAtFrame; continue; }

        switch (col) {

        case 0:
        {
            RealTime rt = RealTime::frame2RealTime(frame, m_model->getSampleRate());
            std::cerr << "Returning time " << rt << std::endl;
            return QVariant(rt.toText().c_str());
        }

        case 1:
            std::cerr << "Returning frame " << frame << std::endl;
            return QVariant(frame); //!!! RealTime

        case 2:
            if (dynamic_cast<const SparseOneDimensionalModel *>(m_model)) {
                const SparseOneDimensionalModel::Point *cp = 
                    reinterpret_cast<const SparseOneDimensionalModel::Point *>(point);
                std::cerr << "Returning label \"" << cp->label.toStdString() << "\"" << std::endl;
                return QVariant(cp->label);
            } else if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
                const SparseTimeValueModel::Point *cp = 
                    reinterpret_cast<const SparseTimeValueModel::Point *>(point);
                std::cerr << "Returning value " << cp->value << std::endl;
                if (withUnit) {
                    return QVariant(QString("%1 %2").arg(cp->value)
                                    .arg(dynamic_cast<const SparseTimeValueModel *>(m_model)->getScaleUnits()));
                } else {
                    return cp->value;
                }
            } else return QVariant();

        case 3: 
            if (dynamic_cast<const SparseOneDimensionalModel *>(m_model)) {
                return QVariant();
            } else if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
                return reinterpret_cast<const SparseTimeValueModel::Point *>(point)->label;
            } else return QVariant();
        }
    }

    return QVariant();
}

bool
ModelDataTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        std::cerr << "setData: ignoring role " << role << std::endl;
        return false;
    }

    //!!! see comment about disgustuality of this whole process, in
    //dataSparse above

    if (!index.isValid()) return false;

    int row = index.row(), col = index.column();

    if (row < 0 || row >= m_rows.size()) return false;

    if (dynamic_cast<const SparseOneDimensionalModel *>(m_model)) {
        return setDataSparse<SparseOneDimensionalModel::Point>(row, col, value);
    } else if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
        return setDataSparse<SparseTimeValueModel::Point>(row, col, value);
    }

    return false;
}

template <typename PointType>
bool
ModelDataTableModel::setDataSparse(int row, int col, QVariant value)
{
    size_t frame = m_rows[row];
    
    typedef SparseModel<PointType> ModelType;
    typedef std::multiset<PointType, typename PointType::OrderComparator> 
        PointListType;
    typedef typename ModelType::EditCommand EditCommandType;

    ModelType *sm = dynamic_cast<const ModelType *>(m_model);
    const PointListType &points = sm->getPoints(frame);

    // it is possible to have more than one point at the same frame

    int indexAtFrame = 0;
    int ri = row;
    while (ri > 0 && m_rows[ri-1] == m_rows[row]) { --ri; ++indexAtFrame; }

    for (typename PointListType::const_iterator i = points.begin();
         i != points.end(); ++i) {

        const PointType *point = &(*i);
        if (point->frame < frame) continue;
        if (point->frame > frame) return false;
        if (indexAtFrame > 0) { --indexAtFrame; continue; }

        switch (col) {

        case 0:
        {
/*
            RealTime rt = RealTime::frame2RealTime(frame, m_model->getSampleRate());
            std::cerr << "Returning time " << rt << std::endl;
            return QVariant(rt.toText().c_str());
*/
        }

        case 1:
        {
            EditCommandType *command = 
                new EditCommandType(sm, tr("Edit point time"));
            PointType newPoint(*point);
            newPoint.frame = value.toInt(); //!!! check validity
            command->deletePoint(*point);
            command->addPoint(newPoint);
            command = command->finish();
            if (command) emit executeCommand(command);
            return true;
        }
//            std::cerr << "Returning frame " << frame << std::endl;
//            return QVariant(frame); //!!! RealTime

        case 2:
            break;

        case 3: 
            break;
        }
    }

    return false;
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
        if (section == 0) return QVariant(tr("Time"));
        if (section == 1) return QVariant(tr("Frame"));
        else if (section == 2) {
            if (dynamic_cast<const SparseOneDimensionalModel *>(m_model)) {
                return QVariant(tr("Label"));
            } else if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
                return QVariant(tr("Value"));
            }
        } else if (section == 3) {
            if (dynamic_cast<const SparseTimeValueModel *>(m_model)) {
                return QVariant(tr("Label"));
            }
        }
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
    return m_rows.size();
}

int
ModelDataTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    if (!canHandleModelType(m_model)) return 0;

    if (dynamic_cast<SparseOneDimensionalModel *>(m_model)) {
        return 3;
    } else if (dynamic_cast<SparseTimeValueModel *>(m_model)) {
        return 4;
    }

    return 2;
}

QModelIndex 
ModelDataTableModel::getModelIndexForFrame(size_t frame) const
{
    std::vector<size_t>::const_iterator i =
        std::lower_bound(m_rows.begin(), m_rows.end(), frame);
    size_t dist = std::distance(m_rows.begin(), i);
    return createIndex(dist, 0, 0);
}

void
ModelDataTableModel::modelChanged()
{
    rebuildRowVector();
    emit layoutChanged();
}

void 
ModelDataTableModel::modelChanged(size_t f0, size_t f1)
{
    std::cerr << "ModelDataTableModel::modelChanged(" << f0 << "," << f1 << ")" << std::endl;
    //!!! highly inefficient
    rebuildRowVector();
    emit layoutChanged();
}

void
ModelDataTableModel::rebuildRowVector()
{
    if (!canHandleModelType(m_model)) return;

    m_rows.clear();

    if (dynamic_cast<SparseOneDimensionalModel *>(m_model)) {
        rebuildRowVectorSparse<SparseOneDimensionalModel::Point>();
    } else if (dynamic_cast<SparseTimeValueModel *>(m_model)) {
        rebuildRowVectorSparse<SparseTimeValueModel::Point>();
    }
}

template <typename PointType>
void
ModelDataTableModel::rebuildRowVectorSparse()
{
    // gah

    typedef SparseModel<PointType> ModelType;
    typedef std::multiset<PointType, typename PointType::OrderComparator> 
        PointListType;

    ModelType *sm = dynamic_cast<ModelType *>(m_model);
    const PointListType &points = sm->getPoints();

    for (typename PointListType::const_iterator i = points.begin();
         i != points.end(); ++i) {
        m_rows.push_back(i->frame);
    }
}

bool
ModelDataTableModel::canHandleModelType(Model *m)
{
    if (dynamic_cast<SparseOneDimensionalModel *>(m)) return true;
    if (dynamic_cast<SparseTimeValueModel *>(m)) return true;
    return false;
}


