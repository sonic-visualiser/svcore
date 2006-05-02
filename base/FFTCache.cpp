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

#include "FFTCache.h"
#include "System.h"

#include <iostream>

FFTMemoryCache::FFTMemoryCache() :
    m_width(0),
    m_height(0),
    m_magnitude(0),
    m_phase(0),
    m_factor(0)
{
}

FFTMemoryCache::~FFTMemoryCache()
{
    std::cerr << "FFTMemoryCache[" << this << "]::~Cache" << std::endl;

    for (size_t i = 0; i < m_width; ++i) {
	if (m_magnitude && m_magnitude[i]) free(m_magnitude[i]);
	if (m_phase && m_phase[i]) free(m_phase[i]);
    }

    if (m_magnitude) free(m_magnitude);
    if (m_phase) free(m_phase);
    if (m_factor) free(m_factor);
}

void
FFTMemoryCache::resize(size_t width, size_t height)
{
    std::cerr << "FFTMemoryCache[" << this << "]::resize(" << width << "x" << height << " = " << width*height << ")" << std::endl;
    
    if (m_width == width && m_height == height) return;

    resize(m_magnitude, width, height);
    resize(m_phase, width, height);

    m_factor = (float *)realloc(m_factor, width * sizeof(float));

    m_width = width;
    m_height = height;

    std::cerr << "done, width = " << m_width << " height = " << m_height << std::endl;
}

void
FFTMemoryCache::resize(uint16_t **&array, size_t width, size_t height)
{
    for (size_t i = width; i < m_width; ++i) {
	free(array[i]);
    }

    if (width != m_width) {
	array = (uint16_t **)realloc(array, width * sizeof(uint16_t *));
	if (!array) throw std::bad_alloc();
	MUNLOCK(array, width * sizeof(uint16_t *));
    }

    for (size_t i = m_width; i < width; ++i) {
	array[i] = 0;
    }

    for (size_t i = 0; i < width; ++i) {
	array[i] = (uint16_t *)realloc(array[i], height * sizeof(uint16_t));
	if (!array[i]) throw std::bad_alloc();
	MUNLOCK(array[i], height * sizeof(uint16_t));
    }
}

void
FFTMemoryCache::reset()
{
    for (size_t x = 0; x < m_width; ++x) {
	for (size_t y = 0; y < m_height; ++y) {
	    m_magnitude[x][y] = 0;
	    m_phase[x][y] = 0;
	}
	m_factor[x] = 1.0;
    }
}	    

