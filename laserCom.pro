QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/Protocol/ProtocolDispatch.cpp \
    src/at/at.cpp \
    src/transferFile/recvFile.cpp \
    src/transferFile/sendFile.cpp

HEADERS += \
    mainwindow.h \
    src/Protocol/ProtocolDispatch.h \
    src/Protocol/protocol.h \
    src/at/at.h \
    src/common.h \
    src/transferFile/recvFile.h \
    src/transferFile/sendFile.h

INCLUDEPATH += ./src/
INCLUDEPATH += ./src/at/
INCLUDEPATH += ./src/transferFile/
INCLUDEPATH += ./src/Protocol/

VERSION = 0.26
DEFINES += SOFT_VERSION=\"\\\"$$VERSION\\\"\"

TARGET = laserCom$$VERSION

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DEFINES += LOG2FILE

RESOURCES += \
    res/res.qrc

#OTHER_FILES += app.rc
RC_FILE += app.rc

#DESTDIR = $$absolute_path($${_PRO_FILE_PWD_}/bin/)

include($$PWD/changelog.pri)
#include($$PWD/deploy.pri)
