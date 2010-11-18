/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.

    Based on QTAudioFile.cpp from SoundBite, copyright 2006
    Chris Sutton and Mark Levy.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
 */

#ifdef HAVE_COREAUDIO

#include "CoreAudioFileReader.h"
#include "base/Profiler.h"
#include "base/ProgressReporter.h"
#include "system/System.h"

#include <QFileInfo>


// TODO: implement for windows
#ifdef _WIN32
#include <QTML.h>
#include <Movies.h>
#else


#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
#include <AudioToolbox/AudioToolbox.h>
#else
#include "AudioToolbox.h"
#include "ExtendedAudioFile.h"
#endif

#include "CAStreamBasicDescription.h"
#include "CAXException.h"




#endif

class CoreAudioFileReader::D
{
public:
  D() : data(0), blockSize(1024) { }

  AudioBufferList              buffer;
  float                        *data;
  OSErr                        err;
  AudioStreamBasicDescription  asbd;
  // file reference

  ExtAudioFileRef              *infile;
  size_t                       blockSize;
};


CoreAudioFileReader::CoreAudioFileReader(FileSource source,
    DecodeMode decodeMode,
    CacheMode mode,
    size_t targetRate,
    ProgressReporter *reporter) :
    CodedAudioFileReader(mode, targetRate),
    m_source(source),
    m_path(source.getLocalFilename()),
    m_d(new D),
    m_reporter(reporter),
    m_cancelled(false),
    m_completion(0),
    m_decodeThread(0)
{
  m_channelCount = 0;
  m_fileRate = 0;

  Profiler profiler("CoreAudioFileReader::CoreAudioFileReader", true);

  std::cerr << "CoreAudioFileReader: path is \"" << m_path.toStdString() << "\"" << std::endl;

  // TODO: check QT version

  Handle dataRef;
  OSType dataRefType;

  //    CFStringRef URLString = CFStringCreateWithCString
  //       (0, m_path.toLocal8Bit().data(), 0);

  // what are these used for???
  // ExtAudioFileRef *infile, outfile;

  // Creates a new CFURL object for a file system entity using the native representation.
  CFURLRef url = CFURLCreateFromFileSystemRepresentation
      (kCFAllocatorDefault,
          (const UInt8 *)m_path.toLocal8Bit().data(),
          (CFIndex)m_path.length(),
          false);

  // first open the input file
  m_d->err = ExtAudioFileOpenURL (url, m_d->infile);

  if (m_d->err) {
      m_error = QString("Error opening Audio File for CoreAudio decoder: code %1").arg(m_d->err);
      return;
  }
  else {
      std::cerr << "CoreAudio: opened URL" << std::endl;
  }


  // Get the audio data format
  // the audio format gets stored in the &m_d->asbd
  UInt32 thePropertySize = sizeof(m_d->asbd);


  std::cerr << "CoreAudio: thePropertySize: " << thePropertySize << std::endl;

  m_d->err = ExtAudioFileGetProperty(*m_d->infile, kAudioFilePropertyDataFormat, &thePropertySize, &m_d->asbd);

  std::cerr << "CoreAudio: ExtAudioFileGetProperty res: " << &m_d->asbd << std::endl;

  // CAStreamBasicDescription clientFormat = (inputFormat.mFormatID == kAudioFormatLinearPCM ? inputFormat : outputFormat);
  // UInt32 size = sizeof(clientFormat);

  // TODO: test input file's DRM rights

  // are these already set?
  m_channelCount = m_d->asbd.mChannelsPerFrame;
  m_fileRate = m_d->asbd.mSampleRate;

  std::cerr << "CoreAudio: Format ID: " << m_d->asbd.mFormatID << std::endl;
  std::cerr << "CoreAudio: " << m_d->asbd.mChannelsPerFrame << " channels, " << m_fileRate << " kHz" << std::endl;



  m_d->asbd.mFormatFlags =
      kAudioFormatFlagIsFloat |
      kAudioFormatFlagIsPacked |
      kAudioFormatFlagsNativeEndian;
  m_d->asbd.mBitsPerChannel = sizeof(float) * 8;
  m_d->asbd.mBytesPerFrame = sizeof(float) * m_d->asbd.mChannelsPerFrame;
  m_d->asbd.mBytesPerPacket = m_d->asbd.mBytesPerFrame;



  /*

  !!! what does this do exactly ?????

  m_d->err = MovieAudioExtractionSetProperty
      (m_d->extractionSessionRef,
          kQTPropertyClass_MovieAudioExtraction_Audio,
          kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
          sizeof(m_d->asbd),
          &m_d->asbd);



  if (m_d->err) {
      m_error = QString("Error in QuickTime decoder property set: code %1").arg(m_d->err);
      m_channelCount = 0;
      return;
  }

   */

  m_d->buffer.mNumberBuffers = 1;
  m_d->buffer.mBuffers[0].mNumberChannels = m_channelCount;
  m_d->buffer.mBuffers[0].mDataByteSize =
      sizeof(float) * m_channelCount * m_d->blockSize;
  m_d->data = new float[m_channelCount * m_d->blockSize];
  m_d->buffer.mBuffers[0].mData = m_d->data;

  initialiseDecodeCache();

  // only decode at once for now
  // if (decodeMode == DecodeAtOnce) {

  if (m_reporter) {
      connect(m_reporter, SIGNAL(cancelled()), this, SLOT(cancelled()));
      m_reporter->setMessage
      (tr("Decoding %1...").arg(QFileInfo(m_path).fileName()));
  }

  while (1) {

      UInt32 framesRead = m_d->blockSize;

      m_d->err = ExtAudioFileRead (*m_d->infile, &framesRead, &m_d->buffer);

      if (m_d->err) {
          m_error = QString("Error in CoreAudio decoding: code %1")
                                        .arg(m_d->err);
          break;
      }

      //!!! progress?

      //    std::cerr << "Read " << framesRead << " frames (block size " << m_d->blockSize << ")" << std::endl;

      // QuickTime buffers are interleaved unless specified otherwise
      addSamplesToDecodeCache(m_d->data, framesRead);

      if (framesRead < m_d->blockSize) break;
  }

  finishDecodeCache();
  endSerialised();

  /*
 TODO - close session
 m_d->err = MovieAudioExtractionEnd(m_d->extractionSessionRef);
  if (m_d->err) {
      m_error = QString("Error ending QuickTime extraction session: code %1").arg(m_d->err);
  }
   */
  m_completion = 100;



  // } else {
  //      if (m_reporter) m_reporter->setProgress(100);
  //
  //      if (m_channelCount > 0) {
  //          m_decodeThread = new DecodeThread(this);
  //          m_decodeThread->start();
  //      }
  //  }

  std::cerr << "QuickTimeFileReader::QuickTimeFileReader: frame count is now " << getFrameCount() << ", error is \"\"" << m_error.toStdString() << "\"" << std::endl;
}





CoreAudioFileReader::~CoreAudioFileReader()
{
  std::cerr << "CoreAudioFileReader::~CoreAudioFileReader" << std::endl;

  if (m_decodeThread) {
      m_cancelled = true;
      m_decodeThread->wait();
      delete m_decodeThread;
  }

  // SetMovieActive(m_d->movie, FALSE);
  // DisposeMovie(m_d->movie);

  delete[] m_d->data;
  delete m_d;
}

void
CoreAudioFileReader::cancelled()
{
  m_cancelled = true;
}

/*
void
CoreAudioFileReader::DecodeThread::run()
{
  if (m_reader->m_cacheMode == CacheInTemporaryFile) {
      m_reader->m_completion = 1;
      m_reader->startSerialised("QuickTimeFileReader::Decode");
  }

  while (1) {

      UInt32 framesRead = m_reader->m_d->blockSize;
      UInt32 extractionFlags = 0;
      m_reader->m_d->err = MovieAudioExtractionFillBuffer
          (m_reader->m_d->extractionSessionRef, &framesRead,
              &m_reader->m_d->buffer, &extractionFlags);
      if (m_reader->m_d->err) {
          m_reader->m_error = QString("Error in QuickTime decoding: code %1")
                                    .arg(m_reader->m_d->err);
          break;
      }

      // QuickTime buffers are interleaved unless specified otherwise
      m_reader->addSamplesToDecodeCache(m_reader->m_d->data, framesRead);

      if (framesRead < m_reader->m_d->blockSize) break;
  }

  m_reader->finishDecodeCache();

  m_reader->m_d->err = MovieAudioExtractionEnd(m_reader->m_d->extractionSessionRef);
  if (m_reader->m_d->err) {
      m_reader->m_error = QString("Error ending QuickTime extraction session: code %1").arg(m_reader->m_d->err);
  }

  m_reader->m_completion = 100;
  m_reader->endSerialised();
} 
*/


void
CoreAudioFileReader::getSupportedExtensions(std::set<QString> &extensions)
{
  extensions.insert("aiff");
  extensions.insert("aif");
  extensions.insert("au");
  extensions.insert("avi");
  extensions.insert("m4a");
  extensions.insert("m4b");
  extensions.insert("m4p");
  extensions.insert("m4v");
  extensions.insert("mov");
  extensions.insert("mp3");
  extensions.insert("mp4");
  extensions.insert("wav");
}

bool
CoreAudioFileReader::supportsExtension(QString extension)
{
  std::set<QString> extensions;
  getSupportedExtensions(extensions);
  return (extensions.find(extension.toLower()) != extensions.end());
}

bool
CoreAudioFileReader::supportsContentType(QString type)
{
  return (type == "audio/x-aiff" ||
      type == "audio/x-wav" ||
      type == "audio/mpeg" ||
      type == "audio/basic" ||
      type == "audio/x-aac" ||
      type == "video/mp4" ||
      type == "video/quicktime");
}

bool
CoreAudioFileReader::supports(FileSource &source)
{
  return (supportsExtension(source.getExtension()) ||
      supportsContentType(source.getContentType()));
}

#endif

