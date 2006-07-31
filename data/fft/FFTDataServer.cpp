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

#include "FFTDataServer.h"

#include "FFTFileCache.h"

#include "model/DenseTimeValueModel.h"

#include "system/System.h"

//#define DEBUG_FFT_SERVER 1
//#define DEBUG_FFT_SERVER_FILL 1

#ifdef DEBUG_FFT_SERVER_FILL
#define DEBUG_FFT_SERVER
#endif

FFTDataServer::ServerMap FFTDataServer::m_servers;
QMutex FFTDataServer::m_serverMapMutex;

FFTDataServer *
FFTDataServer::getInstance(const DenseTimeValueModel *model,
                           int channel,
                           WindowType windowType,
                           size_t windowSize,
                           size_t windowIncrement,
                           size_t fftSize,
                           bool polar,
                           size_t fillFromColumn)
{
    QString n = generateFileBasename(model,
                                     channel,
                                     windowType,
                                     windowSize,
                                     windowIncrement,
                                     fftSize,
                                     polar);

    FFTDataServer *server = 0;
    
    QMutexLocker locker(&m_serverMapMutex);

    if ((server = findServer(n))) {
        return server;
    }

    QString npn = generateFileBasename(model,
                                       channel,
                                       windowType,
                                       windowSize,
                                       windowIncrement,
                                       fftSize,
                                       !polar);

    if ((server = findServer(npn))) {
        return server;
    }

    m_servers[n] = ServerCountPair
        (new FFTDataServer(n,
                           model,
                           channel,
                           windowType,
                           windowSize,
                           windowIncrement,
                           fftSize,
                           polar,
                           fillFromColumn),
         1);

    return m_servers[n].first;
}

FFTDataServer *
FFTDataServer::getFuzzyInstance(const DenseTimeValueModel *model,
                                int channel,
                                WindowType windowType,
                                size_t windowSize,
                                size_t windowIncrement,
                                size_t fftSize,
                                bool polar,
                                size_t fillFromColumn)
{
    // Fuzzy matching:
    // 
    // -- if we're asked for polar and have non-polar, use it (and
    // vice versa).  This one is vital, and we do it for non-fuzzy as
    // well (above).
    //
    // -- if we're asked for an instance with a given fft size and we
    // have one already with a multiple of that fft size but the same
    // window size and type (and model), we can draw the results from
    // it (e.g. the 1st, 2nd, 3rd etc bins of a 512-sample FFT are the
    // same as the the 1st, 5th, 9th etc of a 2048-sample FFT of the
    // same window plus zero padding).
    //
    // -- if we're asked for an instance with a given window type and
    // size and fft size and we have one already the same but with a
    // smaller increment, we can draw the results from it (provided
    // our increment is a multiple of its)
    //
    // The FFTModel knows how to interpret these things.  In
    // both cases we require that the larger one is a power-of-two
    // multiple of the smaller (e.g. even though in principle you can
    // draw the results at increment 256 from those at increment 768
    // or 1536, the model doesn't support this).

    {
        QMutexLocker locker(&m_serverMapMutex);

        ServerMap::iterator best = m_servers.end();
        int bestdist = -1;
    
        for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {

            FFTDataServer *server = i->second.first;

            if (server->getModel() == model &&
                (server->getChannel() == channel || model->getChannelCount() == 1) &&
                server->getWindowType() == windowType &&
                server->getWindowSize() == windowSize &&
                server->getWindowIncrement() <= windowIncrement &&
                server->getFFTSize() >= fftSize) {
                
                if ((windowIncrement % server->getWindowIncrement()) != 0) continue;
                int ratio = windowIncrement / server->getWindowIncrement();
                bool poweroftwo = true;
                while (ratio > 1) {
                    if (ratio & 0x1) {
                        poweroftwo = false;
                        break;
                    }
                    ratio >>= 1;
                }
                if (!poweroftwo) continue;

                if ((server->getFFTSize() % fftSize) != 0) continue;
                ratio = server->getFFTSize() / fftSize;
                while (ratio > 1) {
                    if (ratio & 0x1) {
                        poweroftwo = false;
                        break;
                    }
                    ratio >>= 1;
                }
                if (!poweroftwo) continue;
                
                int distance = 0;
                
                if (server->getPolar() != polar) distance += 1;
                
                distance += ((windowIncrement / server->getWindowIncrement()) - 1) * 15;
                distance += ((server->getFFTSize() / fftSize) - 1) * 10;
                
                if (server->getFillCompletion() < 50) distance += 100;

#ifdef DEBUG_FFT_SERVER
                std::cerr << "Distance " << distance << ", best is " << bestdist << std::endl;
#endif
                
                if (bestdist == -1 || distance < bestdist) {
                    bestdist = distance;
                    best = i;
                }
            }
        }

        if (bestdist >= 0) {
            ++best->second.second;
            return best->second.first;
        }
    }

    // Nothing found, make a new one

    return getInstance(model,
                       channel,
                       windowType,
                       windowSize,
                       windowIncrement,
                       fftSize,
                       polar,
                       fillFromColumn);
}

FFTDataServer *
FFTDataServer::findServer(QString n)
{    
    if (m_servers.find(n) != m_servers.end()) {
        ++m_servers[n].second;
        return m_servers[n].first;
    }

    return 0;
}

void
FFTDataServer::claimInstance(FFTDataServer *server)
{
    
    QMutexLocker locker(&m_serverMapMutex);

    for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {
        if (i->second.first == server) {
            ++i->second.second;
            return;
        }
    }
    
    std::cerr << "ERROR: FFTDataServer::claimInstance: instance "
              << server << " unknown!" << std::endl;
}

void
FFTDataServer::releaseInstance(FFTDataServer *server)
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::releaseInstance(" << server << ")" << std::endl;
#endif

    QMutexLocker locker(&m_serverMapMutex);

    //!!! not a good strategy.  Want something like:

    // -- if ref count > 0, decrement and return
    // -- if the instance hasn't been used at all, delete it immediately 
    // -- if fewer than N instances (N = e.g. 3) remain with zero refcounts,
    //    leave them hanging around
    // -- if N instances with zero refcounts remain, delete the one that
    //    was last released first
    // -- if we run out of disk space when allocating an instance, go back
    //    and delete the spare N instances before trying again
    // -- have an additional method to indicate that a model has been
    //    destroyed, so that we can delete all of its fft server instances

    // also:
    //

    for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {
        if (i->second.first == server) {
            if (i->second.second == 0) {
                std::cerr << "ERROR: FFTDataServer::releaseInstance("
                          << server << "): instance not allocated" << std::endl;
            } else if (--i->second.second == 0) {
                if (server->m_lastUsedCache == -1) { // never used
                    delete server;
                    m_servers.erase(i);
                } else {
                    server->suspend();
                    purgeLimbo();
                }
            }
            return;
        }
    }

    std::cerr << "ERROR: FFTDataServer::releaseInstance(" << server << "): "
              << "instance not found" << std::endl;
}

void
FFTDataServer::purgeLimbo(int maxSize)
{
    ServerMap::iterator i = m_servers.end();

    int count = 0;

    while (i != m_servers.begin()) {
        --i;
        if (i->second.second == 0) {
            if (++count > maxSize) {
                delete i->second.first;
                m_servers.erase(i);
                return;
            }
        }
    }
}

FFTDataServer::FFTDataServer(QString fileBaseName,
                             const DenseTimeValueModel *model,
                             int channel,
			     WindowType windowType,
			     size_t windowSize,
			     size_t windowIncrement,
			     size_t fftSize,
                             bool polar,
                             size_t fillFromColumn) :
    m_fileBaseName(fileBaseName),
    m_model(model),
    m_channel(channel),
    m_windower(windowType, windowSize),
    m_windowSize(windowSize),
    m_windowIncrement(windowIncrement),
    m_fftSize(fftSize),
    m_polar(polar),
    m_lastUsedCache(-1),
    m_fftInput(0),
    m_exiting(false),
    m_fillThread(0)
{
    size_t start = m_model->getStartFrame();
    size_t end = m_model->getEndFrame();

    m_width = (end - start) / m_windowIncrement + 1;
    m_height = m_fftSize / 2;

    size_t maxCacheSize = 20 * 1024 * 1024;
    size_t columnSize = m_height * sizeof(fftsample) * 2 + sizeof(fftsample);
    if (m_width * columnSize < maxCacheSize * 2) m_cacheWidth = m_width;
    else m_cacheWidth = maxCacheSize / columnSize;
    
    int bits = 0;
    while (m_cacheWidth) { m_cacheWidth >>= 1; ++bits; }
    m_cacheWidth = 2;
    while (bits) { m_cacheWidth <<= 1; --bits; }
    
#ifdef DEBUG_FFT_SERVER
    std::cerr << "Width " << m_width << ", cache width " << m_cacheWidth << " (size " << m_cacheWidth * columnSize << ")" << std::endl;
#endif

    for (size_t i = 0; i <= m_width / m_cacheWidth; ++i) {
        m_caches.push_back(0);
    }

    m_fftInput = (fftsample *)
        fftwf_malloc(fftSize * sizeof(fftsample));

    m_fftOutput = (fftwf_complex *)
        fftwf_malloc(fftSize * sizeof(fftwf_complex));

    m_workbuffer = (float *)
        fftwf_malloc(fftSize * sizeof(float));

    m_fftPlan = fftwf_plan_dft_r2c_1d(m_fftSize,
                                      m_fftInput,
                                      m_fftOutput,
                                      FFTW_ESTIMATE);

    if (!m_fftPlan) {
        std::cerr << "ERROR: fftwf_plan_dft_r2c_1d(" << m_windowSize << ") failed!" << std::endl;
        throw(0);
    }

    m_fillThread = new FillThread(*this, fillFromColumn);

    //!!! respond appropriately when thread exits (deleteProcessingData etc)
}

FFTDataServer::~FFTDataServer()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << ")::~FFTDataServer()" << std::endl;
#endif

    m_exiting = true;
    m_condition.wakeAll();
    if (m_fillThread) {
        m_fillThread->wait();
        delete m_fillThread;
    }

    QMutexLocker locker(&m_writeMutex);

    for (CacheVector::iterator i = m_caches.begin(); i != m_caches.end(); ++i) {
        delete *i;
    }

    deleteProcessingData();
}

void
FFTDataServer::deleteProcessingData()
{
    if (m_fftInput) {
        fftwf_destroy_plan(m_fftPlan);
        fftwf_free(m_fftInput);
        fftwf_free(m_fftOutput);
        fftwf_free(m_workbuffer);
    }
    m_fftInput = 0;
}

void
FFTDataServer::suspend()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << "): suspend" << std::endl;
#endif
    QMutexLocker locker(&m_writeMutex);
    m_suspended = true;
    for (CacheVector::iterator i = m_caches.begin(); i != m_caches.end(); ++i) {
        if (*i) (*i)->suspend();
    }
}

void
FFTDataServer::resume()
{
    m_suspended = false;
    m_condition.wakeAll();
}

FFTCache *
FFTDataServer::getCacheAux(size_t c)
{
    QMutexLocker locker(&m_writeMutex);

    if (m_lastUsedCache == -1) {
        m_fillThread->start();
    }

    if (int(c) != m_lastUsedCache) {

//        std::cerr << "switch from " << m_lastUsedCache << " to " << c << std::endl;

        for (IntQueue::iterator i = m_dormantCaches.begin();
             i != m_dormantCaches.end(); ++i) {
            if (*i == c) {
                m_dormantCaches.erase(i);
                break;
            }
        }

        if (m_lastUsedCache >= 0) {
            bool inDormant = false;
            for (size_t i = 0; i < m_dormantCaches.size(); ++i) {
                if (m_dormantCaches[i] == m_lastUsedCache) {
                    inDormant = true;
                    break;
                }
            }
            if (!inDormant) {
                m_dormantCaches.push_back(m_lastUsedCache);
            }
            while (m_dormantCaches.size() > 4) {
                int dc = m_dormantCaches.front();
                m_dormantCaches.pop_front();
                m_caches[dc]->suspend();
            }
        }
    }

    if (m_caches[c]) {
        m_lastUsedCache = c;
        return m_caches[c];
    }

    QString name = QString("%1-%2").arg(m_fileBaseName).arg(c);

    FFTCache *cache = new FFTFileCache(name, MatrixFile::ReadWrite,
                                       m_polar ? FFTFileCache::Polar :
                                                 FFTFileCache::Rectangular);

    size_t width = m_cacheWidth;
    if (c * m_cacheWidth + width > m_width) {
        width = m_width - c * m_cacheWidth;
    }

    cache->resize(width, m_height);
    cache->reset();

    m_caches[c] = cache;
    m_lastUsedCache = c;

    return cache;
}

float
FFTDataServer::getMagnitudeAt(size_t x, size_t y)
{
    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache->haveSetColumnAt(col)) {
        fillColumn(x);
    }
    return cache->getMagnitudeAt(col, y);
}

float
FFTDataServer::getNormalizedMagnitudeAt(size_t x, size_t y)
{
    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache->haveSetColumnAt(col)) {
        fillColumn(x);
    }
    return cache->getNormalizedMagnitudeAt(col, y);
}

float
FFTDataServer::getMaximumMagnitudeAt(size_t x)
{
    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache->haveSetColumnAt(col)) {
        fillColumn(x);
    }
    return cache->getMaximumMagnitudeAt(col);
}

float
FFTDataServer::getPhaseAt(size_t x, size_t y)
{
    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache->haveSetColumnAt(col)) {
        fillColumn(x);
    }
    return cache->getPhaseAt(col, y);
}

void
FFTDataServer::getValuesAt(size_t x, size_t y, float &real, float &imaginary)
{
    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache->haveSetColumnAt(col)) {
#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::getValuesAt(" << x << ", " << y << "): filling" << std::endl;
#endif
        fillColumn(x);
    }        
    float magnitude = cache->getMagnitudeAt(col, y);
    float phase = cache->getPhaseAt(col, y);
    real = magnitude * cosf(phase);
    imaginary = magnitude * sinf(phase);
}

bool
FFTDataServer::isColumnReady(size_t x)
{
    if (!haveCache(x)) {
        if (m_lastUsedCache == -1) {
            m_fillThread->start();
        }
        return false;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);

    return cache->haveSetColumnAt(col);
}    

void
FFTDataServer::fillColumn(size_t x)
{
    size_t col;
#ifdef DEBUG_FFT_SERVER_FILL
    std::cout << "FFTDataServer::fillColumn(" << x << ")" << std::endl;
#endif
    FFTCache *cache = getCache(x, col);

    QMutexLocker locker(&m_writeMutex);

    if (cache->haveSetColumnAt(col)) return;

    int startFrame = m_windowIncrement * x;
    int endFrame = startFrame + m_windowSize;

    startFrame -= int(m_windowSize - m_windowIncrement) / 2;
    endFrame   -= int(m_windowSize - m_windowIncrement) / 2;
    size_t pfx = 0;

    size_t off = (m_fftSize - m_windowSize) / 2;

    for (size_t i = 0; i < off; ++i) {
        m_fftInput[i] = 0.0;
        m_fftInput[m_fftSize - i - 1] = 0.0;
    }

    if (startFrame < 0) {
	pfx = size_t(-startFrame);
	for (size_t i = 0; i < pfx; ++i) {
	    m_fftInput[off + i] = 0.0;
	}
    }

    size_t got = m_model->getValues(m_channel, startFrame + pfx,
				    endFrame, m_fftInput + off + pfx);

    while (got + pfx < m_windowSize) {
	m_fftInput[off + got + pfx] = 0.0;
	++got;
    }

    if (m_channel == -1) {
	int channels = m_model->getChannelCount();
	if (channels > 1) {
	    for (size_t i = 0; i < m_windowSize; ++i) {
		m_fftInput[off + i] /= channels;
	    }
	}
    }

    m_windower.cut(m_fftInput + off);

    for (size_t i = 0; i < m_fftSize/2; ++i) {
	fftsample temp = m_fftInput[i];
	m_fftInput[i] = m_fftInput[i + m_fftSize/2];
	m_fftInput[i + m_fftSize/2] = temp;
    }

    fftwf_execute(m_fftPlan);

    fftsample factor = 0.0;

    for (size_t i = 0; i < m_fftSize/2; ++i) {

	fftsample mag = sqrtf(m_fftOutput[i][0] * m_fftOutput[i][0] +
                              m_fftOutput[i][1] * m_fftOutput[i][1]);
	mag /= m_windowSize / 2;

	if (mag > factor) factor = mag;

	fftsample phase = atan2f(m_fftOutput[i][1], m_fftOutput[i][0]);
	phase = princargf(phase);

        m_workbuffer[i] = mag;
        m_workbuffer[i + m_fftSize/2] = phase;
    }

    cache->setColumnAt(col,
                       m_workbuffer,
                       m_workbuffer + m_fftSize/2,
                       factor);
}    

size_t
FFTDataServer::getFillCompletion() const 
{
    if (m_fillThread) return m_fillThread->getCompletion();
    else return 100;
}

size_t
FFTDataServer::getFillExtent() const
{
    if (m_fillThread) return m_fillThread->getExtent();
    else return m_model->getEndFrame();
}

QString
FFTDataServer::generateFileBasename() const
{
    return generateFileBasename(m_model, m_channel, m_windower.getType(),
                                m_windowSize, m_windowIncrement, m_fftSize,
                                m_polar);
}

QString
FFTDataServer::generateFileBasename(const DenseTimeValueModel *model,
                                    int channel,
                                    WindowType windowType,
                                    size_t windowSize,
                                    size_t windowIncrement,
                                    size_t fftSize,
                                    bool polar)
{
    char buffer[200];

    sprintf(buffer, "%u-%u-%u-%u-%u-%u%s",
            (unsigned int)XmlExportable::getObjectExportId(model),
            (unsigned int)(channel + 1),
            (unsigned int)windowType,
            (unsigned int)windowSize,
            (unsigned int)windowIncrement,
            (unsigned int)fftSize,
            polar ? "-p" : "-r");

    return buffer;
}

void
FFTDataServer::FillThread::run()
{
    m_extent = 0;
    m_completion = 0;
    
    size_t start = m_server.m_model->getStartFrame();
    size_t end = m_server.m_model->getEndFrame();
    size_t remainingEnd = end;

    int counter = 0;
    int updateAt = (end / m_server.m_windowIncrement) / 20;
    if (updateAt < 100) updateAt = 100;

    if (m_fillFrom > start) {

        for (size_t f = m_fillFrom; f < end; f += m_server.m_windowIncrement) {
	    
            m_server.fillColumn(int((f - start) / m_server.m_windowIncrement));

            if (m_server.m_exiting) return;

            while (m_server.m_suspended) {
#ifdef DEBUG_FFT_SERVER
                std::cerr << "FFTDataServer(" << this << "): suspended, waiting..." << std::endl;
#endif
                m_server.m_writeMutex.lock();
                m_server.m_condition.wait(&m_server.m_writeMutex, 10000);
                m_server.m_writeMutex.unlock();
                if (m_server.m_exiting) return;
            }

            if (++counter == updateAt) {
                m_extent = f;
                m_completion = size_t(100 * fabsf(float(f - m_fillFrom) /
                                                  float(end - start)));
                counter = 0;
            }
        }

        remainingEnd = m_fillFrom;
        if (remainingEnd > start) --remainingEnd;
        else remainingEnd = start;
    }

    size_t baseCompletion = m_completion;

    for (size_t f = start; f < remainingEnd; f += m_server.m_windowIncrement) {

        m_server.fillColumn(int((f - start) / m_server.m_windowIncrement));

        if (m_server.m_exiting) return;

        while (m_server.m_suspended) {
#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer(" << this << "): suspended, waiting..." << std::endl;
#endif
            m_server.m_writeMutex.lock();
            m_server.m_condition.wait(&m_server.m_writeMutex, 10000);
            m_server.m_writeMutex.unlock();
            if (m_server.m_exiting) return;
        }
		    
        if (++counter == updateAt) {
            m_extent = f;
            m_completion = baseCompletion +
                size_t(100 * fabsf(float(f - start) /
                                   float(end - start)));
            counter = 0;
        }
    }

    m_completion = 100;
    m_extent = end;
}

