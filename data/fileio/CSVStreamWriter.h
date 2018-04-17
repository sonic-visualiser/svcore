/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2017 Lucas Thompson.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CSV_STREAM_WRITER_H_
#define _CSV_STREAM_WRITER_H_

#include "base/BaseTypes.h"
#include "base/Selection.h"
#include "base/ProgressReporter.h"
#include "base/DataExportOptions.h"
#include "data/model/Model.h"
#include <QString>
#include <algorithm>

namespace CSV
{
using Completed = bool;

template <class OutStream>
auto writeToStreamInChunks(
    OutStream& oss,
    const Model& model,
    const Selection& extents,
    ProgressReporter* reporter = nullptr,
    QString delimiter = ",",
    DataExportOptions options = DataExportDefaults,
    const sv_frame_t blockSize = 16384
) -> Completed
{
    if (blockSize <= 0) return false;
    sv_frame_t readPtr = extents.isEmpty() ?
        model.getStartFrame() : extents.getStartFrame();
    sv_frame_t endFrame = extents.isEmpty() ?
        model.getEndFrame() : extents.getEndFrame();
    int previousPercentagePoint = 0;

    const auto wasCancelled = [&reporter]() { 
        return reporter && reporter->wasCancelled(); 
    };

    while (readPtr < endFrame) {
        if (wasCancelled()) return false;

        const auto start = readPtr;
        const auto end = std::min(start + blockSize, endFrame);

        oss << model.toDelimitedDataStringSubsetWithOptions(
            delimiter,
            options,
            start,
            end
        ) << (end < endFrame ? "\n" : "");

        const auto currentPercentage = 100 * end / endFrame;
        const bool hasIncreased = currentPercentage > previousPercentagePoint;

        if (hasIncreased) {
            if (reporter) reporter->setProgress(currentPercentage);
            previousPercentagePoint = currentPercentage;
        }
        readPtr = end;
    }
    return !wasCancelled(); // setProgress could process event loop
}

template <class OutStream>
auto writeToStreamInChunks(
    OutStream& oss,
    const Model& model,
    ProgressReporter* reporter = nullptr,
    QString delimiter = ",",
    DataExportOptions options = DataExportDefaults,
    const sv_frame_t blockSize = 16384
) -> Completed
{
    const Selection empty;
    return CSV::writeToStreamInChunks(
        oss,
        model,
        empty,
        reporter,
        delimiter,
        options,
        blockSize
    );
}
} // namespace
#endif