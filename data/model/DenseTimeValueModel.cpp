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

#include "DenseTimeValueModel.h"

#include <QStringList>

using namespace std;

QVector<QString>
DenseTimeValueModel::getStringExportHeaders(DataExportOptions) const
{
    int ch = getChannelCount();
    QVector<QString> sv;
    for (int i = 0; i < ch; ++i) {
        sv.push_back(QString("Channel%1").arg(i+1));
    }
    return sv;
}

QVector<QVector<QString>>
DenseTimeValueModel::toStringExportRows(DataExportOptions,
                                        sv_frame_t startFrame,
                                        sv_frame_t duration) const
{
    int ch = getChannelCount();

    if (duration <= 0) return {};

    auto data = getMultiChannelData(0, ch - 1, startFrame, duration);

    if (data.empty() || data[0].empty()) return {};
    
    QVector<QVector<QString>> rows;

    for (sv_frame_t i = 0; in_range_for(data[0], i); ++i) {
        QVector<QString> row;
        row.push_back(QString("%1").arg(startFrame + i));
        for (int c = 0; in_range_for(data, c); ++c) {
            row.push_back(QString("%1").arg(data[c][i]));
        }
        rows.push_back(row);
    }

    return rows;
}
