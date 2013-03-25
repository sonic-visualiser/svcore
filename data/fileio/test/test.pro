
TEMPLATE = app

LIBS += -L../../.. -lsvcore

include(../../../config.pri)

CONFIG += qt thread warn_on stl rtti exceptions console
QT += network xml testlib
QT -= gui

TARGET = svcore-test

DEPENDPATH += ../../..
INCLUDEPATH += ../../..
OBJECTS_DIR = o
MOC_DIR = o

HEADERS += AudioFileReaderTest.h \
           AudioTestData.h
SOURCES += main.cpp

win* {
PRE_TARGETDEPS += ../../../svcore.lib
}
!win* {
PRE_TARGETDEPS += ../../../libsvcore.a
}

!win32 {
    !macx* {
        QMAKE_POST_LINK=./$${TARGET}
    }
    macx* {
        QMAKE_POST_LINK=./$${TARGET}.app/Contents/MacOS/$${TARGET}
    }
}

win32:QMAKE_POST_LINK=./release/$${TARGET}.exe

