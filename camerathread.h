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
    void cameraInfo(int, int, int);
    void errorMessage(const QString &e);

public slots:
    void setOutputDirectory(const QString &d);
    void onStateChanged(QMediaRecorder::State);
    void setCameraOutput(QString);
    void setCameraFramerate(QString);
    void setCameraPower(int, int);

public:
    CameraThread(int i);
    CameraThread(int i, QString wxh);

    void breakLoop();

private:
    QImage Mat2QImage(cv::Mat const& src);

    /// Aspect ratio preserving resize
    void resizeAR(cv::Mat &, cv::Size);

    void setDefaultDesiredInputSize();

    int framerate;
    int fourcc;

    int idx;

    cv::Size desired_input_size;

    cv::Size input_size;

    bool record_video;

    bool is_active, was_active;

    cv::VideoWriter video;

    cv::Size output_size;

    cv::Size window_size;

    QString outdir;
    QString filename;

    bool stopLoop;

};

#endif // CAMERATHREAD_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
