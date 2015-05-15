#ifndef UPLOADWIDGET_H
#define UPLOADWIDGET_H

#include <QDialog>
#include <QPlainTextEdit>

#include "uploadthread.h"

class UploadWidget : public QDialog
{
    Q_OBJECT

signals:

public slots:
    void appendText(const QString&);

public:
    UploadWidget(QWidget *parent, QString _directory);

private:
    UploadThread *uploader;
    QPlainTextEdit *txt;

};

#endif // UPLOADWIDGET_H

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
