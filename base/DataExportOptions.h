/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef DATA_EXPORT_OPTIONS_H
#define DATA_EXPORT_OPTIONS_H

enum DataExportOption
{
    DataExportDefaults = 0x0,

    /**
     * Export sparse event-based models as if they were dense models,
     * writing an event at every interval of the model's
     * resolution. Where no event is present in the actual model, a
     * constant "fill event" is interpolated instead.
     */
    DataExportFillGaps = 0x1,

    /**
     * Omit the level attribute from exported events.
     */
    DataExportOmitLevel = 0x2,

    /**
     * Always include a timestamp in the first column. Otherwise
     * timestamps will only be included in sparse models.
     */
    DataExportAlwaysIncludeTimestamp = 0x4,

    /**
     * Use sample frames rather than seconds for time and duration
     * values.
     */
    DataExportWriteTimeInFrames = 0x8,
    
    /**
     * Write a header row before any data rows.
     */
    DataExportIncludeHeader = 0x10
};

typedef int DataExportOptions;

#endif
