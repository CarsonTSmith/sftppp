#include "sftperror.h"

SFTPError::SFTPError(const int sshCode, const int sftpCode, const std::string& sshMsg)
    : m_sshCode(sshCode), m_sftpCode(sftpCode), m_sshErrorMsg(sshMsg) {}
