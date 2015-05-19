#include <QDebug>
#include <QDir>

#include "uploadthread.h"

extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
}

// ---------------------------------------------------------------------

UploadThread::UploadThread(QString _dir) : directory(_dir)
{
  buffersize = 1024*100;

  server_ip = "128.214.113.2";
  server_path = "/home/fs/jmakoske/foo";
  username = "";
  password = "";
}

// ---------------------------------------------------------------------

void UploadThread::run() Q_DECL_OVERRIDE {
  emit uploadMessage("UploadThread::run() starting");

  if (username == "") {
    emit uploadMessage("username not set, exiting");
    return;
  }
  server_path_user = server_path + "/" + username;
  server_path_meeting = server_path_user + "/" + directory.section('/', -1);
  emit uploadMessage("target directory: "+server_path_meeting);

  int rc = libssh2_init(0);
  if (rc != 0) {
    emit uploadMessage(QString("libssh2_init() failed (%1), exiting")
		       .arg(rc));
    return;
  }
  emit uploadMessage("libssh2 initialization successful");

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  unsigned long hostaddr = inet_addr(server_ip.toStdString().c_str());

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(22);
  sin.sin_addr.s_addr = hostaddr;
  if (::connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
    emit uploadMessage("failed to connect(), exiting");
    return;
  }
  emit uploadMessage("connection established");

  LIBSSH2_SESSION *session = libssh2_session_init();
  if (!session) {
    emit uploadMessage("libssh2_session_init() failed, exiting");
    return;
  }
  emit uploadMessage("libssh2 session initialization successful");

  libssh2_session_set_blocking(session, 1);
  rc = libssh2_session_handshake(session, sock);
  if (rc) {
    emit uploadMessage(QString("failure establishing SSH session (%1)"
			       ", exiting").arg(rc));
    return;
  }
  emit uploadMessage("session established");

  const char *fingerprint = libssh2_hostkey_hash(session,
						 LIBSSH2_HOSTKEY_HASH_SHA1);
  QString fp = "fingerprint: ";
  for(int i = 0; i < 20; i++) {
    QChar c = (QChar)fingerprint[i];
      fp += QString::number(c.unicode());
      fp += " ";
  }
  emit uploadMessage(fp);

  QString pubkey = QDir::homePath()+"/.ssh/id_rsa.pub";
  QString prikey = QDir::homePath()+"/.ssh/id_rsa";
  emit uploadMessage("using public key: "+ pubkey);
  emit uploadMessage("using private key: "+ prikey);

  LIBSSH2_SFTP *sftp_session;

  QDir dir(directory);
  dir.setFilter(QDir::NoDotAndDotDot|QDir::Files);
  QStringList files = dir.entryList();
  long long totalsize = 0;

  // authentication by public key:
  const char *dummypassword="password";
  if (libssh2_userauth_publickey_fromfile(session, 
					  username.toStdString().c_str(),
					  pubkey.toStdString().c_str(),
					  prikey.toStdString().c_str(),
					  dummypassword)) {
    emit uploadMessage("authentication by public key failed");

    //authentication via password
    mutex.lock();
    emit passwordRequested();
    passwordNeeded.wait(&mutex);
    mutex.unlock();
    if (password == "") {
      emit uploadMessage("authentication by password cancelled");
      goto shutdown;
    }
    if (libssh2_userauth_password(session,
				  username.toStdString().c_str(),
				  password.toStdString().c_str())) {
      emit uploadMessage("authentication by password failed");
      goto shutdown;
    } else
      emit uploadMessage("authentication by password successful");

  } else
    emit uploadMessage("authentication by public key successful");

  emit uploadMessage("libssh2_sftp_init()!");
  sftp_session = libssh2_sftp_init(session);
  if (!sftp_session) {
    emit uploadMessage("unable to init SFTP session");
    goto shutdown;
  }

  if (!checkDirectory(sftp_session, server_path_user))
    goto shutdown;
  if (!checkDirectory(sftp_session, server_path_meeting))
    goto shutdown;

  for (int i = 0; i < files.size(); ++i) {
    QString lf = directory + "/" + files.at(i);
    QFileInfo lf_info(lf);
    totalsize += lf_info.size();
  }
  emit nBlocks(totalsize/buffersize+1);

  for (int i = 0; i < files.size(); ++i)
    if (!processFile(sftp_session, files.at(i)))
      goto shutdown;

  libssh2_sftp_shutdown(sftp_session);

shutdown:
  libssh2_session_disconnect(session,
			     "Normal Shutdown, Thank you for playing");
  libssh2_session_free(session);

  close(sock);
  emit uploadMessage("all done");

  libssh2_exit();

  emit uploadMessage("UploadThread::run() ending");
}

// ---------------------------------------------------------------------

bool UploadThread::checkDirectory(LIBSSH2_SFTP *sftp_session, 
				  const QString &dir) {

  QByteArray ba_sp = dir.toLatin1();
  const char *sftppath = ba_sp.data();
  LIBSSH2_SFTP_ATTRIBUTES fileinfo;

  int rc = libssh2_sftp_stat(sftp_session, sftppath, &fileinfo);

  if (rc) {
    emit uploadMessage(QString("server path %1 does not exist, creating it").
		       arg(dir));
    rc = libssh2_sftp_mkdir(sftp_session, sftppath,
			    LIBSSH2_SFTP_S_IRWXU|
			    LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IXGRP|
			    LIBSSH2_SFTP_S_IROTH|LIBSSH2_SFTP_S_IXOTH);

    if (rc) {
      emit uploadMessage(QString("libssh2_sftp_mkdir failed: (%1)").arg(rc));
      return false;
    }
  } else
    emit uploadMessage(QString("server path %1 exists").arg(dir));

  return true;
}

// ---------------------------------------------------------------------

bool UploadThread::processFile(LIBSSH2_SFTP *sftp_session,
			       const QString &filename) {

  QString lf = directory + "/" + filename;

  QByteArray ba_lf = lf.toLatin1();
  const char *loclfile = ba_lf.data();
  FILE *local = fopen(loclfile, "rb");
  if (!local) {
    emit uploadMessage(QString("can't open local file %1")
		       .arg(loclfile));
    return false;
  }
  emit uploadMessage(QString("opened local file %1").arg(loclfile));

  QString sp = server_path_meeting + "/" + filename;
  QByteArray ba_sp = sp.toLatin1();
  const char *sftppath = ba_sp.data();

  emit uploadMessage(QString("libssh2_sftp_open() for %1").arg(sftppath));
  /* Request a file via SFTP */
  LIBSSH2_SFTP_HANDLE *sftp_handle =
    libssh2_sftp_open(sftp_session, sftppath,
                      LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                      LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
                      LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
  
  if (!sftp_handle) {
    emit uploadMessage("unable to open file with SFTP");
    if (local)
      fclose(local);
    return false;
  }

  emit uploadMessage("libssh2_sftp_open() is done, now sending data");

  char mem[buffersize], *ptr;
  int rc;

  do {
    size_t nread = fread(mem, 1, sizeof(mem), local);
    if (nread <= 0) {
      /* end of file */
      break;
    }
    ptr = mem;
    
    do {
      /* write data in a loop until we block */
      rc = libssh2_sftp_write(sftp_handle, ptr, nread);
      if(rc < 0)
	break;
      ptr += rc;
      nread -= rc;
    } while (nread);

    emit blockSent();
  } while (rc > 0);
  
  libssh2_sftp_close(sftp_handle);

  if (local)
    fclose(local);  

  emit uploadMessage("data sent successfully");
  emit uploadFinished();

  return true;
}

// ---------------------------------------------------------------------

void UploadThread::setPreferences(const QString &_username) {
  username = _username;
}

// ---------------------------------------------------------------------

void UploadThread::setPassword(const QString &_password) {
  password = _password;
  mutex.lock();
  passwordNeeded.wakeAll();
  mutex.unlock();
}

// ---------------------------------------------------------------------
