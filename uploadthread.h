#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H

#include <QThread>

class UploadThread : public QThread
{
    Q_OBJECT

    void run();

signals:
    void uploadMessage(const QString &e);
    void blockSent();
    void nBlocks(int);

public slots:

public:
    UploadThread(QString directory);

private:
    QString directory;

    int buffersize;

    QString server_ip;
    QString server_path;
    QString username;  
};

#endif // UPLOADTHREAD_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:

