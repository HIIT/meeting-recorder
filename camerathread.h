#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QImage>

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

private:
    QImage Mat2QImage(cv::Mat const& src);
};

#endif // CAMERATHREAD_H

