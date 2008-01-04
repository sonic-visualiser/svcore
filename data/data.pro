TEMPLATE = lib

SV_UNIT_PACKAGES = fftw3f sndfile mad quicktime id3tag oggz fishsound liblo
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions
QT += network

TARGET = svdata

DEPENDPATH += fft fileio model osc ..
INCLUDEPATH += . fft fileio model osc ..
OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += fft/FFTapi.h \
           fft/FFTCache.h \
           fft/FFTDataServer.h \
           fft/FFTFileCache.h \
           fft/FFTMemoryCache.h \
           fileio/AudioFileReader.h \
           fileio/AudioFileReaderFactory.h \
           fileio/BZipFileDevice.h \
           fileio/CodedAudioFileReader.h \
           fileio/CSVFileReader.h \
           fileio/CSVFileWriter.h \
           fileio/DataFileReader.h \
           fileio/DataFileReaderFactory.h \
           fileio/FileFinder.h \
           fileio/FileReadThread.h \
           fileio/FileSource.h \
           fileio/MatchFileReader.h \
           fileio/MatrixFile.h \
           fileio/MIDIEvent.h \
           fileio/MIDIFileReader.h \
           fileio/MIDIFileWriter.h \
           fileio/MP3FileReader.h \
           fileio/OggVorbisFileReader.h \
           fileio/PlaylistFileReader.h \
           fileio/ProgressPrinter.h \
           fileio/QuickTimeFileReader.h \
           fileio/ResamplingWavFileReader.h \
           fileio/WavFileReader.h \
           fileio/WavFileWriter.h \
           model/AggregateWaveModel.h \
           model/AlignmentModel.h \
           model/DenseThreeDimensionalModel.h \
           model/DenseTimeValueModel.h \
           model/EditableDenseThreeDimensionalModel.h \
           model/FFTModel.h \
           model/ImageModel.h \
           model/Labeller.h \
           model/Model.h \
           model/NoteModel.h \
           model/PowerOfSqrtTwoZoomConstraint.h \
           model/PowerOfTwoZoomConstraint.h \
           model/RangeSummarisableTimeValueModel.h \
           model/SparseModel.h \
           model/SparseOneDimensionalModel.h \
           model/SparseTimeValueModel.h \
           model/SparseValueModel.h \
           model/TextModel.h \
           model/WaveFileModel.h \
           model/WritableWaveFileModel.h \
           osc/OSCMessage.h \
           osc/OSCQueue.h 
SOURCES += fft/FFTapi.cpp \
           fft/FFTDataServer.cpp \
           fft/FFTFileCache.cpp \
           fft/FFTMemoryCache.cpp \
           fileio/AudioFileReader.cpp \
           fileio/AudioFileReaderFactory.cpp \
           fileio/BZipFileDevice.cpp \
           fileio/CodedAudioFileReader.cpp \
           fileio/CSVFileReader.cpp \
           fileio/CSVFileWriter.cpp \
           fileio/DataFileReaderFactory.cpp \
           fileio/FileFinder.cpp \
           fileio/FileReadThread.cpp \
           fileio/FileSource.cpp \
           fileio/MatchFileReader.cpp \
           fileio/MatrixFile.cpp \
           fileio/MIDIFileReader.cpp \
           fileio/MIDIFileWriter.cpp \
           fileio/MP3FileReader.cpp \
           fileio/OggVorbisFileReader.cpp \
           fileio/PlaylistFileReader.cpp \
           fileio/ProgressPrinter.cpp \
           fileio/QuickTimeFileReader.cpp \
           fileio/ResamplingWavFileReader.cpp \
           fileio/WavFileReader.cpp \
           fileio/WavFileWriter.cpp \
           model/AggregateWaveModel.cpp \
           model/AlignmentModel.cpp \
           model/DenseTimeValueModel.cpp \
           model/EditableDenseThreeDimensionalModel.cpp \
           model/FFTModel.cpp \
           model/Model.cpp \
           model/NoteModel.cpp \
           model/PowerOfSqrtTwoZoomConstraint.cpp \
           model/PowerOfTwoZoomConstraint.cpp \
           model/RangeSummarisableTimeValueModel.cpp \
           model/WaveFileModel.cpp \
           model/WritableWaveFileModel.cpp \
           osc/OSCMessage.cpp \
           osc/OSCQueue.cpp 
