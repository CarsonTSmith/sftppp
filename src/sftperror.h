// MIT License
//
// Copyright (c) 2025 Carson Smith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SFTP_ERROR_H
#define SFTP_ERROR_H

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <string>

namespace cts {

class SFTPError {
   public:
    SFTPError(const int sshCode = SSH_OK, const int sftpCode = SSH_FX_OK,
              const std::string& sshMsg = "")
        : m_sshCode(sshCode), m_sftpCode(sftpCode), m_sshErrorMsg(sshMsg) {}

    bool isOk() const { return m_sshCode == SSH_OK && m_sftpCode == SSH_FX_OK; }

    int getSSHErrorCode() const { return m_sshCode; }

    int getSFTPErrorCode() const { return m_sftpCode; }

    std::string getSSHErrorMsg() const { return m_sshErrorMsg; }

   private:
    int m_sshCode;
    int m_sftpCode;
    std::string m_sshErrorMsg;
};

}  // namespace cts

#endif /* SFTP_ERROR_H */