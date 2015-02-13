/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avrecorder.h"
#include "camerathread.h"

#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AvRecorder recorder;
    recorder.show();

#if defined(Q_OS_MAC)
    qDebug() << "Running on OS X";
#elif defined(Q_OS_LINUX)
    qDebug() << "Running on Linux";
    int res = system("/usr/bin/v4l2-ctl --list-devices");
    qDebug() << res;
#else
    qDebug() << "Unknown operating system";
#endif

    int only_one_camera = -1;
    if (QCoreApplication::arguments().size()>1) {
      only_one_camera = QCoreApplication::arguments().at(1).toInt();
      qDebug() << "Args:" << QCoreApplication::arguments().size() << QCoreApplication::arguments().at(1)
	       << only_one_camera;
    }
    
    QList<CameraThread *> cameras;
    for (int idx=0; idx<2; idx++) {
      if (only_one_camera>-1 && idx != only_one_camera) {
	qDebug() << "Camera" << idx << "disabled";
	continue;
      }
      
        CameraThread* cam = new CameraThread(idx);
        cam->start();
        cameras.append(cam);

        QObject::connect(&recorder, SIGNAL(outputDirectory(const QString&)),
                         cam, SLOT(setOutputDirectory(const QString&)));

        QObject::connect(&recorder, SIGNAL(stateChanged(QMediaRecorder::State)),
                         cam, SLOT(onStateChanged(QMediaRecorder::State)));

        QObject::connect(&recorder, SIGNAL(cameraOutput(QString)),
                         cam, SLOT(setCameraOutput(QString)));

        QObject::connect(&recorder, SIGNAL(cameraFramerate(QString)),
                         cam, SLOT(setCameraFramerate(QString)));

        QObject::connect(&recorder, SIGNAL(cameraStateChanged(int, int)),
                         cam, SLOT(setCameraState(int, int)));

        QObject::connect(cam, SIGNAL(cameraInfo(int,int,int)),
                         &recorder, SLOT(processCameraInfo(int, int, int)));

        QObject::connect(cam, SIGNAL(qimgReady(int, const QImage)),
                         &recorder, SLOT(processQImage(int, const QImage)));

        QObject::connect(cam, SIGNAL(errorMessage(const QString&)),
                         &recorder, SLOT(displayErrorMessage(const QString&)));
    }

    const int retval = app.exec();

    QList<CameraThread *>::iterator i;
    for (i = cameras.begin(); i != cameras.end(); ++i)
      if ((*i)->isRunning()) {
	(*i)->breakLoop();
	(*i)->quit();
        if (!(*i)->wait(2000)) {
	  (*i)->terminate();
	  if (!(*i)->wait(2000))
	    qDebug() << "CameraThread to terminate!";
	}
      }

  /*
    if (camera.isRunning()){
        camera.breakLoop();
        camera.quit();
        if (!camera.wait(5000)) {
            camera.terminate();
            if (!camera.wait(5000))
                qDebug() << "Failed to terminate!";
        }
    }
    */

    return retval;
}
