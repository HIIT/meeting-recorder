TEMPLATE = app
TARGET = avrecorder

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

QT += multimedia

win32:INCLUDEPATH += $$PWD

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
