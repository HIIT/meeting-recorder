
#include <QDebug>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>

#include "uploadwidget.h"

// ---------------------------------------------------------------------

UploadWidget::UploadWidget(QWidget *parent, QString directory) : QDialog(parent) 
{

    resize(700,500);

    txt = new QPlainTextEdit();
    txt->setPlainText("UploadWidget() started");

    pbar_value = 0;
    pbar = new QProgressBar();

    if (!settings.contains("username"))
	preferences();
    else
	appendText("using username: " + username());

    QPushButton *prefButton = new QPushButton("Preferences");
    QPushButton *startButton = new QPushButton("Start");
    exitButton = new QPushButton("Cancel");

    QDialogButtonBox *bb = new QDialogButtonBox();
    bb->addButton(prefButton, QDialogButtonBox::ActionRole);
    bb->addButton(startButton, QDialogButtonBox::ActionRole);
    bb->addButton(exitButton, QDialogButtonBox::RejectRole);

    connect(prefButton, SIGNAL(released()), this, SLOT(preferences()));
    connect(startButton, SIGNAL(released()), this, SLOT(startUpload()));
    connect(exitButton, SIGNAL(released()), this, SLOT(reject()));

    // QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok
    // 						| QDialogButtonBox::Cancel);
    
    // connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    // connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(txt);
    layout->addWidget(pbar);
    layout->addWidget(bb);
    setLayout(layout);

    uploader = new UploadThread(directory);

    connect(uploader, SIGNAL(uploadMessage(const QString&)), 
	    this, SLOT(appendText(const QString&)));
    connect(uploader, SIGNAL(blockSent()),
	    this, SLOT(updateProgressbar()));
    connect(uploader, SIGNAL(nBlocks(int)),
	    this, SLOT(setMaximumProgressbar(int)));
    connect(uploader, SIGNAL(uploadFinished()),
	    this, SLOT(uploadOK()));
    connect(uploader, SIGNAL(passwordRequested()),
	    this, SLOT(passwordWidget()));
    connect(this, SIGNAL(passwordEntered(const QString&)),
	    uploader, SLOT(setPassword(const QString&)));

}

// ---------------------------------------------------------------------

void UploadWidget::appendText(const QString& text) {
    txt->appendPlainText(text);
}

// ---------------------------------------------------------------------

void UploadWidget::updateProgressbar() {
    pbar->setValue(++pbar_value);
}

// ---------------------------------------------------------------------

void UploadWidget::setMaximumProgressbar(int value) {
    qDebug() << "setMaximumProgressbar() called, value:" << value;
    pbar->setMinimum(0);
    pbar->setMaximum(value);
}

// ---------------------------------------------------------------------

void UploadWidget::preferences() {
    bool ok;

    QString deftext = settings.value("username", "").toString();

    QString text = QInputDialog::getText(this, "Preferences",
	                                 "Username:", QLineEdit::Normal,
					 deftext, &ok);
    if (ok) {
	settings.setValue("username", text);
	appendText("username changed to: " + username());

    }
}

// ---------------------------------------------------------------------

void UploadWidget::preferences_new() {
    QDialog *prefs = new QDialog(this);

    QString deftext = settings.value("username", "").toString();

    QLineEdit *usernameEdit = new QLineEdit(deftext, prefs);
    QLineEdit *serveripEdit = new QLineEdit(prefs);
    QLineEdit *serverpathEdit = new QLineEdit(prefs);

    QDialogButtonBox *pbb = new QDialogButtonBox(QDialogButtonBox::Ok
						 | QDialogButtonBox::Cancel);

    connect(pbb, SIGNAL(accepted()), prefs, SLOT(accept()));
    connect(pbb, SIGNAL(rejected()), prefs, SLOT(reject()));

    QGroupBox *gb = new QGroupBox("Preferences");
    QFormLayout *flayout = new QFormLayout;
    flayout->addRow(new QLabel("HIIT username"), usernameEdit);
    flayout->addRow(new QLabel("server IP address"), serveripEdit);
    flayout->addRow(new QLabel("server path"), serverpathEdit);
    gb->setLayout(flayout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(gb);
    layout->addWidget(pbb);

    prefs->setLayout(layout);
    prefs->exec();

}
// ---------------------------------------------------------------------

void UploadWidget::startUpload() {
    uploader->setPreferences(username());
    uploader->start();
}


// ---------------------------------------------------------------------

QString UploadWidget::username() {
    return settings.value("username", "MISSING_USERNAME").toString();
}


// ---------------------------------------------------------------------

void UploadWidget::uploadOK() {
    exitButton->setText("Ok");
}

// ---------------------------------------------------------------------

void UploadWidget::passwordWidget() {
    bool ok;
    QString pw = QInputDialog::getText(NULL, "Enter your password",
				       "Password:", QLineEdit::Password,
				       "", &ok);
    if (ok)
	emit passwordEntered(pw);
    else
	emit passwordEntered("");

}

// ---------------------------------------------------------------------

// Local Variables:
// c-basic-offset: 4
// End:
