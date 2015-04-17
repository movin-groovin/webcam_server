
CONFIG -= qt


INCLUDEPATH += /home/yarr/Downloads/gtest/gtest-1.7.0/include
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/4.7.2/include/
INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/4.7.2/


QMAKE_CXXFLAGS += -Wno-reorder
QMAKE_CXXFLAGS += -Wno-switch
QMAKE_CXXFLAGS += -pthread
QMAKE_LFLAGS += -L /home/yarr/Downloads/gtest/gtest-1.7.0 -pthread


DEFINES += NDEBUG


LIBS += -lgtest


TARGET = utests
DESTDIR = .
OBJECTS_DIR = objects


SOURCES += sources/main.cpp \
    ../as_srv.cpp \
    ../cam.cpp \
    ../net.cpp \
    ../s_cln.cpp

HEADERS += \
    ../cam.hpp \
    ../net.hpp

