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
           ConfigFile.h \
           Exceptions.h \
           Pitch.h \
           PlayParameterRepository.h \
           PlayParameters.h \
           Preferences.h \
           Profiler.h \
           PropertyContainer.h \
           RealTime.h \
           RecentFiles.h \
           ResizeableBitset.h \
           RingBuffer.h \
           Scavenger.h \
           Selection.h \
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
           ConfigFile.cpp \
           Exceptions.cpp \
           Pitch.cpp \
           PlayParameterRepository.cpp \
           PlayParameters.cpp \
           Preferences.cpp \
           Profiler.cpp \
           PropertyContainer.cpp \
           RealTime.cpp \
           RecentFiles.cpp \
           Selection.cpp \
           TempDirectory.cpp \
           Thread.cpp \
           UnitDatabase.cpp \
           XmlExportable.cpp
