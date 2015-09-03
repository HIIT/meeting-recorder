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

#ifndef AVRECORDER_H
#define AVRECORDER_H

#include <QMainWindow>
#include <QMediaRecorder>
#include <QUrl>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class AvRecorder; }
class QAudioRecorder;
class QAudioProbe;
class QAudioBuffer;
QT_END_NAMESPACE

class QAudioLevel;

class AvRecorder : public QMainWindow
{
    Q_OBJECT

public:
    AvRecorder(QWidget *parent = 0);
    ~AvRecorder();

signals:
    void outputDirectory(const QString&);
    void stateChanged(QMediaRecorder::State);
    void cameraOutput(QString);
    void cameraFramerate(QString);
    void cameraPowerChanged(int, int);

public slots:
    void processBuffer(const QAudioBuffer&);
    void processQImage(int n, const QImage qimg);
    void processCameraInfo(int, int, int);
    void disableCameraCheckbox(int n);
    void displayErrorMessage(const QString&);
    void uncheckEvent1();
    void uncheckEvent2();
    void uncheckEvent3();
    void uncheckEvent4();

private slots:
    void setOutputLocation();
    void upload();
    void togglePause();
    void toggleRecord();

    void setStatusTo1();
    void setStatusTo2();
    void setStatusTo3();
    void setStatusTo4();
    void setStatusTo5();
    void setStatusTo6();

    void setPoseTo1();
    void setPoseTo2();

    void event1();
    void event2();
    void event3();
    void event4();

    void setCameraOutput(QString);
    void setCameraFramerate(QString);
    void setCamera0State(int);
    void setCamera1State(int);

    void updateStatus(QMediaRecorder::Status);
    void onStateChanged(QMediaRecorder::State);
    void updateProgress(qint64 pos);
    void displayErrorMessage();

private:
    void clearAudioLevels();
    void setStatus(int, bool=true);
    void setPose(int, bool=true);
    void handleEvent(int);
    void writeAnnotation(int, const QString &);

    Ui::AvRecorder *ui;

    QAudioRecorder *audioRecorder;
    QAudioProbe *probe;
    QList<QAudioLevel*> audioLevels;
    bool outputLocationSet;

    QString defaultDir;
    QString dirName;

    QDateTime rec_started;

};

#endif // AVRECORDER_H
