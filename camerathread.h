#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QImage>
#include <QMediaRecorder>

// Include standard OpenCV headers
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

class CameraThread : public QThread
{
    Q_OBJECT

    void run();

signals:
    void qimgReady(int n, const QImage qimg);
    void resultReady(const QString &s);
    void cameraInfo(int, int, int, int);
    void errorMessage(const QString &e);

public slots:
    void setOutputDirectory(const QString &d);
    void onStateChanged(QMediaRecorder::State);
    void setCameraOutput(QString);
    void setCameraFramerate(QString);

public:
    void breakLoop();

private:
    QImage Mat2QImage(cv::Mat const& src);

    int framerate;

    double width_one, height_one, width_two, height_two;

    bool record_video;

    cv::VideoWriter video_one;
    cv::VideoWriter video_two;

    int width_out, height_out;

    QString outdir;

    bool stopLoop;

};

#endif // CAMERATHREAD_H

