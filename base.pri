# Common build instructions to all libraries, plug-ins and programs.
# Insert this in your project-specific pro file:
#
# INTODIR = into_root_dir_relative_to_your_project_dir
# include($$INTODIR/base.pri)
#
# Use "qmake MODE=debug" or "qmake MODE=release" to switch between debug/release builds.

INTO_LIB_VERSION = 2.0.0
win32:INTO_LIBV=2

# If MODE is not explicitly set, inspect CONFIG
isEmpty(MODE) {
  debug {
    MODE = debug
  } else {
    MODE = release
  }
}

MODE = $$lower($$MODE)
equals(MODE,debug) {
  CONFIG -= release
} else:equals(MODE,release) {
  CONFIG -= debug
} else {
  error(Build mode must be either debug or release.)
}

CONFIG *= $$MODE

isEmpty(INTODIR) { INTODIR = $$PWD }

DESTDIR = $$MODE
OBJECTS_DIR = $$MODE
MOC_DIR = $$MODE

CONFIG += thread warn_on exceptions
qt {
  QT += network
} else {
  QT =
  INCLUDEPATH += $$INTODIR/core/stdwrapper
  DEFINES += PII_NO_QT
}

INCLUDEPATH += . \
  $$INTODIR/core \
  $$INTODIR/core/serialization \
  $$INTODIR/core/matrix \
  $$INTODIR/core/network \
  $$INTODIR/ydin \
  $$INTODIR/ydin/network \
  $$INTODIR/gui
LIBS += -L$$INTODIR/core/$$MODE -L$$INTODIR/ydin/$$MODE -L$$INTODIR/gui/$$MODE

win32-msvc2008 {
# Disable checked iterators for VC2008 because they cause compilation errors.
  DEFINES += _SECURE_SCL=0 _CRT_SECURE_NO_WARNINGS
}

# disable warnings about throw specifications and deprecated POSIX names with VC compilers
win32-msvc* {
  QMAKE_CXXFLAGS_DEBUG += /wd4290 /wd4996
  QMAKE_CXXFLAGS_RELEASE += /wd4290 /wd4996
  QMAKE_LFLAGS += /OPT:NOREF
}

win32-g++ {
  QMAKE_LFLAGS += -Wl,--export-all-symbols -Wl,--enable-auto-import
}

# HACK: we don't want rpath in shared libs!
# Change the -Wl,-rpath, commandline option to a harmless -L
unix {
  eval(QT_MAJOR_VERSION == 4 && QT_MINOR_VERSION > 5) {
    QMAKE_LFLAGS_RPATH = -L
  } else {
    QMAKE_RPATH = -L
  }
}
linux: QMAKE_CXXFLAGS += -fvisibility=hidden
macx*: CONFIG += x86
linux-g++: QMAKE_CXXFLAGS += -Wno-psabi

equals(MODE, release) {
  unix:CONFIG += separate_debug_info
  DEFINES += _RELEASE QT_NO_DEBUG
} else {
  DEFINES += _DEBUG
}

defineReplace(forcedLink) {
  linux:return("-Wl,--whole-archive" -l$$ARGS "-Wl,--no-whole-archive")
  !linux:return(-l$$ARGS)
}

PRO_FILE_BASENAME = $$basename(_PRO_FILE_)
PRO_FILE_BASENAME = $$replace(PRO_FILE_BASENAME, .pro, "")

include(qt5.pri)
include(c++11.pri)
include(extensions.pri)
