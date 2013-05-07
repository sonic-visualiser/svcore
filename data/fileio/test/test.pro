
TEMPLATE = app

LIBS += -L../../.. -L../../../debug -L../../../../dataquay -lsvcore -ldataquay

win32-g++ {
    INCLUDEPATH += ../../../../sv-dependency-builds/win32-mingw/include
    LIBS += -L../../../../sv-dependency-builds/win32-mingw/lib
}

exists(../../../config.pri) {
    include(../../../config.pri)
}

win* {
    !exists(../../../config.pri) {
        DEFINES += HAVE_BZ2 HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_SAMPLERATE HAVE_VAMP HAVE_VAMPHOSTSDK HAVE_RUBBERBAND HAVE_DATAQUAY HAVE_LIBLO HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO_2_0
        LIBS += -lbz2 -lrubberband -lvamp-hostsdk -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lportaudio -lsamplerate -llo -lz -lsord-0 -lserd-0 -lwinmm -lws2_32
    }
}

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
//PRE_TARGETDEPS += ../../../svcore.lib
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

