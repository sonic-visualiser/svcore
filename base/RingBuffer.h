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

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#ifndef SV_RINGBUFFER_H
#define SV_RINGBUFFER_H

#include <sys/types.h>

#include "system/System.h"

#include <bqvec/Barrier.h>
#include <bqvec/VectorOps.h>

#include <vector>

//#define DEBUG_RINGBUFFER 1

#ifdef DEBUG_RINGBUFFER
#include <iostream>
#endif

namespace sv {

/**
 * RingBuffer implements a lock-free ring buffer for one writer and N
 * readers, that is to be used to store a sample type T.
 */

template <typename T, int N = 1>
class RingBuffer
{
public:
    /**
     * Create a ring buffer with room to write n samples.
     *
     * Note that the internal storage size will actually be n+1
     * samples, as one element is unavailable for administrative
     * reasons.  Since the ring buffer performs best if its size is a
     * power of two, this means n should ideally be some power of two
     * minus one.
     */
    RingBuffer(int n);

    virtual ~RingBuffer();

    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    int getSize() const;

    /**
     * Return a new ring buffer of the given size, containing the same
     * data as this one as perceived by reader 0 of this buffer.  If
     * another thread reads from or writes to this buffer during the
     * call, the contents of the new buffer may be incomplete or
     * inconsistent.  If this buffer's data will not fit in the new
     * size, the contents are undetermined.
     */
    RingBuffer<T, N> resized(int newSize) const;

    /**
     * Reset read and write pointers, thus emptying the buffer.
     * Should be called from the write thread.
     */
    void reset();

    /**
     * Return the amount of data available for reading by reader R, in
     * samples.
     */
    int getReadSpace(int R = 0) const;

    /**
     * Return the amount of space available for writing, in samples.
     */
    int getWriteSpace() const;

    /**
     * Read n samples from the buffer, for reader R.  If fewer than n
     * are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    int read(T *destination, int n, int R = 0);

    /**
     * Read n samples from the buffer, for reader R, adding them to
     * the destination.  If fewer than n are available, the remainder
     * will be left alone.  Returns the number of samples actually
     * read.
     */
    int readAdding(T *destination, int n, int R = 0);

    /**
     * Read one sample from the buffer, for reader R.  If no sample is
     * available, this will silently return zero.  Calling this
     * repeatedly is obviously slower than calling read once, but it
     * may be good enough if you don't want to allocate a buffer to
     * read into.
     */
    T readOne(int R = 0);

    /**
     * Read n samples from the buffer, if available, for reader R,
     * without advancing the read pointer -- i.e. a subsequent read()
     * or skip() will be necessary to empty the buffer.  If fewer than
     * n are available, the remainder will be zeroed out.  Returns the
     * number of samples actually read.
     */
    int peek(T *destination, int n, int R = 0) const;

    /**
     * Read one sample from the buffer, if available, without
     * advancing the read pointer -- i.e. a subsequent read() or
     * skip() will be necessary to empty the buffer.  Returns zero if
     * no sample was available.
     */
    T peekOne(int R = 0) const;

    /**
     * Pretend to read n samples from the buffer, for reader R,
     * without actually returning them (i.e. discard the next n
     * samples).  Returns the number of samples actually available for
     * discarding.
     */
    int skip(int n, int R = 0);

    /**
     * Write n samples to the buffer.  If insufficient space is
     * available, not all samples may actually be written.  Returns
     * the number of samples actually written.
     */
    int write(const T *source, int n);

    /**
     * Write n zero-value samples to the buffer.  If insufficient
     * space is available, not all zeros may actually be written.
     * Returns the number of zeroes actually written.
     */
    int zero(int n);

    RingBuffer(const RingBuffer &) =default;
    RingBuffer &operator=(const RingBuffer &) =default;
    
protected:
    std::vector<T, breakfastquay::StlAllocator<T>> m_buffer;
    int m_writer;
    std::vector<int> m_readers;
    int m_size;
};

template <typename T, int N>
RingBuffer<T, N>::RingBuffer(int n) :
    m_buffer(n + 1, T()),
    m_writer(0),
    m_readers(N, 0),
    m_size(n + 1)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::RingBuffer(" << n << ")" << std::endl;
#endif
}

template <typename T, int N>
RingBuffer<T, N>::~RingBuffer()
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::~RingBuffer" << std::endl;
#endif
}

template <typename T, int N>
int
RingBuffer<T, N>::getSize() const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getSize(): " << m_size-1 << std::endl;
#endif

    return m_size - 1;
}

template <typename T, int N>
RingBuffer<T, N>
RingBuffer<T, N>::resized(int newSize) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::resized(" << newSize << ")" << std::endl;
#endif

    RingBuffer<T, N> newBuffer(newSize);

    int w = m_writer;
    int r = m_readers[0];

    while (r != w) {
        T value = m_buffer[r];
        newBuffer->write(&value, 1);
        if (++r == m_size) r = 0;
    }

    return newBuffer;
}

template <typename T, int N>
void
RingBuffer<T, N>::reset()
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::reset" << std::endl;
#endif

    m_writer = 0;
    for (int i = 0; i < N; ++i) {
        m_readers[i] = 0;
    }
}

template <typename T, int N>
int
RingBuffer<T, N>::getReadSpace(int R) const
{
    int writer = m_writer;
    int reader = m_readers[R];
    int space = 0;

    if (writer > reader) space = writer - reader;
    else space = ((writer + m_size) - reader) % m_size;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getReadSpace(" << R << "): " << space << std::endl;
#endif

    return space;
}

template <typename T, int N>
int
RingBuffer<T, N>::getWriteSpace() const
{
    int space = 0;
    for (int i = 0; i < N; ++i) {
        int here = (m_readers[i] + m_size - m_writer - 1) % m_size;
        if (i == 0 || here < space) space = here;
    }

#ifdef DEBUG_RINGBUFFER
    int rs(getReadSpace()), rp(m_readers[0]);

    std::cerr << "RingBuffer: write space " << space << ", read space "
              << rs << ", total " << (space + rs) << ", m_size " << m_size << std::endl;
    std::cerr << "RingBuffer: reader " << rp << ", writer " << m_writer << std::endl;
#endif

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::getWriteSpace(): " << space << std::endl;
#endif

    return space;
}

template <typename T, int N>
int
RingBuffer<T, N>::read(T *destination, int n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::read(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        breakfastquay::v_zero(destination + available, n - available);
        n = available;
    }
    if (n == 0) return n;

    int here = m_size - m_readers[R];
    if (here >= n) {
        breakfastquay::v_copy(destination, m_buffer.data() + m_readers[R], n);
    } else {
        breakfastquay::v_copy(destination, m_buffer.data() + m_readers[R], here);
        breakfastquay::v_copy(destination + here, m_buffer.data(), n - here);
    }

    BQ_MBARRIER();
    m_readers[R] = (m_readers[R] + n) % m_size;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::read: read " << n << ", reader now " << m_readers[R] << std::endl;
#endif

    return n;
}

template <typename T, int N>
int
RingBuffer<T, N>::readAdding(T *destination, int n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::readAdding(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;

    int here = m_size - m_readers[R];

    if (here >= n) {
        for (int i = 0; i < n; ++i) {
            destination[i] += (m_buffer.data() + m_readers[R])[i];
        }
    } else {
        for (int i = 0; i < here; ++i) {
            destination[i] += (m_buffer.data() + m_readers[R])[i];
        }
        for (int i = 0; i < (n - here); ++i) {
            destination[i + here] += m_buffer[i];
        }
    }

    BQ_MBARRIER();
    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
T
RingBuffer<T, N>::readOne(int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::readOne(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: No sample available"
                  << std::endl;
#endif
        return T();
    }
    T value = m_buffer[m_readers[R]];
    BQ_MBARRIER();
    if (++m_readers[R] == m_size) m_readers[R] = 0;
    return value;
}

template <typename T, int N>
int
RingBuffer<T, N>::peek(T *destination, int n, int R) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek(dest, " << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        breakfastquay::v_zero(destination + available, n - available);
        n = available;
    }
    if (n == 0) return n;

    int here = m_size - m_readers[R];
    if (here >= n) {
        breakfastquay::v_copy(destination, m_buffer.data() + m_readers[R], n);
    } else {
        breakfastquay::v_copy(destination, m_buffer.data() + m_readers[R], here);
        breakfastquay::v_copy(destination + here, m_buffer.data(), n - here);
    }

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek: read " << n << std::endl;
#endif

    return n;
}

template <typename T, int N>
T
RingBuffer<T, N>::peekOne(int R) const
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::peek(" << R << ")" << std::endl;
#endif

    if (m_writer == m_readers[R]) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: No sample available"
                  << std::endl;
#endif
        return T();
    }
    T value = m_buffer[m_readers[R]];
    return value;
}

template <typename T, int N>
int
RingBuffer<T, N>::skip(int n, int R)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::skip(" << n << ", " << R << ")" << std::endl;
#endif

    int available = getReadSpace(R);
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only " << available << " samples available"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;
    m_readers[R] = (m_readers[R] + n) % m_size;
    return n;
}

template <typename T, int N>
int
RingBuffer<T, N>::write(const T *source, int n)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::write(" << n << ")" << std::endl;
#endif

    int available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only room for " << available << " samples"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;

    int here = m_size - m_writer;
    if (here >= n) {
        breakfastquay::v_copy(m_buffer.data() + m_writer, source, n);
    } else {
        breakfastquay::v_copy(m_buffer.data() + m_writer, source, here);
        breakfastquay::v_copy(m_buffer.data(), source + here, n - here);
    }

    BQ_MBARRIER();
    m_writer = (m_writer + n) % m_size;

#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::write: wrote " << n << ", writer now " << m_writer << std::endl;
#endif

    return n;
}

template <typename T, int N>
int
RingBuffer<T, N>::zero(int n)
{
#ifdef DEBUG_RINGBUFFER
    std::cerr << "RingBuffer<T," << N << ">[" << this << "]::zero(" << n << ")" << std::endl;
#endif

    int available = getWriteSpace();
    if (n > available) {
#ifdef DEBUG_RINGBUFFER
        std::cerr << "WARNING: Only room for " << available << " samples"
                  << std::endl;
#endif
        n = available;
    }
    if (n == 0) return n;

    int here = m_size - m_writer;
    if (here >= n) {
        breakfastquay::v_zero(m_buffer.data() + m_writer, n);
    } else {
        breakfastquay::v_zero(m_buffer.data() + m_writer, here);
        breakfastquay::v_zero(m_buffer.data(), n - here);
    }
    
    BQ_MBARRIER();
    m_writer = (m_writer + n) % m_size;
    return n;
}

} // end namespace sv


#endif // _RINGBUFFER_H_
