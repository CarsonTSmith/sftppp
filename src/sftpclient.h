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

#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#ifdef _WIN32
#include <io.h>  // For _S_IREAD and _S_IWRITE
#define open _open
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD  // Read permission for user
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE  // Write permission for user
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#elif _POSIX_VERSION
#include <fcntl.h>     // O_WRONLY, O_CREAT, O_TRUNC
#include <sys/stat.h>  // mode_t, S_IRUSR, S_IWUSR
#endif

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "sftpattributes.h"
#include "sftperror.h"

namespace cts {

class SFTPClient {
   public:
    SFTPClient() = default;
    ~SFTPClient();

    SFTPClient(SFTPClient&&) = default;

    SFTPClient& operator=(SFTPClient&&) = default;

    SFTPError connect(const std::string& host, const std::string& user, const std::string& pw,
                      const uint16_t port = 22, const bool onlyKnownServers = true);

    void disconnect();

    SFTPError put(const std::string& localFileName, const std::string& remoteFileName,
                  unsigned int chunkSize = kDefaultChunkSize) const;

    SFTPError get(const std::string& localFileName, const std::string& remoteFileName,
                  unsigned int chunkSize = kDefaultChunkSize) const;

    SFTPError mkdir(const std::string& remoteDir, const mode_t permissions) const;

    std::pair<SFTPError, std::vector<SFTPAttributes>> ls(const std::string& remoteDir) const;

    SFTPError rename(const std::string& oldRemoteName, const std::string& newRemoteName) const;

    SFTPError rm(const std::string& remoteFileName) const;

    SFTPError rmdir(const std::string& remoteDirName) const;

    std::pair<SFTPError, SFTPAttributes> stat(const std::string& remotePath) const;

   private:
    SFTPClient& operator=(const SFTPClient&) = delete;
    SFTPClient(const SFTPClient&) = delete;

    struct SSHSessionDeleter {
        void operator()(ssh_session session) const;
    };

    struct SFTPSessionDeleter {
        void operator()(sftp_session session) const;
    };

    struct SFTPFileDeleter {
        void operator()(sftp_file file) const;
    };

    using SSHSessionPtr = std::unique_ptr<ssh_session_struct, SSHSessionDeleter>;
    using SFTPSessionPtr = std::unique_ptr<sftp_session_struct, SFTPSessionDeleter>;

    SSHSessionPtr m_sshSession;
    SFTPSessionPtr m_sftpSession;

    static constexpr unsigned int kDefaultChunkSize = 16 * 1024;
    static constexpr unsigned int kMaxChunkSize = 32 * 1024;
};

}  // namespace cts

#endif /* SFTP_CLIENT_H */