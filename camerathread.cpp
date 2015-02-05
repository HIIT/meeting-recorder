#include <QDebug>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "camerathread.h"

using namespace boost::posix_time;
using namespace cv;

void CameraThread::run() Q_DECL_OVERRIDE {
    QString result;

    time_duration td, td1;
    ptime nextFrameTimestamp, currentFrameTimestamp, initialLoopTimestamp, finalLoopTimestamp;
    int delayFound = 0;

    // initialize capture on default source
    VideoCapture capture_one(0);
    VideoCapture capture_two(1);
    if (!capture_one.isOpened() || !capture_two.isOpened()) {
      emit errorMessage(QString("ERROR: Failed to initialize cameras. "
                                "Status 0: %1, 1: %2").arg(capture_one.isOpened()).arg(capture_two.isOpened()));
      return;
    }

    // set framerate to record and capture at
    framerate = 15;

    // Get the properties from the camera
    width_one  = capture_one.get(CV_CAP_PROP_FRAME_WIDTH);
    height_one = capture_one.get(CV_CAP_PROP_FRAME_HEIGHT);
    width_two  = capture_two.get(CV_CAP_PROP_FRAME_WIDTH);
    height_two = capture_two.get(CV_CAP_PROP_FRAME_HEIGHT);

    // print camera frame size
    qDebug() << "Camera properties";
    qDebug() << "width = " << width_one << endl <<"height = "<< height_one;
    qDebug() << "width = " << width_two << endl <<"height = "<< height_two;
    emit cameraInfo(width_one, height_one, width_two, height_two);

    outdir = "";

    record_video = false;
    width_out = 0;
    height_out = 0;

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

        Mat frame_one, frame_two;

        capture_one >> frame_one;
        capture_two >> frame_two;

        if (width_out != 0) {
            resize(frame_one, frame_one, Size(width_out, height_out));
            resize(frame_two, frame_two, Size(width_out, height_out));
        }

        // Save frame to video
        if (record_video) {
            if (video_one.isOpened())
                video_one << frame_one;
            if (video_two.isOpened())
                video_two << frame_two;
        }

        Mat window_one;
        resize(frame_one, window_one, Size(240,135));
        QImage qimg_one = Mat2QImage(window_one);
        emit qimgReady(0, qimg_one);

        Mat window_two;
        resize(frame_two, window_two, Size(240,135));
        QImage qimg_two = Mat2QImage(window_two);
        emit qimgReady(1, qimg_two);

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
        if (!video_one.isOpened())
            video_one.open(outdir.toStdString()+"captureone.avi", CV_FOURCC('m','p','4','v'), framerate,
                           cvSize((width_out?width_out:(int)width_one),
                                  (height_out?height_out:(int)height_one)));
        if (!video_two.isOpened())
            video_two.open(outdir.toStdString()+"capturetwo.avi", CV_FOURCC('m','p','4','v'), framerate,
                           cvSize((width_out?width_out:(int)width_two),
                                  (height_out?height_out:(int)height_two)));
        if (!video_one.isOpened() || !video_two.isOpened()) {
            emit errorMessage(QString("ERROR: Failed to initialize recording. "
                                      "Status 0: %1, 1: %2").arg(video_one.isOpened()).arg(video_two.isOpened()));
        } else {
            record_video = true;
        }
        break;
    case QMediaRecorder::PausedState:
        record_video = false;
        break;
    case QMediaRecorder::StoppedState:
        record_video = false;
        video_one.release();
        video_two.release();
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
        width_out  = 0;
        height_out = 0;
    } else {
        QStringList wh = wxh.split("x");
        if (wh.length()==2) {
            width_out  = wh.at(0).toInt();
            height_out = wh.at(1).toInt();
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
