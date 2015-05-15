#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H

#include <QThread>

class UploadThread : public QThread
{
    Q_OBJECT

    void run();

signals:
    void uploadMessage(const QString &e);

public slots:

public:
    UploadThread(QString directory);

private:
    QString directory;

    QString server_ip;
    QString server_path;
    QString username;  
};

#endif // UPLOADTHREAD_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:

