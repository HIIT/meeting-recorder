#include <QDebug>
#include <QDir>

#include "uploadthread.h"

extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <libssh2.h>
#include <libssh2_sftp.h>
}

// ---------------------------------------------------------------------

UploadThread::UploadThread(QString _dir) : directory(_dir)
{
  server_ip = "128.214.113.2";
  server_path = "/home/fs/jmakoske/foo";
  username = "jmakoske";  
}

// ---------------------------------------------------------------------

void UploadThread::run() Q_DECL_OVERRIDE {
  emit uploadMessage("UploadThread::run() starting");

  int rc = libssh2_init(0);
  if (rc != 0) {
    emit uploadMessage(QString("libssh2_init() failed (%1)").arg(rc));
    return;
  }
  emit uploadMessage("libssh2 initialization successful");

  QString lf = directory + "/audio.wav";
  QByteArray ba_lf = lf.toLatin1();
  const char *loclfile = ba_lf.data();
  FILE *local = fopen(loclfile, "rb");
  if (!local) {
    emit uploadMessage(QString("can't open local file %1").arg(loclfile));
    return;
  }
  emit uploadMessage(QString("opened local file %1").arg(loclfile));

  QString sp = server_path + "/audio.wav";
  QByteArray ba_sp = sp.toLatin1();
  const char *sftppath = ba_sp.data();  

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  unsigned long hostaddr = inet_addr(server_ip.toStdString().c_str());

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(22);
  sin.sin_addr.s_addr = hostaddr;
  if (::connect(sock, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
    emit uploadMessage("failed to connect()");
    return;
  }
  emit uploadMessage("connection established");

  LIBSSH2_SESSION *session = libssh2_session_init();
  if (!session) {
    emit uploadMessage("libssh2_session_init() failed"); 
    return;
  }
  emit uploadMessage("libssh2 session initialization successful");

  libssh2_session_set_blocking(session, 1);
  rc = libssh2_session_handshake(session, sock);
  if (rc) {
    emit uploadMessage(QString("failure establishing SSH session (%1)").arg(rc));
    return;
  }
  emit uploadMessage("session established");

  const char *fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
  QString fp = "fingerprint: ";
  for(int i = 0; i < 20; i++) {
    QChar c = (QChar)fingerprint[i];
      fp += QString::number(c.unicode());
      fp += " ";
  }
  emit uploadMessage(fp);

  QString pubkey = QDir::homePath()+"/.ssh/id_rsa.pub";
  QString prikey = QDir::homePath()+"/.ssh/id_rsa";
  emit uploadMessage("public key: "+ pubkey);
  emit uploadMessage("private key: "+ prikey);

  LIBSSH2_SFTP *sftp_session;
  LIBSSH2_SFTP_HANDLE *sftp_handle;

  // authentication by public key:
  const char *password="password";
  if (libssh2_userauth_publickey_fromfile(session, 
					  username.toStdString().c_str(),
					  pubkey.toStdString().c_str(),
					  prikey.toStdString().c_str(),
					  password)) {
    emit uploadMessage("authentication by public key failed");
    goto shutdown;
  }
  emit uploadMessage("authentication by public key successful");

  emit uploadMessage("libssh2_sftp_init()!");
  sftp_session = libssh2_sftp_init(session);
  if (!sftp_session) {
    emit uploadMessage("unable to init SFTP session");
    goto shutdown;
  }
  
  emit uploadMessage(QString("libssh2_sftp_open() for %1").arg(sftppath));
  /* Request a file via SFTP */
  sftp_handle =
    libssh2_sftp_open(sftp_session, sftppath,
                      LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
                      LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
                      LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
  
  if (!sftp_handle) {
    emit uploadMessage("unable to open file with SFTP");
    goto shutdown;
  }

  emit uploadMessage("libssh2_sftp_open() is done, now send data!");
  char mem[1024*100], *ptr;

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
    
  } while (rc > 0);
  
  libssh2_sftp_close(sftp_handle);
  libssh2_sftp_shutdown(sftp_session);

shutdown:
  libssh2_session_disconnect(session,
			     "Normal Shutdown, Thank you for playing");
  libssh2_session_free(session);

  close(sock);
  if (local)
    fclose(local);  
  emit uploadMessage("all done");

  libssh2_exit();

  emit uploadMessage("UploadThread::run() ending");
}
// ---------------------------------------------------------------------
