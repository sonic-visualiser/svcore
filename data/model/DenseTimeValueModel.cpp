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
#include "base/PlayParameterRepository.h"

#include <QStringList>

DenseTimeValueModel::DenseTimeValueModel()
{
    PlayParameterRepository::getInstance()->addPlayable(this);
}

DenseTimeValueModel::~DenseTimeValueModel()
{
    PlayParameterRepository::getInstance()->removePlayable(this);
}
	
QString
DenseTimeValueModel::toDelimitedDataStringSubset(QString delimiter, sv_frame_t f0, sv_frame_t f1) const
{
    int ch = getChannelCount();

    cerr << "f0 = " << f0 << ", f1 = " << f1 << endl;

    if (f1 <= f0) return "";

    auto data = getMultiChannelData(0, ch - 1, f0, f1 - f0);

    if (data.empty() || data[0].empty()) return "";
    
    QStringList list;
    for (sv_frame_t i = 0; in_range_for(data[0], i); ++i) {
        QStringList parts;
        parts << QString("%1").arg(f0 + i);
        for (int c = 0; in_range_for(data, c); ++c) {
            parts << QString("%1").arg(data[c][i]);
        }
        list << parts.join(delimiter);
    }

    return list.join("\n");
}
