#-------------------------------------------------
#
# Project created by QtCreator 2016-06-14T15:23:48
#
#-------------------------------------------------

QT       -= gui
QT       += core

TARGET = QEpigDataFourierAnalysisLib
TEMPLATE = lib

DEFINES += QEPIGDATAFOURIERANALYSISLIB_LIBRARY

SOURCES += qepigdatafourieranalysislib.cpp \
        cprocessrawepigraphdata.cpp

HEADERS += qepigdatafourieranalysislib.h\
        qepigdatafourieranalysislib_global.h \
        cprocessrawepigraphdata.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
