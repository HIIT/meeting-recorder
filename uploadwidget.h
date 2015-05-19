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

public slots:
    void appendText(const QString&);
    void updateProgressbar();
    void setMaximumProgressbar(int);
    void preferences();
    void startUpload();
    void uploadOK();

public:
    UploadWidget(QWidget *parent, QString _directory);

private:
    QString username();

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
