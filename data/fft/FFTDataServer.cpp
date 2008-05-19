/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FFTDataServer.h"

#include "FFTFileCache.h"
#include "FFTMemoryCache.h"

#include "model/DenseTimeValueModel.h"

#include "system/System.h"

#include "base/StorageAdviser.h"
#include "base/Exceptions.h"
#include "base/Profiler.h"
#include "base/Thread.h" // for debug mutex locker

//#define DEBUG_FFT_SERVER 1
//#define DEBUG_FFT_SERVER_FILL 1

#ifdef DEBUG_FFT_SERVER_FILL
#ifndef DEBUG_FFT_SERVER
#define DEBUG_FFT_SERVER 1
#endif
#endif


FFTDataServer::ServerMap FFTDataServer::m_servers;
FFTDataServer::ServerQueue FFTDataServer::m_releasedServers;
QMutex FFTDataServer::m_serverMapMutex;

FFTDataServer *
FFTDataServer::getInstance(const DenseTimeValueModel *model,
                           int channel,
                           WindowType windowType,
                           size_t windowSize,
                           size_t windowIncrement,
                           size_t fftSize,
                           bool polar,
                           StorageAdviser::Criteria criteria,
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
    
    MutexLocker locker(&m_serverMapMutex, "FFTDataServer::getInstance::m_serverMapMutex");

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

    try {
        server = new FFTDataServer(n,
                                   model,
                                   channel,
                                   windowType,
                                   windowSize,
                                   windowIncrement,
                                   fftSize,
                                   polar,
                                   criteria,
                                   fillFromColumn);
    } catch (InsufficientDiscSpace) {
        delete server;
        server = 0;
    }

    if (server) {
        m_servers[n] = ServerCountPair(server, 1);
    }

    return server;
}

FFTDataServer *
FFTDataServer::getFuzzyInstance(const DenseTimeValueModel *model,
                                int channel,
                                WindowType windowType,
                                size_t windowSize,
                                size_t windowIncrement,
                                size_t fftSize,
                                bool polar,
                                StorageAdviser::Criteria criteria,
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
        MutexLocker locker(&m_serverMapMutex, "FFTDataServer::getFuzzyInstance::m_serverMapMutex");

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
                std::cerr << "FFTDataServer::getFuzzyInstance: Distance for server " << server << " is " << distance << ", best is " << bestdist << std::endl;
#endif
                
                if (bestdist == -1 || distance < bestdist) {
                    bestdist = distance;
                    best = i;
                }
            }
        }

        if (bestdist >= 0) {
            FFTDataServer *server = best->second.first;
#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer::getFuzzyInstance: We like server " << server << " (with distance " << bestdist << ")" << std::endl;
#endif
            claimInstance(server, false);
            return server;
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
                       criteria,
                       fillFromColumn);
}

FFTDataServer *
FFTDataServer::findServer(QString n)
{    
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::findServer(\"" << n.toStdString() << "\")" << std::endl;
#endif

    if (m_servers.find(n) != m_servers.end()) {

        FFTDataServer *server = m_servers[n].first;

#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::findServer(\"" << n.toStdString() << "\"): found " << server << std::endl;
#endif

        claimInstance(server, false);

        return server;
    }

#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::findServer(\"" << n.toStdString() << "\"): not found" << std::endl;
#endif

    return 0;
}

void
FFTDataServer::claimInstance(FFTDataServer *server)
{
    claimInstance(server, true);
}

void
FFTDataServer::claimInstance(FFTDataServer *server, bool needLock)
{
    MutexLocker locker(needLock ? &m_serverMapMutex : 0,
                       "FFTDataServer::claimInstance::m_serverMapMutex");

#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::claimInstance(" << server << ")" << std::endl;
#endif

    for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {
        if (i->second.first == server) {

            for (ServerQueue::iterator j = m_releasedServers.begin();
                 j != m_releasedServers.end(); ++j) {

                if (*j == server) {
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::claimInstance: found in released server list, removing from it" << std::endl;
#endif
                    m_releasedServers.erase(j);
                    break;
                }
            }

            ++i->second.second;

#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer::claimInstance: new refcount is " << i->second.second << std::endl;
#endif

            return;
        }
    }
    
    std::cerr << "ERROR: FFTDataServer::claimInstance: instance "
              << server << " unknown!" << std::endl;
}

void
FFTDataServer::releaseInstance(FFTDataServer *server)
{
    releaseInstance(server, true);
}

void
FFTDataServer::releaseInstance(FFTDataServer *server, bool needLock)
{    
    MutexLocker locker(needLock ? &m_serverMapMutex : 0,
                       "FFTDataServer::releaseInstance::m_serverMapMutex");

#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::releaseInstance(" << server << ")" << std::endl;
#endif

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

    for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {
        if (i->second.first == server) {
            if (i->second.second == 0) {
                std::cerr << "ERROR: FFTDataServer::releaseInstance("
                          << server << "): instance not allocated" << std::endl;
            } else if (--i->second.second == 0) {
                if (server->m_lastUsedCache == -1) { // never used
#ifdef DEBUG_FFT_SERVER
                    std::cerr << "FFTDataServer::releaseInstance: instance "
                              << server << " has never been used, erasing"
                              << std::endl;
#endif
                    delete server;
                    m_servers.erase(i);
                } else {
#ifdef DEBUG_FFT_SERVER
                    std::cerr << "FFTDataServer::releaseInstance: instance "
                              << server << " no longer in use, marking for possible collection"
                              << std::endl;
#endif
                    bool found = false;
                    for (ServerQueue::iterator j = m_releasedServers.begin();
                         j != m_releasedServers.end(); ++j) {
                        if (*j == server) {
                            std::cerr << "ERROR: FFTDataServer::releaseInstance("
                                      << server << "): server is already in "
                                      << "released servers list" << std::endl;
                            found = true;
                        }
                    }
                    if (!found) m_releasedServers.push_back(server);
                    server->suspend();
                    purgeLimbo();
                }
            } else {
#ifdef DEBUG_FFT_SERVER
                    std::cerr << "FFTDataServer::releaseInstance: instance "
                              << server << " now has refcount " << i->second.second
                              << std::endl;
#endif
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
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::purgeLimbo(" << maxSize << "): "
              << m_releasedServers.size() << " candidates" << std::endl;
#endif

    while (int(m_releasedServers.size()) > maxSize) {

        FFTDataServer *server = *m_releasedServers.begin();

        bool found = false;

#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::purgeLimbo: considering candidate "
                  << server << std::endl;
#endif

        for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {

            if (i->second.first == server) {
                found = true;
                if (i->second.second > 0) {
                    std::cerr << "ERROR: FFTDataServer::purgeLimbo: Server "
                              << server << " is in released queue, but still has non-zero refcount "
                              << i->second.second << std::endl;
                    // ... so don't delete it
                    break;
                }
#ifdef DEBUG_FFT_SERVER
                std::cerr << "FFTDataServer::purgeLimbo: looks OK, erasing it"
                          << std::endl;
#endif

                m_servers.erase(i);
                delete server;
                break;
            }
        }

        if (!found) {
            std::cerr << "ERROR: FFTDataServer::purgeLimbo: Server "
                      << server << " is in released queue, but not in server map!"
                      << std::endl;
            delete server;
        }

        m_releasedServers.pop_front();
    }

#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::purgeLimbo(" << maxSize << "): "
              << m_releasedServers.size() << " remain" << std::endl;
#endif

}

void
FFTDataServer::modelAboutToBeDeleted(Model *model)
{
    MutexLocker locker(&m_serverMapMutex,
                       "FFTDataServer::modelAboutToBeDeleted::m_serverMapMutex");

#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer::modelAboutToBeDeleted(" << model << ")"
              << std::endl;
#endif

    for (ServerMap::iterator i = m_servers.begin(); i != m_servers.end(); ++i) {
        
        FFTDataServer *server = i->second.first;

        if (server->getModel() == model) {

#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer::modelAboutToBeDeleted: server is "
                      << server << std::endl;
#endif

            if (i->second.second > 0) {
                std::cerr << "WARNING: FFTDataServer::modelAboutToBeDeleted: Model " << model << " (\"" << model->objectName().toStdString() << "\") is about to be deleted, but is still being referred to by FFT server " << server << " with non-zero refcount " << i->second.second << std::endl;
                return;
            }
            for (ServerQueue::iterator j = m_releasedServers.begin();
                 j != m_releasedServers.end(); ++j) {
                if (*j == server) {
#ifdef DEBUG_FFT_SERVER
                    std::cerr << "FFTDataServer::modelAboutToBeDeleted: erasing from released servers" << std::endl;
#endif
                    m_releasedServers.erase(j);
                    break;
                }
            }
#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer::modelAboutToBeDeleted: erasing server" << std::endl;
#endif
            m_servers.erase(i);
            delete server;
            return;
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
                             StorageAdviser::Criteria criteria,
                             size_t fillFromColumn) :
    m_fileBaseName(fileBaseName),
    m_model(model),
    m_channel(channel),
    m_windower(windowType, windowSize),
    m_windowSize(windowSize),
    m_windowIncrement(windowIncrement),
    m_fftSize(fftSize),
    m_polar(polar),
    m_width(0),
    m_height(0),
    m_cacheWidth(0),
    m_cacheWidthPower(0),
    m_cacheWidthMask(0),
    m_lastUsedCache(-1),
    m_criteria(criteria),
    m_fftInput(0),
    m_exiting(false),
    m_suspended(true), //!!! or false?
    m_fillThread(0)
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "])::FFTDataServer" << std::endl;
#endif

    //!!! end is not correct until model finished reading -- what to do???

    size_t start = m_model->getStartFrame();
    size_t end = m_model->getEndFrame();

    m_width = (end - start) / m_windowIncrement + 1;
    m_height = m_fftSize / 2 + 1; // DC == 0, Nyquist == fftsize/2

#ifdef DEBUG_FFT_SERVER 
    std::cerr << "FFTDataServer(" << this << "): dimensions are "
              << m_width << "x" << m_height << std::endl;
#endif

    size_t maxCacheSize = 20 * 1024 * 1024;
    size_t columnSize = m_height * sizeof(fftsample) * 2 + sizeof(fftsample);
    if (m_width * columnSize < maxCacheSize * 2) m_cacheWidth = m_width;
    else m_cacheWidth = maxCacheSize / columnSize;
    
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << "): cache width nominal "
              << m_cacheWidth << ", actual ";
#endif
    
    int bits = 0;
    while (m_cacheWidth > 1) { m_cacheWidth >>= 1; ++bits; }
    m_cacheWidthPower = bits + 1;
    m_cacheWidth = 2;
    while (bits) { m_cacheWidth <<= 1; --bits; }
    m_cacheWidthMask = m_cacheWidth - 1;

#ifdef DEBUG_FFT_SERVER
    std::cerr << m_cacheWidth << " (power " << m_cacheWidthPower << ", mask "
              << m_cacheWidthMask << ")" << std::endl;
#endif

    if (m_criteria == StorageAdviser::NoCriteria) {

        // assume "spectrogram" criteria for polar ffts, and "feature
        // extraction" criteria for rectangular ones.

        if (m_polar) {
            m_criteria = StorageAdviser::Criteria
                (StorageAdviser::SpeedCritical |
                 StorageAdviser::LongRetentionLikely);
        } else {
            m_criteria = StorageAdviser::Criteria
                (StorageAdviser::PrecisionCritical);
        }
    }

    for (size_t i = 0; i <= m_width / m_cacheWidth; ++i) {
        m_caches.push_back(0);
    }

    m_fftInput = (fftsample *)
        fftf_malloc(fftSize * sizeof(fftsample));

    m_fftOutput = (fftf_complex *)
        fftf_malloc((fftSize/2 + 1) * sizeof(fftf_complex));

    m_workbuffer = (float *)
        fftf_malloc((fftSize+2) * sizeof(float));

    m_fftPlan = fftf_plan_dft_r2c_1d(m_fftSize,
                                     m_fftInput,
                                     m_fftOutput,
                                     FFTW_MEASURE);

    if (!m_fftPlan) {
        std::cerr << "ERROR: fftf_plan_dft_r2c_1d(" << m_windowSize << ") failed!" << std::endl;
        throw(0);
    }

    m_fillThread = new FillThread(*this, fillFromColumn);
}

FFTDataServer::~FFTDataServer()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "])::~FFTDataServer()" << std::endl;
#endif

    m_suspended = false;
    m_exiting = true;
    m_condition.wakeAll();
    if (m_fillThread) {
        m_fillThread->wait();
        delete m_fillThread;
    }

    MutexLocker locker(&m_writeMutex,
                       "FFTDataServer::~FFTDataServer::m_writeMutex");

    for (CacheVector::iterator i = m_caches.begin(); i != m_caches.end(); ++i) {

        if (*i) {
            delete *i;
        }
    }

    deleteProcessingData();
}

void
FFTDataServer::deleteProcessingData()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): deleteProcessingData" << std::endl;
#endif
    if (m_fftInput) {
        fftf_destroy_plan(m_fftPlan);
        fftf_free(m_fftInput);
        fftf_free(m_fftOutput);
        fftf_free(m_workbuffer);
    }
    m_fftInput = 0;
}

void
FFTDataServer::suspend()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): suspend" << std::endl;
#endif
    Profiler profiler("FFTDataServer::suspend", false);

    MutexLocker locker(&m_writeMutex,
                       "FFTDataServer::suspend::m_writeMutex");
    m_suspended = true;
    for (CacheVector::iterator i = m_caches.begin(); i != m_caches.end(); ++i) {
        if (*i) (*i)->suspend();
    }
}

void
FFTDataServer::suspendWrites()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): suspendWrites" << std::endl;
#endif
    Profiler profiler("FFTDataServer::suspendWrites", false);

    m_suspended = true;
}

void
FFTDataServer::resume()
{
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): resume" << std::endl;
#endif
    Profiler profiler("FFTDataServer::resume", false);

    m_suspended = false;
    if (m_fillThread) {
        if (m_fillThread->isFinished()) {
            delete m_fillThread;
            m_fillThread = 0;
            deleteProcessingData();
        } else if (!m_fillThread->isRunning()) {
            m_fillThread->start();
        } else {
            m_condition.wakeAll();
        }
    }
}

void
FFTDataServer::getStorageAdvice(size_t w, size_t h,
                                bool &memoryCache, bool &compactCache)
{
    int cells = w * h;
    int minimumSize = (cells / 1024) * sizeof(uint16_t); // kb
    int maximumSize = (cells / 1024) * sizeof(float); // kb

    // We don't have a compact rectangular representation, and compact
    // of course is never precision-critical

    bool canCompact = true;
    if ((m_criteria & StorageAdviser::PrecisionCritical) || !m_polar) {
        canCompact = false;
        minimumSize = maximumSize; // don't use compact
    }
    
    StorageAdviser::Recommendation recommendation;

    try {

        recommendation =
            StorageAdviser::recommend(m_criteria, minimumSize, maximumSize);

    } catch (InsufficientDiscSpace s) {

        // Delete any unused servers we may have been leaving around
        // in case we wanted them again

        purgeLimbo(0);

        // This time we don't catch InsufficientDiscSpace -- we
        // haven't allocated anything yet and can safely let the
        // exception out to indicate to the caller that we can't
        // handle it.

        recommendation =
            StorageAdviser::recommend(m_criteria, minimumSize, maximumSize);
    }

    std::cerr << "Recommendation was: " << recommendation << std::endl;

    memoryCache = false;

    if ((recommendation & StorageAdviser::UseMemory) ||
        (recommendation & StorageAdviser::PreferMemory)) {
        memoryCache = true;
    }

    compactCache = canCompact &&
        (recommendation & StorageAdviser::ConserveSpace);

#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer: memory cache = " << memoryCache << ", compact cache = " << compactCache << std::endl;
    
    std::cerr << "Width " << w << " of " << m_width << ", height " << h << ", size " << w * h << std::endl;
#endif
}

FFTCache *
FFTDataServer::getCacheAux(size_t c)
{
    Profiler profiler("FFTDataServer::getCacheAux", false);
#ifdef DEBUG_FFT_SERVER
    std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "])::getCacheAux" << std::endl;
#endif

    MutexLocker locker(&m_writeMutex,
                       "FFTDataServer::getCacheAux::m_writeMutex");

    if (m_lastUsedCache == -1) {
        m_fillThread->start();
    }

    if (int(c) != m_lastUsedCache) {

//        std::cerr << "switch from " << m_lastUsedCache << " to " << c << std::endl;

        for (IntQueue::iterator i = m_dormantCaches.begin();
             i != m_dormantCaches.end(); ++i) {
            if (*i == int(c)) {
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

    FFTCache *cache = 0;

    size_t width = m_cacheWidth;
    if (c * m_cacheWidth + width > m_width) {
        width = m_width - c * m_cacheWidth;
    }

    bool memoryCache = false;
    bool compactCache = false;

    getStorageAdvice(width, m_height, memoryCache, compactCache);

    try {

        if (memoryCache) {

            cache = new FFTMemoryCache
                (compactCache ? FFTMemoryCache::Compact :
                      m_polar ? FFTMemoryCache::Polar :
                                FFTMemoryCache::Rectangular);

        } else {

            cache = new FFTFileCache
                (name,
                 MatrixFile::ReadWrite,
                 compactCache ? FFTFileCache::Compact :
                      m_polar ? FFTFileCache::Polar :
                                FFTFileCache::Rectangular);
        }

        cache->resize(width, m_height);
        cache->reset();

    } catch (std::bad_alloc) {

        delete cache;
        cache = 0;

        if (memoryCache) {
            
            std::cerr << "WARNING: Memory allocation failed when resizing"
                      << " FFT memory cache no. " << c << " to " << width 
                      << "x" << m_height << " (of total width " << m_width
                      << "): falling back to disc cache" << std::endl;

            try {

                purgeLimbo(0);

                cache = new FFTFileCache(name,
                                         MatrixFile::ReadWrite,
                                         FFTFileCache::Compact);

                cache->resize(width, m_height);
                cache->reset();

            } catch (std::bad_alloc) {

                delete cache;
                cache = 0;
            }
        }

        if (!cache) {
            std::cerr << "ERROR: Memory allocation failed when resizing"
                      << " FFT file cache no. " << c << " to " << width
                      << "x" << m_height << " (of total width " << m_width
                      << "): abandoning this cache" << std::endl;

            throw AllocationFailed("Failed to create or resize an FFT model slice");
        }
    }

    m_caches[c] = cache;
    m_lastUsedCache = c;
    return cache;
}

float
FFTDataServer::getMagnitudeAt(size_t x, size_t y)
{
    Profiler profiler("FFTDataServer::getMagnitudeAt", false);

    if (x >= m_width || y >= m_height) return 0;

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return 0;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getMagnitudeAt: filling");
#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::getMagnitudeAt: calling fillColumn(" 
                  << x << ")" << std::endl;
#endif
        // hold mutex so that write thread doesn't mess with class
        // member data in fillColumn
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getMagnitudeAt: m_writeMutex");
        fillColumn(x, true);
    }
    return cache->getMagnitudeAt(col, y);
}

bool
FFTDataServer::getMagnitudesAt(size_t x, float *values, size_t minbin, size_t count, size_t step)
{
    Profiler profiler("FFTDataServer::getMagnitudesAt", false);

    if (x >= m_width) return false;

    if (minbin >= m_height) minbin = m_height - 1;
    if (count == 0) count = (m_height - minbin) / step;
    else if (minbin + count * step > m_height) {
        count = (m_height - minbin) / step;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return false;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getMagnitudesAt: filling");
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getMagnitudesAt: m_writeMutex");
        fillColumn(x, true);
    }

    for (size_t i = 0; i < count; ++i) {
        values[i] = cache->getMagnitudeAt(col, i * step + minbin);
    }

    return true;
}

float
FFTDataServer::getNormalizedMagnitudeAt(size_t x, size_t y)
{
    Profiler profiler("FFTDataServer::getNormalizedMagnitudeAt", false);

    if (x >= m_width || y >= m_height) return 0;

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return 0;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getNormalizedMagnitudeAt: filling");
        // hold mutex so that write thread doesn't mess with class
        // member data in fillColumn
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getNormalizedMagnitudeAt: m_writeMutex");
        fillColumn(x, true);
    }
    return cache->getNormalizedMagnitudeAt(col, y);
}

bool
FFTDataServer::getNormalizedMagnitudesAt(size_t x, float *values, size_t minbin, size_t count, size_t step)
{
    Profiler profiler("FFTDataServer::getNormalizedMagnitudesAt", false);

    if (x >= m_width) return false;

    if (minbin >= m_height) minbin = m_height - 1;
    if (count == 0) count = (m_height - minbin) / step;
    else if (minbin + count * step > m_height) {
        count = (m_height - minbin) / step;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return false;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getNormalizedMagnitudesAt: filling");
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getNormalizedMagnitudesAt: m_writeMutex");
        fillColumn(x, true);
    }

    for (size_t i = 0; i < count; ++i) {
        values[i] = cache->getNormalizedMagnitudeAt(col, i * step + minbin);
    }

    return true;
}

float
FFTDataServer::getMaximumMagnitudeAt(size_t x)
{
    Profiler profiler("FFTDataServer::getMaximumMagnitudeAt", false);

    if (x >= m_width) return 0;

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return 0;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getMaximumMagnitudeAt: filling");
        // hold mutex so that write thread doesn't mess with class
        // member data in fillColumn
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getMaximumMagnitudeAt: m_writeMutex");
        fillColumn(x, true);
    }
    return cache->getMaximumMagnitudeAt(col);
}

float
FFTDataServer::getPhaseAt(size_t x, size_t y)
{
    Profiler profiler("FFTDataServer::getPhaseAt", false);

    if (x >= m_width || y >= m_height) return 0;

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return 0;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getPhaseAt: filling");
        // hold mutex so that write thread doesn't mess with class
        // member data in fillColumn
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getPhaseAt: m_writeMutex");
        fillColumn(x, true);
    }
    return cache->getPhaseAt(col, y);
}

bool
FFTDataServer::getPhasesAt(size_t x, float *values, size_t minbin, size_t count, size_t step)
{
    Profiler profiler("FFTDataServer::getPhasesAt", false);

    if (x >= m_width) return false;

    if (minbin >= m_height) minbin = m_height - 1;
    if (count == 0) count = (m_height - minbin) / step;
    else if (minbin + count * step > m_height) {
        count = (m_height - minbin) / step;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return false;

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getPhasesAt: filling");
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getPhasesAt: m_writeMutex");
        fillColumn(x, true);
    }

    for (size_t i = 0; i < count; ++i) {
        values[i] = cache->getPhaseAt(col, i * step + minbin);
    }

    return true;
}

void
FFTDataServer::getValuesAt(size_t x, size_t y, float &real, float &imaginary)
{
    Profiler profiler("FFTDataServer::getValuesAt", false);

    if (x >= m_width || y >= m_height) {
        real = 0;
        imaginary = 0;
        return;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);

    if (!cache) {
        real = 0;
        imaginary = 0;
        return;
    }

    if (!cache->haveSetColumnAt(col)) {
        Profiler profiler("FFTDataServer::getValuesAt: filling");
#ifdef DEBUG_FFT_SERVER
        std::cerr << "FFTDataServer::getValuesAt(" << x << ", " << y << "): filling" << std::endl;
#endif
        // hold mutex so that write thread doesn't mess with class
        // member data in fillColumn
        MutexLocker locker(&m_writeMutex,
                           "FFTDataServer::getValuesAt: m_writeMutex");
        fillColumn(x, true);
    }        

    cache->getValuesAt(col, y, real, imaginary);
}

bool
FFTDataServer::isColumnReady(size_t x)
{
    Profiler profiler("FFTDataServer::isColumnReady", false);

    if (x >= m_width) return true;

    if (!haveCache(x)) {
        if (m_lastUsedCache == -1) {
            if (m_suspended) {
                std::cerr << "FFTDataServer::isColumnReady(" << x << "): no cache, calling resume" << std::endl;
                resume();
            }
            m_fillThread->start();
        }
        return false;
    }

    size_t col;
    FFTCache *cache = getCache(x, col);
    if (!cache) return true;

    return cache->haveSetColumnAt(col);
}    

void
FFTDataServer::fillColumn(size_t x, bool lockHeld)
{
    Profiler profiler("FFTDataServer::fillColumn", false);

    if (!m_model->isReady()) {
        std::cerr << "WARNING: FFTDataServer::fillColumn(" 
                  << x << "): model not yet ready" << std::endl;
        return;
    }

    if (!m_fftInput) {
        std::cerr << "WARNING: FFTDataServer::fillColumn(" << x << "): "
                  << "input has already been completed and discarded?"
                  << std::endl;
        return;
    }

    if (x >= m_width) {
        std::cerr << "WARNING: FFTDataServer::fillColumn(" << x << "): "
                  << "x > width (" << x << " > " << m_width << ")"
                  << std::endl;
//        abort(); //!!!
        return;
    }

    size_t col;
#ifdef DEBUG_FFT_SERVER_FILL
    std::cout << "FFTDataServer::fillColumn(" << x << ")" << std::endl;
#endif
    FFTCache *cache = getCache(x, col);
    if (!cache) return;

    {
        MutexLocker locker(lockHeld ? 0 : &m_writeMutex,
                           "FFTDataServer::fillColumn::m_writeMutex [1]");

        if (cache->haveSetColumnAt(col)) return;
    }

    int winsize = m_windowSize;
    int fftsize = m_fftSize;
    int hs = fftsize/2;

    int pfx = 0;
    int off = (fftsize - winsize) / 2;

    int startFrame = m_windowIncrement * x;
    int endFrame = startFrame + m_windowSize;

    startFrame -= winsize / 2;
    endFrame   -= winsize / 2;

    for (int i = 0; i < off; ++i) {
        m_fftInput[i] = 0.0;
    }

    for (int i = 0; i < off; ++i) {
        m_fftInput[fftsize - i - 1] = 0.0;
    }

    if (startFrame < 0) {
	pfx = -startFrame;
	for (int i = 0; i < pfx; ++i) {
	    m_fftInput[off + i] = 0.0;
	}
    }

#ifdef DEBUG_FFT_SERVER_FILL
    std::cerr << "FFTDataServer::fillColumn: requesting frames "
              << startFrame + pfx << " -> " << endFrame << " ( = "
              << endFrame - (startFrame + pfx) << ") at index "
              << off + pfx << " in buffer of size " << m_fftSize
              << " with window size " << m_windowSize 
              << " from channel " << m_channel << std::endl;
#endif

    int count = 0;
    if (endFrame > startFrame + pfx) count = endFrame - (startFrame + pfx);

    int got = m_model->getData(m_channel, startFrame + pfx,
                               count, m_fftInput + off + pfx);

    while (got + pfx < winsize) {
	m_fftInput[off + got + pfx] = 0.0;
	++got;
    }

    if (m_channel == -1) {
	int channels = m_model->getChannelCount();
	if (channels > 1) {
	    for (int i = 0; i < winsize; ++i) {
		m_fftInput[off + i] /= channels;
	    }
	}
    }

    m_windower.cut(m_fftInput + off);

    for (int i = 0; i < hs; ++i) {
	fftsample temp = m_fftInput[i];
	m_fftInput[i] = m_fftInput[i + hs];
	m_fftInput[i + hs] = temp;
    }

    fftf_execute(m_fftPlan);

    // If our cache uses polar storage, it's more friendly for us to
    // do the conversion before taking the write mutex

    float factor = 0.f;

    if (cache->getStorageType() == FFTCache::Compact ||
        cache->getStorageType() == FFTCache::Polar) {

        for (int i = 0; i <= hs; ++i) {
            fftsample real = m_fftOutput[i][0];
            fftsample imag = m_fftOutput[i][1];
            float mag = sqrtf(real * real + imag * imag);
            m_workbuffer[i] = mag;
            m_workbuffer[i + hs + 1] = atan2f(imag, real);
            if (mag > factor) factor = mag;
        }

    } else {

        for (int i = 0; i <= hs; ++i) {
            m_workbuffer[i] = m_fftOutput[i][0];
            m_workbuffer[i + hs + 1] = m_fftOutput[i][1];
        }
    }

    Profiler subprof("FFTDataServer::fillColumn: set to cache");

    {
        MutexLocker locker(lockHeld ? 0 : &m_writeMutex,
                           "FFTDataServer::fillColumn: m_writeMutex [2]");

        if (cache->getStorageType() == FFTCache::Compact ||
            cache->getStorageType() == FFTCache::Polar) {
            
            cache->setColumnAt(col,
                               m_workbuffer,
                               m_workbuffer + hs + 1,
                               factor);

        } else {

            cache->setColumnAt(col,
                               m_workbuffer,
                               m_workbuffer + hs + 1);
        }
    }

    if (m_suspended) {
//        std::cerr << "FFTDataServer::fillColumn(" << x << "): calling resume" << std::endl;
//        resume();
    }
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
#ifdef DEBUG_FFT_SERVER_FILL
    std::cerr << "FFTDataServer::FillThread::run()" << std::endl;
#endif
    
    m_extent = 0;
    m_completion = 0;
    
    while (!m_server.m_model->isReady() && !m_server.m_exiting) {
#ifdef DEBUG_FFT_SERVER_FILL
        std::cerr << "FFTDataServer::FillThread::run(): waiting for model " << m_server.m_model << " to be ready" << std::endl;
#endif
        sleep(1);
    }
    if (m_server.m_exiting) return;

    size_t start = m_server.m_model->getStartFrame();
    size_t end = m_server.m_model->getEndFrame();
    size_t remainingEnd = end;

    int counter = 0;
    int updateAt = 1;
    int maxUpdateAt = (end / m_server.m_windowIncrement) / 20;
    if (maxUpdateAt < 100) maxUpdateAt = 100;

    if (m_fillFrom > start) {

        for (size_t f = m_fillFrom; f < end; f += m_server.m_windowIncrement) {
	    
            m_server.fillColumn(int((f - start) / m_server.m_windowIncrement),
                                false);

            if (m_server.m_exiting) return;

            while (m_server.m_suspended) {
#ifdef DEBUG_FFT_SERVER
                std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): suspended, waiting..." << std::endl;
#endif
                {
                    MutexLocker locker(&m_server.m_writeMutex,
                                       "FFTDataServer::run::m_writeMutex [1]");
                    m_server.m_condition.wait(&m_server.m_writeMutex, 10000);
                }
#ifdef DEBUG_FFT_SERVER
                std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): waited" << std::endl;
#endif
                if (m_server.m_exiting) return;
            }

            if (++counter == updateAt) {
                m_extent = f;
                m_completion = size_t(100 * fabsf(float(f - m_fillFrom) /
                                                  float(end - start)));
                counter = 0;
                if (updateAt < maxUpdateAt) {
                    updateAt *= 2;
                    if (updateAt > maxUpdateAt) updateAt = maxUpdateAt;
                }
            }
        }

        remainingEnd = m_fillFrom;
        if (remainingEnd > start) --remainingEnd;
        else remainingEnd = start;
    }

    size_t baseCompletion = m_completion;

    for (size_t f = start; f < remainingEnd; f += m_server.m_windowIncrement) {

        m_server.fillColumn(int((f - start) / m_server.m_windowIncrement),
                            false);

        if (m_server.m_exiting) return;

        while (m_server.m_suspended) {
#ifdef DEBUG_FFT_SERVER
            std::cerr << "FFTDataServer(" << this << " [" << (void *)QThread::currentThreadId() << "]): suspended, waiting..." << std::endl;
#endif
            {
                MutexLocker locker(&m_server.m_writeMutex,
                                   "FFTDataServer::run::m_writeMutex [2]");
                m_server.m_condition.wait(&m_server.m_writeMutex, 10000);
            }
            if (m_server.m_exiting) return;
        }
		    
        if (++counter == updateAt) {
            m_extent = f;
            m_completion = baseCompletion +
                size_t(100 * fabsf(float(f - start) /
                                   float(end - start)));
            counter = 0;
            if (updateAt < maxUpdateAt) {
                updateAt *= 2;
                if (updateAt > maxUpdateAt) updateAt = maxUpdateAt;
            }
        }
    }

    m_completion = 100;
    m_extent = end;
}

