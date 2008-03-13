TEMPLATE = lib

SV_UNIT_PACKAGES =
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions
QT -= gui

TARGET = svbase

DEPENDPATH += .
INCLUDEPATH += . ..
OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += AudioLevel.h \
           AudioPlaySource.h \
           Clipboard.h \
           Command.h \
           Exceptions.h \
           LogRange.h \
           Pitch.h \
           Playable.h \
           PlayParameterRepository.h \
           PlayParameters.h \
           Preferences.h \
           Profiler.h \
           PropertyContainer.h \
           RangeMapper.h \
           RealTime.h \
           RecentFiles.h \
           Resampler.h \
           ResizeableBitset.h \
           RingBuffer.h \
           Scavenger.h \
           Selection.h \
           Serialiser.h \
           StorageAdviser.h \
           TempDirectory.h \
           Thread.h \
           UnitDatabase.h \
           ViewManagerBase.h \
           Window.h \
           XmlExportable.h \
           ZoomConstraint.h
SOURCES += AudioLevel.cpp \
           Clipboard.cpp \
           Command.cpp \
           Exceptions.cpp \
           LogRange.cpp \
           Pitch.cpp \
           PlayParameterRepository.cpp \
           PlayParameters.cpp \
           Preferences.cpp \
           Profiler.cpp \
           PropertyContainer.cpp \
           RangeMapper.cpp \
           RealTime.cpp \
           RecentFiles.cpp \
           Resampler.cpp \
           Selection.cpp \
           Serialiser.cpp \
           StorageAdviser.cpp \
           TempDirectory.cpp \
           Thread.cpp \
           UnitDatabase.cpp \
           XmlExportable.cpp
