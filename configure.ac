
AC_INIT(SVcore, 1.8, cannam@all-day-breakfast.com)

AC_CONFIG_SRCDIR(base/AudioLevel.cpp)

# Autoconf will set CXXFLAGS; we don't usually want it to, because we
# either define our own flags (at least if GCC is in use) or else use
# the user's preferences.  We need to ensure CXXFLAGS is only set if
# the user has expressly set it.  So, save the user's (or empty)
# setting now and restore it after Autoconf has done its bit of
# piddling about.
USER_CXXFLAGS="$CXXFLAGS"

AC_LANG_CPLUSPLUS

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

AC_PATH_X
AC_HEADER_STDC

# These are the flags Autoconf guesses for us; we use them later if
# the user has set none and we are not using GCC (so lack our own
# preferred flags)
AUTOCONF_CXXFLAGS="$CXXFLAGS"

PKG_PROG_PKG_CONFIG

SV_CHECK_QT

AC_CHECK_LIB([bz2],[BZ2_bzReadOpen],[LIBS="$LIBS -lbz2"],[AC_MSG_ERROR(Failed to find required bzip2 library)])

PKG_CHECK_MODULES([JACK],[jack >= 0.100],[HAVES="$HAVES HAVE_LIBJACK"],[
AC_MSG_WARN(No JACK library found)])

PKG_CHECK_MODULES([libpulse],[libpulse >= 0.9],[HAVES="$HAVES HAVE_LIBPULSE"],[
AC_MSG_WARN(No PulseAudio library found)])

# bit messy this
PKG_CHECK_MODULES([fftw3],[fftw3 >= 3.0.0],[HAVES="$HAVES HAVE_FFTW3"],[])
PKG_CHECK_MODULES([fftw3f],[fftw3f >= 3.0.0],[HAVES="$HAVES HAVE_FFTW3F"],[])
PKG_CHECK_MODULES([sndfile],[sndfile >= 1.0.16],[HAVES="$HAVES HAVE_SNDFILE"],[])
PKG_CHECK_MODULES([liblo],[liblo >= 0.7],[HAVES="$HAVES HAVE_LIBLO"],[])
PKG_CHECK_MODULES([lrdf],[lrdf >= 0.2],[HAVES="$HAVES HAVE_LRDF"],[])
PKG_CHECK_MODULES([samplerate],[samplerate >= 0.1.2],[HAVES="$HAVES HAVE_SAMPLERATE"],[])
PKG_CHECK_MODULES([vamp],[vamp >= 2.1],[HAVES="$HAVES HAVE_VAMP"],[])
PKG_CHECK_MODULES([vamphostsdk],[vamp-hostsdk >= 2.1],[HAVES="$HAVES HAVE_VAMP_HOSTSDK"],[])
PKG_CHECK_MODULES([oggz],[oggz >= 1.0.0],[HAVES="$HAVES HAVE_OGGZ"],[])
PKG_CHECK_MODULES([fishsound],[fishsound >= 1.0.0],[HAVES="$HAVES HAVE_FISHSOUND"],[])
PKG_CHECK_MODULES([mad],[fishsound >= 0.15.0],[HAVES="$HAVES HAVE_MAD"],[])
PKG_CHECK_MODULES([id3tag],[id3tag >= 0.15.0],[HAVES="$HAVES HAVE_ID3TAG"],[])
PKG_CHECK_MODULES([raptor],[raptor >= 1.4.20],[HAVES="$HAVES HAVE_RAPTOR"],[])
PKG_CHECK_MODULES([rasqal],[rasqal >= 0.9.19],[HAVES="$HAVES HAVE_RASQAL"],[])
PKG_CHECK_MODULES([redland],[redland >= 1.0.10],[HAVES="$HAVES HAVE_REDLAND"],[])

AC_PATH_PROG([PERL],[perl])
if test x$PERL = x ; then
       	AC_MSG_ERROR([Failed to find required perl program.])
fi

AC_PATH_PROG([XARGS],[xargs])
if test x$XARGS = x ; then
       	AC_MSG_ERROR([Failed to find required xargs program.])
fi

AC_PATH_PROG([MAKEDEPEND],[makedepend])
if test x$MAKEDEPEND = x ; then
       	AC_MSG_ERROR([Failed to find required makedepend program.])
fi

AC_PATH_PROG([SHA1SUM],[sha1sum])
AC_PATH_PROG([SHA1SUM],[sha1 -q])
if test x$SHA1SUM = x ; then
       	AC_MSG_ERROR([Failed to find required sha1sum or sha1 program.])
fi

AC_PATH_PROG([CUT],[cut])
if test x$CUT = x ; then
       	AC_MSG_ERROR([Failed to find required cut program.])
fi

SV_DEFINES_DEBUG="-DDEBUG -DBUILD_DEBUG -DWANT_TIMING"
SV_DEFINES_RELEASE="-DNDEBUG -DBUILD_RELEASE -DNO_TIMING"
SV_DEFINES_MINIMAL="$SV_DEFINES_RELEASE"

# Now we have: USER_CXXFLAGS contains any flags the user set
# explicitly; AUTOCONF_CXXFLAGS contains flags that Autoconf thought
# we should use.  If we have GCC, we override the latter but then
# allow ourselves to be overridden (later) by the former

CXXFLAGS_DEBUG="$AUTOCONF_CXXFLAGS"
CXXFLAGS_RELEASE="$AUTOCONF_CXXFLAGS"
CXXFLAGS_MINIMAL="$AUTOCONF_CXXFLAGS"

if test "x$GCC" = "xyes"; then
        CXXFLAGS_DEBUG="-Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -g -pipe"
   	CXXFLAGS_RELEASE="-g0 -O2 -Wall -pipe"
   	CXXFLAGS_MINIMAL="-g0 -O0"
fi

CXXFLAGS_BUILD="$CXXFLAGS_RELEASE"
SV_DEFINES_BUILD="$SV_DEFINES_RELEASE"

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
CXXFLAGS_BUILD="$CXXFLAGS_DEBUG"
SV_DEFINES_BUILD="$SV_DEFINES_DEBUG"])

if test x"$USER_CXXFLAGS" != x; then
   	AC_MSG_NOTICE([The CXXFLAGS environment variable is set to "$USER_CXXFLAGS".])
	AC_MSG_NOTICE(Overriding default compiler flags with the above user setting.)
	CXXFLAGS_BUILD="$USER_CXXFLAGS"
	CXXFLAGS_MINIMAL="$USER_CXXFLAGS"
fi

CXXFLAGS="$CXXFLAGS_BUILD $SV_DEFINES_BUILD"

#if test -z "$JACK_CFLAGS" ; then
#   AC_MSG_WARN([Required JACK library not found, building with no audio!])
#fi
#if test -z "$ALSA_CFLAGS" ; then
#   AC_MSG_WARN([Required ALSA library not found, building with no MIDI!])
#fi

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXFLAGS_MINIMAL)
AC_SUBST(HAVES)
#AC_SUBST(DSSI_LOCALFLAGS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(CODENAME)

AC_OUTPUT([config.pri])

# qmake builds our static libraries, which is fine -- what we'd really
# like to do afterwards is bundle them into a single .so with e.g.

# gcc -Wl,--whole-archive */*.a -Wl,--no-whole-archive -o libsvcore.so

# but how best to arrange this? if it's not possible, we'll presumably
# just have to use a single (q)make(project)file instead.

