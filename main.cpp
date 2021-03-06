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

/*
  Copyright (c) 2015 University of Helsinki

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "avrecorder.h"
#include "camerathread.h"

#include <QtWidgets>
#include <QTextStream>

// ---------------------------------------------------------------------

void help(const QString& cmd) {
  QTextStream cout(stdout);
  cout << "USAGE: " << cmd << " [0:videosize] [1:videosize]" << endl
       << endl
       << "  supported videosizes: WxH, fullhd, 1080p, hd, 720p" << endl
       << "  (currently only for Linux; OS X uses max camera resolution)"
       << endl
       << endl;
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("HIIT");
    QCoreApplication::setOrganizationDomain("hiit.fi");
    QCoreApplication::setApplicationName("mrecorder");

    QApplication app(argc, argv);

    AvRecorder recorder;
    recorder.show();

#if defined(Q_OS_MAC)
    qDebug() << "Running on OS X";
#elif defined(Q_OS_LINUX)
    qDebug() << "Running on Linux";
    qDebug() << "Querying for v4l2 devices:";
    int res = system("/usr/bin/v4l2-ctl --list-devices");
    if (res)
      qWarning() << "WARNING: /usr/bin/v4l2-ctl not found or failed. "
		 << "This may cause problems";
#elif defined(Q_OS_WIN)
    qDebug() << "Running on Windows";
#else
    qWarning() << "Unknown operating system";
#endif

    QStringList args = QCoreApplication::arguments();
    QBitArray use_cameras(2);
    QMap<int, QString> wxhs;
    QMap<QString, QString> resolutions;
    resolutions.insert("fullhd", "1920x1080");
    resolutions.insert("1080p", "1920x1080");
    resolutions.insert("hd", "1280x720");
    resolutions.insert("720p", "1280x720");

    if (args.size()>1)
	use_cameras.fill(false);
    else {
	use_cameras.fill(true);
	QStringList items;
	items << "Both webcams" << "First webcam only" << "Second webcam only"
	      << "None";
	bool ok;
	QString item =
	    QInputDialog::getItem(&recorder, "Re:Know Meeting recorder",
				  "Please select the webcams to open",
				  items, 1, false, &ok);
	if (ok) {
	    int idx = items.indexOf(item);
	    if (idx == 1)
		use_cameras[1] = false;
	    if (idx == 2)
		use_cameras[0] = false;
	    if (idx == 3)
		use_cameras[0] = use_cameras[1] = false;
	}
    }

    for (int i = 1; i < args.size(); ++i) {
      bool ok = true;
      int c = -1;

      if (args.at(i) == "-h" || args.at(i) == "--help") {
	help(args.at(0));
	return 0;
      } else if (args.at(i).contains(':')) {
	QStringList parts = args.at(i).split(':');
	c = parts.at(0).toInt(&ok);
	if (ok)
	  wxhs.insert(c, parts.at(1));
      } else {
	c = args.at(i).toInt(&ok);
      }

      if (ok)
	use_cameras[c] = true;
      else
	qWarning() << "WARNING: Failed to parse" << args.at(i);
    }

    qDebug() << args.size()
	     << use_cameras.at(0)
	     << use_cameras.at(1);
    
    QList<CameraThread *> cameras;
    for (int idx=0; idx<2; idx++) {
      if (!use_cameras.at(idx)) {
	qDebug() << "Camera" << idx << "disabled";
	recorder.disableCameraCheckbox(idx);
	continue;
      }
      
      CameraThread* cam;
      if (wxhs.contains(idx)) {
	QString val = wxhs.value(idx);
	if (resolutions.contains(val))
	  cam = new CameraThread(idx, resolutions.value(val));
	else
	  cam = new CameraThread(idx, val);
      } else {
	cam = new CameraThread(idx);
      }

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

        QObject::connect(&recorder, SIGNAL(cameraPowerChanged(int, int)),
                         cam, SLOT(setCameraPower(int, int)));

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
	    qDebug() << "CameraThread failed to terminate!";
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

// ---------------------------------------------------------------------

// Local Variables:
// c-basic-offset: 4
// End:
