
#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "uploadwidget.h"

// ---------------------------------------------------------------------

UploadWidget::UploadWidget(QWidget *parent, QString directory) : QDialog(parent) 
{

    txt = new QPlainTextEdit();
    txt->setPlainText("UploadWidget() started");

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok
						| QDialogButtonBox::Cancel);
    
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(txt);
    layout->addWidget(bb);
    setLayout(layout);

    uploader = new UploadThread(directory);

    uploader->start();

    connect(uploader, SIGNAL(uploadMessage(const QString&)), 
	    this, SLOT(appendText(const QString&)));

}

// ---------------------------------------------------------------------

void UploadWidget::appendText(const QString& text) {
    txt->appendPlainText(text);
}

// ---------------------------------------------------------------------

// Local Variables:
// c-basic-offset: 4
// End:
