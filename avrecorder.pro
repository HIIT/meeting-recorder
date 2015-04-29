TEMPLATE = app
TARGET = avrecorder

# Choose one of the following:
CONFIG += debug
# CONFIG += release

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
CONFIG(release, debug|release):message(Release build)
CONFIG(debug, debug|release):message(Debug build)

ICON = avrecorder.icns

message(Qt version: $$[QT_VERSION] (>= 5.4 recommended))
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(Qt is installed in $$[CONFIG])

macx {
     message(Platform: Mac OS X)
     INCLUDEPATH += /opt/local/include
     LIBS += /opt/local/lib/libopencv_core.dylib
     LIBS += /opt/local/lib/libopencv_highgui.dylib
     LIBS += /opt/local/lib/libopencv_imgproc.dylib

     #LIBS += /opt/local/lib/libopencv_nonfree.dylib
     #LIBS +=	/opt/local/lib/libopencv_objdetect.dylib
     #LIBS +=	/opt/local/lib/libopencv_flann.dylib
     #LIBS +=	/opt/local/lib/libopencv_features2d.dylib
     #LIBS +=	/opt/local/lib/libopencv_calib3d.dylib
     #LIBS +=	/opt/local/lib/libopencv_video.dylib
     #LIBS +=	/opt/local/lib/libz.dylib
}

linux-g++* {
     message(Platform: Linux)
     INCLUDEPATH += /home/jmakoske/src/opencv249/include
     LIBS += /home/jmakoske/src/opencv249/lib/libopencv_core.so
     LIBS += /home/jmakoske/src/opencv249/lib/libopencv_highgui.so
     LIBS += /home/jmakoske/src/opencv249/lib/libopencv_imgproc.so

}


QT += multimedia

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

target.path = /Users/jmakoske/bin/avrecorder
INSTALLS += target

QT+=widgets
