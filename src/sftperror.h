#ifndef SFTP_ERROR_H
#define SFTP_ERROR_H

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <string>

class SFTPError {
   public:
    SFTPError(const int code = SSH_OK, const int sftpCode = SSH_FX_OK,
              const std::string& sshMsg = "");

    bool isOk() const { return m_sshCode == SSH_OK && m_sftpCode == SSH_FX_OK; }

    int getSSHErrorCode() const { return m_sshCode; }

    int getSFTPErrorCode() const { return m_sftpCode; }

    std::string getSSHErrorMsg() const { return m_sshErrorMsg; }

   private:
    int m_sshCode;
    int m_sftpCode;
    std::string m_sshErrorMsg;
};

#endif /* SFTP_ERROR_H */