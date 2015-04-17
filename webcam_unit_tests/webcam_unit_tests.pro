
CONFIG -= qt


INCLUDEPATH += /home/yarr/Downloads/gtest/gtest-1.7.0/include
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/4.7.2/include/
INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/4.7.2/
INCLUDEPATH += /usr/local/include/boost-1_57
INCLUDEPATH += /usr/include/opencv2
INCLUDEPATH += -pedantic -std=c++11 -Wno-reorder -Wno-switch -O2 -Wall


QMAKE_CXXFLAGS += -Wno-reorder -std=c++11
QMAKE_CXXFLAGS += -Wno-switch
QMAKE_CXXFLAGS += -pthread
QMAKE_LFLAGS += -L /home/yarr/Downloads/gtest/gtest-1.7.0 -pthread
QMAKE_LFLAGS += -L/usr/local/lib/boost1_57/shared
QMAKE_LFLAGS += -lboost_system-gcc47-mt-1_57 -lboost_chrono-gcc47-mt-1_57 -lboost_thread-gcc47-mt-1_57
QMAKE_LFLAGS += -lboost_regex-gcc47-mt-1_57 -lopencv_highgui -lopencv_core -lopencv_imgproc


#DEFINES += NDEBUG


LIBS += -lgtest


TARGET = utests
DESTDIR = .
OBJECTS_DIR = objects


SOURCES += \
    ../as_srv.cpp \
    ../cam.cpp \
    ../net.cpp \
    ../s_cln.cpp \
    sources/CConfig_tests.cpp \
    sources/Main_tests.cpp

HEADERS += \
    ../cam.hpp \
    ../net.hpp \
    sources/CConfig_tests.h

