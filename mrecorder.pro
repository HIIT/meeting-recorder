TEMPLATE = app
TARGET = mrecorder

# Choose one of the following:
CONFIG += debug
# CONFIG += release

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG(release, debug|release):message(Release build)
CONFIG(debug, debug|release):message(Debug build)

ICON = mrecorder.icns

message(Qt version: $$[QT_VERSION] (>= 5.4 recommended))
message(Qt is installed in $$[QT_INSTALL_PREFIX])

macx {
     message(Platform: Mac OS X)
     OPENCVDIR = /opt/local

     INCLUDEPATH += $$OPENCVDIR/include
     LIBS += $$OPENCVDIR/lib/libopencv_core.dylib
     LIBS += $$OPENCVDIR/lib/libopencv_highgui.dylib
     LIBS += $$OPENCVDIR/lib/libopencv_imgproc.dylib
}

linux-g++* {
     message(Platform: Linux)
     OPENCVDIR = /home/jmakoske/src/opencv249

     INCLUDEPATH += $$OPENCVDIR/include
     LIBS += $$OPENCVDIR/lib/libopencv_core.so
     LIBS += $$OPENCVDIR/lib/libopencv_highgui.so
     LIBS += $$OPENCVDIR/lib/libopencv_imgproc.so
}


QT += multimedia
QT += widgets

HEADERS = \
    avrecorder.h \
    qaudiolevel.h \
    camerathread.h

SOURCES = \
    main.cpp \
    avrecorder.cpp \
    qaudiolevel.cpp \
    camerathread.cpp

FORMS += avrecorder.ui

#target.path = /Users/jmakoske/bin/mrecorder
#INSTALLS += target


