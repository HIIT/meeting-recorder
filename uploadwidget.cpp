
#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "uploadwidget.h"

// ---------------------------------------------------------------------

UploadWidget::UploadWidget(QWidget *parent, QString directory) : QDialog(parent) 
{

    resize(700,500);

    txt = new QPlainTextEdit();
    txt->setPlainText("UploadWidget() started");

    pbar_value = 0;
    pbar = new QProgressBar();

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok
						| QDialogButtonBox::Cancel);
    
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(txt);
    layout->addWidget(pbar);
    layout->addWidget(bb);
    setLayout(layout);

    uploader = new UploadThread(directory);

    uploader->start();

    connect(uploader, SIGNAL(uploadMessage(const QString&)), 
	    this, SLOT(appendText(const QString&)));
    connect(uploader, SIGNAL(blockSent()),
	    this, SLOT(updateProgressbar()));
    connect(uploader, SIGNAL(nBlocks(int)),
	    this, SLOT(setMaximumProgressbar(int)));

}

// ---------------------------------------------------------------------

void UploadWidget::appendText(const QString& text) {
    txt->appendPlainText(text);
}

// ---------------------------------------------------------------------

void UploadWidget::updateProgressbar() {
    // qDebug() << "updateProgressbar() called";
    pbar->setValue(++pbar_value);
}

// ---------------------------------------------------------------------

void UploadWidget::setMaximumProgressbar(int value) {
    qDebug() << "setMaximumProgressbar() called, value:" << value;
    pbar->setMinimum(0);
    pbar->setMaximum(value);
}

// ---------------------------------------------------------------------

// Local Variables:
// c-basic-offset: 4
// End:
