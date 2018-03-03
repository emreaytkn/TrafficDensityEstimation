#-------------------------------------------------
#
# Project created by QtCreator 2018-02-21T15:51:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = bin/vehicletracker
TEMPLATE = app
CONFIG += c++14
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEPENDPATH +=   .                               \
                ./src                           \
                ./src/gui                       \
                ./src/core                      \
                ./src/util                      \
                ./src/comm                      \
                ./ui

INCLUDEPATH +=  ./src                           \
                ./src/gui                       \
                ./src/core                      \
                ./src/util                      \
                ./ui

INCLUDEPATH +=	$$(OPENCV_ROOT)/include

LIBS +=	-L$$(OPENCV_ROOT)/lib/		\
        -lopencv_highgui            	\
        -lopencv_core               	\
        -lopencv_imgcodecs          	\
        -lopencv_imgproc                \
        -lopencv_videoio                \
        -lopencv_video                  \
        -lopencv_tracking               \
        -lopencv_cudaarithm             \
        -lopencv_cudaoptflow            \
        -lopencv_cudaimgproc

SOURCES += \
        ./src/main.cpp \
        ./src/gui/mainwindow.cpp \
    ./src/core/Ctracker.cpp \
    ./src/core/Tracker/track.cpp \
    ./src/core/Tracker/LocalTracker.cpp \
    ./src/core/Tracker/Kalman.cpp \
    ./src/core/Tracker/HungarianAlg/HungarianAlg.cpp \
    src/util/qcustomplot.cpp \
    src/gui/dialog.cpp \
    src/core/player.cpp

HEADERS += \
        ./src/gui/mainwindow.h \
    ./src/core/Ctracker.h \
    ./src/core/Tracker/track.h \
    ./src/core/Tracker/LocalTracker.h \
    ./src/core/Tracker/defines.h \
    ./src/core/Tracker/Kalman.h \
    ./src/core/Tracker/HungarianAlg/HungarianAlg.h \
    src/util/qcustomplot.h \
    src/util/csvhandler.h \
    src/gui/dialog.h \
    src/core/player.h

FORMS += \
        ./ui/mainwindow.ui \
    ./ui/dialog.ui

MOC_DIR 	= .moc
UI_DIR 		= .ui
OBJECTS_DIR 	= .obj
