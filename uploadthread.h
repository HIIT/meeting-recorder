#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H

#include <QtCore>
#include <QThread>

extern "C" {
#include <libssh2.h>
#include <libssh2_sftp.h>
}

class UploadThread : public QThread
{
    Q_OBJECT

    void run();

signals:
    void uploadMessage(const QString &e);
    void blockSent();
    void nBlocks(int);
    void uploadFinished();
    void passwordRequested();

public slots:

    void setPreferences(const QString &);
    void setPassword(const QString &);

public:
    UploadThread(QString directory);

private:
    bool processFile(LIBSSH2_SFTP *, const QString &);
    bool checkDirectory(LIBSSH2_SFTP *, const QString &);

    /// local directory to upload
    QString directory;

    int buffersize;

    QString server_ip;
    QString server_path;
    QString username;  

    QString server_path_user;
    QString server_path_meeting;

    QString password;

    QWaitCondition passwordNeeded;
    QMutex mutex;

};

#endif // UPLOADTHREAD_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:

