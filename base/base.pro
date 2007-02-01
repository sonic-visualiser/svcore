TEMPLATE = lib

SV_UNIT_PACKAGES =
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions

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
           CommandHistory.h \
           Exceptions.h \
           LogRange.h \
           Pitch.h \
           PlayParameterRepository.h \
           PlayParameters.h \
           Preferences.h \
           Profiler.h \
           PropertyContainer.h \
           RangeMapper.h \
           RealTime.h \
           RecentFiles.h \
           ResizeableBitset.h \
           RingBuffer.h \
           Scavenger.h \
           Selection.h \
           StorageAdviser.h \
           TempDirectory.h \
           Thread.h \
           UnitDatabase.h \
           Window.h \
           XmlExportable.h \
           ZoomConstraint.h
SOURCES += AudioLevel.cpp \
           Clipboard.cpp \
           Command.cpp \
           CommandHistory.cpp \
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
           Selection.cpp \
           StorageAdviser.cpp \
           TempDirectory.cpp \
           Thread.cpp \
           UnitDatabase.cpp \
           XmlExportable.cpp
