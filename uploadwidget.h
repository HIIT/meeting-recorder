#ifndef UPLOADWIDGET_H
#define UPLOADWIDGET_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>

#include "uploadthread.h"

class UploadWidget : public QDialog
{
    Q_OBJECT

signals:
    void passwordEntered(const QString&);

public slots:
    void appendText(const QString&);
    void updateProgressbar();
    void setMaximumProgressbar(int);
    void preferences();
    void preferences_new();
    void startUpload();
    void uploadOK();
    void passwordWidget();

public:
    UploadWidget(QWidget *parent, QString _directory);

private:
    QString username();
    QString server_ip();
    QString server_path();

    UploadThread *uploader;
    QPlainTextEdit *txt;
    QProgressBar *pbar;
    int pbar_value;
    QPushButton *exitButton;

    QSettings settings;

};

#endif // UPLOADWIDGET_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
