#include <QDebug>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "camerathread.h"

using namespace boost::posix_time;
using namespace cv;

CameraThread::CameraThread(int i) : idx(i)
{
}

void CameraThread::run() Q_DECL_OVERRIDE {
    QString result;

    time_duration td, td1;
    ptime nextFrameTimestamp, currentFrameTimestamp, initialLoopTimestamp, finalLoopTimestamp;
    int delayFound = 0;

    // initialize capture on default source
    VideoCapture capture(idx);
    if (!capture.isOpened()) {
      emit errorMessage(QString("ERROR: Failed to initialize camera %1. ")
                                .arg(idx));
      return;
    }

    framerate = 15;
    fourcc = CV_FOURCC('m','p','4','v');

    //capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    //capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    // Get the properties from the camera
    input_size.width =  capture.get(CV_CAP_PROP_FRAME_WIDTH);
    input_size.height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

    // print camera frame size
    qDebug() << "Camera" << idx
             << ": width =" << input_size.width <<"height ="<< input_size.height;
    emit cameraInfo(idx, input_size.width, input_size.height);

    outdir = "";
    filename = QString("capture%1.avi").arg(idx);

    record_video = false;
    output_size = Size(0,0);

    // initialize initial timestamps
    nextFrameTimestamp = microsec_clock::local_time();
    currentFrameTimestamp = nextFrameTimestamp;
    td = (currentFrameTimestamp - nextFrameTimestamp);

    stopLoop = false;
    for(;;) {

        if (stopLoop)
            break;

        // wait for X microseconds until 1second/framerate time has passed after previous frame write
        while(td.total_microseconds() < 1000000/framerate){
        //determine current elapsed time
            currentFrameTimestamp = microsec_clock::local_time();
            td = (currentFrameTimestamp - nextFrameTimestamp);
        }

        //	 determine time at start of write
        initialLoopTimestamp = microsec_clock::local_time();

        Mat frame;

        capture >> frame;

        if (output_size.width != 0) {
            resize(frame, frame, output_size);
        }

        // Save frame to video
        if (record_video) {
            if (video.isOpened())
                video << frame;
        }

        Mat window;
        resize(frame, window, Size(240,135));
        QImage qimg = Mat2QImage(window);
        emit qimgReady(idx, qimg);

        //write previous and current frame timestamp to console
        //qDebug() << nextFrameTimestamp << " " << currentFrameTimestamp << " ";

        // add 1second/framerate time for next loop pause
        nextFrameTimestamp = nextFrameTimestamp + microsec(1000000/framerate);

        // reset time_duration so while loop engages
        td = (currentFrameTimestamp - nextFrameTimestamp);

        //determine and print out delay in ms, should be less than 1000/FPS
        //occasionally, if delay is larger than said value, correction will occur
        //if delay is consistently larger than said value, then CPU is not powerful
        // enough to capture/decompress/record/compress that fast.
        finalLoopTimestamp = microsec_clock::local_time();
        td1 = (finalLoopTimestamp - initialLoopTimestamp);
        delayFound = td1.total_milliseconds();
        //qDebug() << "Delay: " << delayFound;

    }

    emit resultReady(result);
}
void CameraThread::setOutputDirectory(const QString &d) {
    outdir = d+"/";
}

void CameraThread::onStateChanged(QMediaRecorder::State state) {
    switch (state) {
    case QMediaRecorder::RecordingState:
        if (!video.isOpened())
            video.open(QString(outdir+filename).toStdString(), fourcc, framerate,
                       (output_size.width ? output_size : input_size));
        if (!video.isOpened()) {
            emit errorMessage(QString("ERROR: Failed to initialize recording for camera %1")
                                      .arg(idx));
        } else {
            record_video = true;
        }
        break;
    case QMediaRecorder::PausedState:
        record_video = false;
        break;
    case QMediaRecorder::StoppedState:
        record_video = false;
        video.release();
        break;
    }
}

QImage CameraThread::Mat2QImage(cv::Mat const& src) {
     cv::Mat temp; // make the same cv::Mat
     cvtColor(src, temp,CV_BGR2RGB); // cvtColor Makes a copt, that what i need
     QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
     dest.bits(); // enforce deep copy, see documentation
     // of QImage::QImage ( const uchar * data, int width, int height, Format format )
     return dest;
}

void CameraThread::setCameraOutput(QString wxh) {
    qDebug() << "CameraThread::setCameraOutput(): " << wxh;
    if (wxh == "Original") {
        output_size = Size(0,0);
    } else {
        QStringList wh = wxh.split("x");
        if (wh.length()==2) {
            output_size = Size(wh.at(0).toInt(), wh.at(1).toInt());
        }
    }
}

void CameraThread::setCameraFramerate(QString fps) {
    qDebug() << "CameraThread::setCameraFramerate(): " << fps;
    framerate  = fps.toInt();
}

void CameraThread::breakLoop() {
    stopLoop = true;
}
