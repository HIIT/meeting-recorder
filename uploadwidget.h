#ifndef UPLOADWIDGET_H
#define UPLOADWIDGET_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QProgressBar>

#include "uploadthread.h"

class UploadWidget : public QDialog
{
    Q_OBJECT

signals:

public slots:
    void appendText(const QString&);
    void updateProgressbar();
    void setMaximumProgressbar(int);

public:
    UploadWidget(QWidget *parent, QString _directory);

private:
    UploadThread *uploader;
    QPlainTextEdit *txt;
    QProgressBar *pbar;
    int pbar_value;

};

#endif // UPLOADWIDGET_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
