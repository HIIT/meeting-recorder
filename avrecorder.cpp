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

#include <QAudioProbe>
#include <QAudioRecorder>
#include <QDir>
#include <QFileDialog>
#include <QMediaRecorder>
#include <QHostInfo>
#include <QMessageBox>
#include <QShortcut>
#include <QTimer>
#include <QDebug>

#include "avrecorder.h"
#include "qaudiolevel.h"

#include "ui_avrecorder.h"

#if !defined(Q_OS_WIN)
#include "uploadwidget.h"
#endif

// ---------------------------------------------------------------------

static qreal getPeakValue(const QAudioFormat &format);
static QVector<qreal> getBufferLevels(const QAudioBuffer &buffer);

template <class T>
static QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels);

// ---------------------------------------------------------------------

AvRecorder::AvRecorder(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AvRecorder),
    outputLocationSet(false)
{
    ui->setupUi(this);
    resize(0,0);

    audioRecorder = new QAudioRecorder(this);
    probe = new QAudioProbe;
    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
            this, SLOT(processBuffer(QAudioBuffer)));
    probe->setSource(audioRecorder);

    //audio devices
    ui->audioDeviceBox->addItem(tr("Default"), QVariant(QString()));
    foreach (const QString &device, audioRecorder->audioInputs()) {
        ui->audioDeviceBox->addItem(device, QVariant(device));
    }

    //audio codecs
    ui->audioCodecBox->addItem(tr("Default"), QVariant(QString()));
    foreach (const QString &codecName, audioRecorder->supportedAudioCodecs()) {
        ui->audioCodecBox->addItem(codecName, QVariant(codecName));
    }

    //containers
    ui->containerBox->addItem(tr("Default"), QVariant(QString()));
    foreach (const QString &containerName, audioRecorder->supportedContainers()) {
        ui->containerBox->addItem(containerName, QVariant(containerName));
    }

    //sample rate
    ui->sampleRateBox->addItem(tr("Default"), QVariant(0));
    foreach (int sampleRate, audioRecorder->supportedAudioSampleRates()) {
        ui->sampleRateBox->addItem(QString::number(sampleRate), QVariant(
                sampleRate));
    }

    //channels
    ui->channelsBox->addItem(tr("Default"), QVariant(-1));
    ui->channelsBox->addItem(QStringLiteral("1"), QVariant(1));
    ui->channelsBox->addItem(QStringLiteral("2"), QVariant(2));
    ui->channelsBox->addItem(QStringLiteral("4"), QVariant(4));

    //quality
    ui->qualitySlider->setRange(0, int(QMultimedia::VeryHighQuality));
    ui->qualitySlider->setValue(int(QMultimedia::NormalQuality));

    //bitrates:
    ui->bitrateBox->addItem(tr("Default"), QVariant(0));
    ui->bitrateBox->addItem(QStringLiteral("32000"), QVariant(32000));
    ui->bitrateBox->addItem(QStringLiteral("64000"), QVariant(64000));
    ui->bitrateBox->addItem(QStringLiteral("96000"), QVariant(96000));
    ui->bitrateBox->addItem(QStringLiteral("128000"), QVariant(128000));

    //camera output sizes:
    ui->cameraOutBox->addItem("Original");
    ui->cameraOutBox->addItem("960x540");
    ui->cameraOutBox->addItem("768x432");
    ui->cameraOutBox->addItem("640x360");
    ui->cameraOutBox->addItem("480x270");
    ui->cameraOutBox->setCurrentIndex(3); // Needs to match CameraThread::output_size

    //camera framerates:
    ui->frameRateBox->addItem("30");
    ui->frameRateBox->addItem("25");
    ui->frameRateBox->addItem("15");
    ui->frameRateBox->addItem("10");
    ui->frameRateBox->addItem("5");
    ui->frameRateBox->setCurrentIndex(1);  // Needs to match CameraThread::framerate

    connect(audioRecorder, SIGNAL(durationChanged(qint64)), this,
            SLOT(updateProgress(qint64)));
    connect(audioRecorder, SIGNAL(statusChanged(QMediaRecorder::Status)), this,
            SLOT(updateStatus(QMediaRecorder::Status)));
    connect(audioRecorder, SIGNAL(stateChanged(QMediaRecorder::State)),
            this, SLOT(onStateChanged(QMediaRecorder::State)));
    connect(audioRecorder, SIGNAL(error(QMediaRecorder::Error)), this,
            SLOT(displayErrorMessage()));

    defaultDir = QDir::homePath() + "/Meetings";
    dirName = ".";

    setStatus(1, false);
    setPose(1, false);

    /*
    QShortcut *anno1 = new QShortcut(QKeySequence(QKeySequence::MoveToPreviousChar),
				     this);
    QShortcut *anno2 = new QShortcut(QKeySequence(QKeySequence::MoveToNextLine),
				     this);
    QShortcut *anno3 = new QShortcut(QKeySequence(QKeySequence::MoveToNextChar),
				     this);
    connect(anno1, SIGNAL(activated()), this, SLOT(setAnnotationTo1()));
    connect(anno2, SIGNAL(activated()), this, SLOT(setAnnotationTo2()));
    connect(anno3, SIGNAL(activated()), this, SLOT(setAnnotationTo3()));
    */

}

// ---------------------------------------------------------------------

AvRecorder::~AvRecorder()
{
    delete audioRecorder;
    delete probe;
}

// ---------------------------------------------------------------------

void AvRecorder::updateProgress(qint64 duration)
{
    if (audioRecorder->error() != QMediaRecorder::NoError || duration < 2000)
        return;

    QFileInfo wavFile(dirName+"/audio.wav");
    QFileInfo ca1File(dirName+"/capture0.avi");
    QFileInfo ca2File(dirName+"/capture1.avi");

    ui->statusbar->showMessage(tr("Rec started %1 (%2 secs), audio %3 MB, camera 0: %4 MB, camera 1: %5 MB")
                               .arg(rec_started.toString("hh:mm:ss"))
                               .arg(duration / 1000)
                               .arg(wavFile.size()/1024/1024)
                               .arg(ca1File.size()/1024/1024)
                               .arg(ca2File.size()/1024/1024));
}

// ---------------------------------------------------------------------

void AvRecorder::updateStatus(QMediaRecorder::Status status)
{
    QString statusMessage;

    switch (status) {
    case QMediaRecorder::RecordingStatus:
        statusMessage = tr("Starting to record...");
        break;
    case QMediaRecorder::PausedStatus:
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaRecorder::UnloadedStatus:
    case QMediaRecorder::LoadedStatus:
        clearAudioLevels();
        statusMessage = tr("Recording stopped");
    default:
        break;
    }

    if (audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

// ---------------------------------------------------------------------

void AvRecorder::onStateChanged(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        //ui->pauseButton->setText(tr("Pause"));
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        //ui->pauseButton->setText(tr("Resume"));
        break;
    case QMediaRecorder::StoppedState:
        ui->recordButton->setText(tr("Record"));
        //ui->pauseButton->setText(tr("Pause"));
        break;
    }

    //ui->pauseButton->setEnabled(audioRecorder->state() != QMediaRecorder::StoppedState);

    emit stateChanged(state);
}

// ---------------------------------------------------------------------

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

// ---------------------------------------------------------------------

void AvRecorder::toggleRecord()
{
    if (!outputLocationSet) {
	QMessageBox msgBox;
	msgBox.setWindowTitle("Re:Know Meeting recorder");
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText("Create target directory first");
	msgBox.setInformativeText("Before recording, you need to create a directory "
				  "to store the media files.");
	msgBox.exec();
        setOutputLocation();
    }

    if (!outputLocationSet)
        return;

    if (audioRecorder->state() == QMediaRecorder::StoppedState) {
        audioRecorder->setAudioInput(boxValue(ui->audioDeviceBox).toString());

        QAudioEncoderSettings settings;
        settings.setCodec(boxValue(ui->audioCodecBox).toString());
        settings.setSampleRate(boxValue(ui->sampleRateBox).toInt());
        settings.setBitRate(boxValue(ui->bitrateBox).toInt());
        settings.setChannelCount(boxValue(ui->channelsBox).toInt());
        settings.setQuality(QMultimedia::EncodingQuality(ui->qualitySlider->value()));
        settings.setEncodingMode(ui->constantQualityRadioButton->isChecked() ?
                                 QMultimedia::ConstantQualityEncoding :
                                 QMultimedia::ConstantBitRateEncoding);

        QString container = boxValue(ui->containerBox).toString();

        audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
        audioRecorder->record();

        rec_started = QDateTime::currentDateTime();

        QFile timefile(dirName+"/starttime.txt");
        if (timefile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&timefile);
            out << rec_started.toString("yyyy-MM-dd'T'hh:mm:sst") << "\n";
            timefile.close();
        }
        QFile hostfile(dirName+"/hostname.txt");
        if (hostfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&hostfile);
            out << QHostInfo::localHostName() << "\n";
            hostfile.close();
        }
    }
    else {
        audioRecorder->stop();
    }
}

// ---------------------------------------------------------------------

void AvRecorder::togglePause()
{
    if (audioRecorder->state() != QMediaRecorder::PausedState)
        audioRecorder->pause();
    else
        audioRecorder->record();
}

// ---------------------------------------------------------------------

void AvRecorder::upload()
{
    if (!outputLocationSet)
        setOutputLocation();

    if (!outputLocationSet)
        return;

    QFileInfo wavFile(dirName+"/audio.wav");
    QFileInfo ca1File(dirName+"/capture0.avi");
    QFileInfo ca2File(dirName+"/capture1.avi");
    int totalsize = (wavFile.size()+ca1File.size()+ca2File.size())/1024/1024;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Re:Know Meeting recorder");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);

#if defined(Q_OS_WIN)

    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Upload not supported on Windows.");
    msgBox.setInformativeText("Data upload is currently not supported on Windows. "
                              "Please use some alternative way to send the data.");
    msgBox.exec();

#else
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("About to upload meeting data from %1 (%2 MB).")
		   .arg(dirName).arg(totalsize));
    msgBox.setInformativeText("Do you want to start the upload process?");

    int ret = msgBox.exec();
    if (ret == QMessageBox::Ok) {
      UploadWidget uw(this, dirName);
      uw.exec();
    }
#endif

}

// ---------------------------------------------------------------------

void AvRecorder::setOutputLocation()
{
    QDir dir(defaultDir);
    if (!dir.exists()) {
	qDebug() << "Creating directory" << defaultDir;
	    if (!dir.mkpath(".")) {
		qWarning() << "WARNING: Failed to create directory" << defaultDir;
		defaultDir = QDir::homePath();
	    }
    }

    dirName = QFileDialog::getExistingDirectory(this, "Select or create a meeting",
						defaultDir,
                                                QFileDialog::ShowDirsOnly);
     if (!dirName.isNull() && !dirName.isEmpty()) {
	ui->statusbar->showMessage("Output directory: "+dirName);
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(dirName+"/audio.wav"));
	emit outputDirectory(dirName);
	//audioRecorder->setOutputLocation(QUrl::fromLocalFile(fileName));
	outputLocationSet = true;
    } else
	outputLocationSet = false;

}

// ---------------------------------------------------------------------

void AvRecorder::displayErrorMessage() {
    ui->statusbar->showMessage(audioRecorder->errorString());
}

void AvRecorder::displayErrorMessage(const QString &e) {
    ui->statusbar->showMessage(e);
}

// ---------------------------------------------------------------------

void AvRecorder::clearAudioLevels()
{
    for (int i = 0; i < audioLevels.size(); ++i)
        audioLevels.at(i)->setLevel(0);
}

// ---------------------------------------------------------------------

// This function returns the maximum possible sample value for a given audio format
qreal getPeakValue(const QAudioFormat& format)
{
    // Note: Only the most common sample formats are supported
    if (!format.isValid())
        return qreal(0);

    if (format.codec() != "audio/pcm")
        return qreal(0);

    switch (format.sampleType()) {
    case QAudioFormat::Unknown:
        break;
    case QAudioFormat::Float:
        if (format.sampleSize() != 32) // other sample formats are not supported
            return qreal(0);
        return qreal(1.00003);
    case QAudioFormat::SignedInt:
        if (format.sampleSize() == 32)
            return qreal(INT_MAX);
        if (format.sampleSize() == 16)
            return qreal(SHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(CHAR_MAX);
        break;
    case QAudioFormat::UnSignedInt:
        if (format.sampleSize() == 32)
            return qreal(UINT_MAX);
        if (format.sampleSize() == 16)
            return qreal(USHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(UCHAR_MAX);
        break;
    }

    return qreal(0);
}

// ---------------------------------------------------------------------

// returns the audio level for each channel
QVector<qreal> getBufferLevels(const QAudioBuffer& buffer)
{
    QVector<qreal> values;

    if (!buffer.format().isValid() || buffer.format().byteOrder() != QAudioFormat::LittleEndian)
        return values;

    if (buffer.format().codec() != "audio/pcm")
        return values;

    int channelCount = buffer.format().channelCount();
    values.fill(0, channelCount);
    qreal peak_value = getPeakValue(buffer.format());
    if (qFuzzyCompare(peak_value, qreal(0)))
        return values;

    switch (buffer.format().sampleType()) {
    case QAudioFormat::Unknown:
    case QAudioFormat::UnSignedInt:
        if (buffer.format().sampleSize() == 32)
            values = getBufferLevels(buffer.constData<quint32>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 16)
            values = getBufferLevels(buffer.constData<quint16>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 8)
            values = getBufferLevels(buffer.constData<quint8>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] = qAbs(values.at(i) - peak_value / 2) / (peak_value / 2);
        break;
    case QAudioFormat::Float:
        if (buffer.format().sampleSize() == 32) {
            values = getBufferLevels(buffer.constData<float>(), buffer.frameCount(), channelCount);
            for (int i = 0; i < values.size(); ++i)
                values[i] /= peak_value;
        }
        break;
    case QAudioFormat::SignedInt:
        if (buffer.format().sampleSize() == 32)
            values = getBufferLevels(buffer.constData<qint32>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 16)
            values = getBufferLevels(buffer.constData<qint16>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 8)
            values = getBufferLevels(buffer.constData<qint8>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] /= peak_value;
        break;
    }

    return values;
}

// ---------------------------------------------------------------------

template <class T>
QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels)
{
    QVector<qreal> max_values;
    max_values.fill(0, channels);

    for (int i = 0; i < frames; ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(qreal(buffer[i * channels + j]));
            if (value > max_values.at(j))
                max_values.replace(j, value);
        }
    }

    return max_values;
}

// ---------------------------------------------------------------------

void AvRecorder::processBuffer(const QAudioBuffer& buffer)
{
    if (audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(audioLevels);
        audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            QAudioLevel *level = new QAudioLevel(ui->centralwidget);
            audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }

    QVector<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        audioLevels.at(i)->setLevel(levels.at(i));
}

// ---------------------------------------------------------------------

void AvRecorder::processQImage(int n, const QImage qimg) {
    //qDebug() << "processQImage(): n=" << n;
    if (n==0) {
        ui->viewfinder_0->setPixmap(QPixmap::fromImage(qimg));
        ui->viewfinder_0->show();
    } else if (n==1) {
        ui->viewfinder_1->setPixmap(QPixmap::fromImage(qimg));
        ui->viewfinder_1->show();
    }
}

// ---------------------------------------------------------------------

void AvRecorder::processCameraInfo(int n, int w, int h) {
    if (n==0) {
        ui->camera_label_0->setText(QString("Camera 0: %1x%2").arg(w).arg(h));
        ui->camera_label_0->setChecked(true);
    } else if (n==1) {
        ui->camera_label_1->setText(QString("Camera 1: %1x%2").arg(w).arg(h));
        ui->camera_label_1->setChecked(true);
    }
}

// ---------------------------------------------------------------------

void AvRecorder::disableCameraCheckbox(int n) {
    if (n==0) {
        ui->camera_label_0->setEnabled(false);
    } else if (n==1) {
        ui->camera_label_1->setEnabled(false);
    }
}

// ---------------------------------------------------------------------

void AvRecorder::setCameraOutput(QString wxh) {
    //qDebug() << "setCameraOutput(): idx=" << idx;
    emit cameraOutput(wxh);
}

// ---------------------------------------------------------------------

void AvRecorder::setCameraFramerate(QString fps) {
    emit cameraFramerate(fps);
}

// ---------------------------------------------------------------------

void AvRecorder::setCamera0State(int state) {
    qDebug() << "setCamera0State(): state=" << state;
    emit cameraPowerChanged(0, state);
}

void AvRecorder::setCamera1State(int state) {
    qDebug() << "setCamera0State(): state=" << state;
    emit cameraPowerChanged(1, state);
}

// ---------------------------------------------------------------------

void AvRecorder::writeAnnotation(int anno, const QString &fn) {
    if (!outputLocationSet) {
	QMessageBox msgBox;
	msgBox.setWindowTitle("Re:Know Meeting recorder");
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText("Create target directory first");
	msgBox.setInformativeText("Before annotation, you need to create a directory "
				  "to store the annotations.");
	msgBox.exec();
        setOutputLocation();
    }

    QDateTime now = QDateTime::currentDateTime();
    QFile annofile(dirName+"/"+fn);
    if (annofile.open(QIODevice::WriteOnly | QIODevice::Append |
		      QIODevice::Text)) {
	QTextStream out(&annofile);
	out << anno << " " << now.toString("yyyy-MM-dd'T'hh:mm:sst")
	    << "\n";
	annofile.close();
    }
}

// ---------------------------------------------------------------------

void AvRecorder::setStatusTo1() { setStatus(1); }
void AvRecorder::setStatusTo2() { setStatus(2); }
void AvRecorder::setStatusTo3() { setStatus(3); }
void AvRecorder::setStatusTo4() { setStatus(4); }
void AvRecorder::setStatusTo5() { setStatus(5); }
void AvRecorder::setStatusTo6() { setStatus(6); }

void AvRecorder::setStatus(int status, bool write) {
    qDebug() << "Status set to:" << status;
    ui->statusButton_1->setChecked(status==1);
    ui->statusButton_2->setChecked(status==2);
    ui->statusButton_3->setChecked(status==3);
    ui->statusButton_4->setChecked(status==4);
    ui->statusButton_5->setChecked(status==5);
    ui->statusButton_6->setChecked(status==6);

    if (write)
	writeAnnotation(status, "status.txt");
}

// ---------------------------------------------------------------------

void AvRecorder::setPoseTo1() { setPose(1); }
void AvRecorder::setPoseTo2() { setPose(2); }

void AvRecorder::setPose(int pose, bool write) {
    qDebug() << "Pose set to:" << pose;
    ui->poseButton_1->setChecked(pose==1);
    ui->poseButton_2->setChecked(pose==2);

    if (write)
	writeAnnotation(pose, "pose.txt");
}

// ---------------------------------------------------------------------

void AvRecorder::event1() { handleEvent(1); }
void AvRecorder::event2() { handleEvent(2); }
void AvRecorder::event3() { handleEvent(3); }
void AvRecorder::event4() { handleEvent(4); }

void AvRecorder::handleEvent(int ev) {
    qDebug() << "Event registered:" << ev;
    switch (ev) {
    case 1:
	ui->eventButton_1->setChecked(true);
	QTimer::singleShot(2000, this, SLOT(uncheckEvent1()));
	break;
    case 2:
	ui->eventButton_2->setChecked(true);
	QTimer::singleShot(2000, this, SLOT(uncheckEvent2()));
	break;
    case 3:
	ui->eventButton_3->setChecked(true);
	QTimer::singleShot(2000, this, SLOT(uncheckEvent3()));
	break;
    case 4:
	ui->eventButton_4->setChecked(true);
	QTimer::singleShot(2000, this, SLOT(uncheckEvent4()));
	break;
    }
    writeAnnotation(ev, "events.txt");
}

// ---------------------------------------------------------------------

void AvRecorder::uncheckEvent1() {
    ui->eventButton_1->setChecked(false);
}
void AvRecorder::uncheckEvent2() {
    ui->eventButton_2->setChecked(false);
}
void AvRecorder::uncheckEvent3() {
    ui->eventButton_3->setChecked(false);
}
void AvRecorder::uncheckEvent4() {
    ui->eventButton_4->setChecked(false);
}

// ---------------------------------------------------------------------


// Local Variables:
// c-basic-offset: 4
// End:
