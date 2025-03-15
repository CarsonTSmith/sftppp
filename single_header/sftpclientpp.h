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
    #define O_RDONLY _O_RDONLY  // Write permission for user
#endif
#elif _POSIX_VERSION
#include <fcntl.h>     // O_WRONLY, O_CREAT, O_TRUNC
#include <sys/stat.h>  // mode_t, S_IRUSR, S_IWUSR
#endif

#include <algorithm>  // min
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class SFTPAttributes {
   public:
    using SFTPAttributesPtr = std::shared_ptr<sftp_attributes_struct>;

    SFTPAttributes(sftp_attributes attr) : m_sftpAttributes(attr, SFTPAttributesDeleter()) {}
    ~SFTPAttributes() = default;

    SFTPAttributes(const SFTPAttributes& other) = default;
    SFTPAttributes(SFTPAttributes&& other) = default;

    SFTPAttributes& operator=(const SFTPAttributes& other) = default;
    SFTPAttributes& operator=(SFTPAttributes&& other) = default;

    const SFTPAttributesPtr &get() const { return m_sftpAttributes; }

   private:
    struct SFTPAttributesDeleter {
        void operator()(sftp_attributes attr) const {
            if (attr) {
                sftp_attributes_free(attr);
            }
        }
    };

    SFTPAttributesPtr m_sftpAttributes;
};

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

class SFTPClient {
   public:
    SFTPClient() = default;
    ~SFTPClient();

    SFTPClient(const SFTPClient&) = default;
    SFTPClient(SFTPClient&&) = default;

    SFTPClient& operator=(const SFTPClient&) = default;
    SFTPClient& operator=(SFTPClient&&) = default;

    SFTPError connect(const std::string& host, const std::string& user, const std::string& pw,
                      const uint16_t port = 22);

    void disconnect();

    SFTPError put(const std::string& localFileName, const std::string& remoteFileName,
                  unsigned int chunkSize = kDefaultChunkSize);

    SFTPError get(const std::string& localFileName, const std::string& remoteFileName,
                  unsigned int chunkSize = kDefaultChunkSize);

    SFTPError mkdir(const std::string& remoteDir, const mode_t permissions);

    std::pair<SFTPError, std::vector<SFTPAttributes>> ls(const std::string& remoteDir);

    SFTPError rename(const std::string& oldRemoteName, const std::string& newRemoteName);

    SFTPError rm(const std::string& remoteFileName);

    SFTPError rmdir(const std::string& remoteDirName);

   private:
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

SFTPError::SFTPError(const int sshCode, const int sftpCode, const std::string& sshMsg)
    : m_sshCode(sshCode), m_sftpCode(sftpCode), m_sshErrorMsg(sshMsg) {}

SFTPClient::~SFTPClient() { disconnect(); }

SFTPError SFTPClient::connect(const std::string& host, const std::string& user,
                              const std::string& pw, const uint16_t port) {
    m_sshSession = SSHSessionPtr(ssh_new());
    if (m_sshSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_OK, "Failed to create ssh session.");
    }

    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_HOST, host.c_str());
    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_PORT, &port);
    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_USER, user.c_str());

    int rc = ssh_connect(m_sshSession.get());
    if (rc != SSH_OK) {
        return SFTPError(rc, SSH_FX_OK, ssh_get_error(m_sshSession.get()));
    }

    // rc = verify_knownhost(m_sshSession.get());
    // if (rc < 0) {
    //     return SFTPError(rc, ssh_get_error(m_sshSession.get()));
    // }

    rc = ssh_userauth_password(m_sshSession.get(), nullptr, pw.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        return SFTPError(rc, SSH_FX_OK, ssh_get_error(m_sshSession.get()));
    }

    m_sftpSession = SFTPSessionPtr(sftp_new(m_sshSession.get()));
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()), SSH_FX_FAILURE,
                         "Failed to create a new sftp session.");
    }

    rc = sftp_init(m_sftpSession.get());
    if (rc != SSH_OK) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()), ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

void SFTPClient::disconnect() {
    m_sftpSession.reset();
    m_sshSession.reset();
}

SFTPError SFTPClient::put(const std::string& localFileName, const std::string& remoteFileName,
                          unsigned int chunkSize) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }

    if (chunkSize < 1) {
        chunkSize = kDefaultChunkSize;
    }

    chunkSize = std::min(chunkSize, kMaxChunkSize);

    std::ifstream file(localFileName, std::ios::binary);
    if (!file) {
        return SFTPError(SSH_OK, SSH_FX_NO_SUCH_FILE,
                         "Failed to open local file: " + localFileName);
    }

    auto remoteFilePtr = std::unique_ptr<sftp_file_struct, SFTPFileDeleter>(
        sftp_open(m_sftpSession.get(), remoteFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR));

    if (!remoteFilePtr.get()) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()), ssh_get_error(m_sshSession.get()));
    }

    std::vector<char> buffer(chunkSize);
    size_t totalBytes = 0;

    while (file) {
        file.read(buffer.data(), chunkSize);
        auto bytesRead = file.gcount();

        if (bytesRead == 0) {
            break;  // End of file
        }

        auto bytesWritten = sftp_write(remoteFilePtr.get(), buffer.data(), bytesRead);
        if (bytesWritten < 0) {
            return SFTPError(ssh_get_error_code(m_sshSession.get()),
                             sftp_get_error(m_sftpSession.get()),
                             "Failed to write to remote file [" + remoteFileName + "] " +
                                 ssh_get_error(m_sshSession.get()));
        }

        totalBytes += bytesWritten;
    }

    return SFTPError();
}

SFTPError SFTPClient::get(const std::string& remoteFileName, const std::string& localFileName,
                          unsigned int chunkSize) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }

    if (chunkSize < 1) {
        chunkSize = kDefaultChunkSize;
    }

    chunkSize = std::min(chunkSize, kMaxChunkSize);

    auto remoteFilePtr = std::unique_ptr<sftp_file_struct, SFTPFileDeleter>(
        sftp_open(m_sftpSession.get(), remoteFileName.c_str(), O_RDONLY, S_IRUSR));

    if (!remoteFilePtr.get()) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()),
                         "Failed to open remote file [" + remoteFileName + "] " +
                             ssh_get_error(m_sshSession.get()));
    }

    std::ofstream file(localFileName, std::ios::binary);
    if (!file) {
        return SFTPError(SSH_OK, SSH_FX_NO_SUCH_FILE,
                         "Failed to open local file: " + localFileName);
    }

    std::vector<char> buffer(chunkSize);
    size_t totalBytes = 0;

    while (true) {
        auto bytesRead = sftp_read(remoteFilePtr.get(), buffer.data(), chunkSize);

        if (bytesRead < 0) {
            return SFTPError(ssh_get_error_code(m_sshSession.get()),
                             sftp_get_error(m_sftpSession.get()),
                             "Failed to read from remote file [" + remoteFileName + "] " +
                                 ssh_get_error(m_sshSession.get()));
        }

        if (bytesRead == 0) {
            break;  // End of file
        }

        file.write(buffer.data(), bytesRead);
        if (!file) {
            return SFTPError(SSH_OK, SSH_FX_FAILURE,
                             "Failed to write to local file [" + localFileName + "]");
        }

        totalBytes += bytesRead;
    }

    return SFTPError();
}

SFTPError SFTPClient::mkdir(const std::string& remoteDir, const mode_t permissions) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }

    int rc = sftp_mkdir(m_sftpSession.get(), remoteDir.c_str(), permissions);
    if (rc != SSH_OK) {
        return SFTPError(rc, sftp_get_error(m_sftpSession.get()),
                         ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

std::pair<SFTPError, std::vector<SFTPAttributes>> SFTPClient::ls(const std::string& remoteDir) {
    if (!m_sftpSession.get()) {
        return {SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session"), {}};
    }

    auto dir = std::unique_ptr<sftp_dir_struct, decltype(&sftp_closedir)>(
        sftp_opendir(m_sftpSession.get(), remoteDir.c_str()), sftp_closedir);

    if (!dir) {
        return {
            SFTPError(ssh_get_error_code(m_sftpSession.get()), sftp_get_error(m_sftpSession.get()),
                      "Failed to open directory: " + remoteDir),
            {}};
    }

    std::vector<SFTPAttributes> attributesList;

    // Read directory contents
    while (sftp_attributes attributes = sftp_readdir(m_sftpSession.get(), dir.get())) {
        attributesList.emplace_back(attributes);
    }

    // Check if the directory listing was complete
    if (!sftp_dir_eof(dir.get())) {
        return {
            SFTPError(ssh_get_error_code(m_sftpSession.get()), sftp_get_error(m_sftpSession.get()),
                      "Failed to read directory: " + remoteDir),
            {}};
    }

    return {SFTPError(SSH_OK, SSH_FX_OK), std::move(attributesList)};
}

SFTPError SFTPClient::rename(const std::string& oldRemoteName, const std::string& newRemoteName) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }

    int rc = sftp_rename(m_sftpSession.get(), oldRemoteName.c_str(), newRemoteName.c_str());
    if (rc < 0) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()), ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

SFTPError SFTPClient::rm(const std::string& remoteFileName) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }

    int rc = sftp_unlink(m_sftpSession.get(), remoteFileName.c_str());
    if (rc != SSH_OK) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()),
                         "Failed to remove remote file [" + remoteFileName + "] " +
                             ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

SFTPError SFTPClient::rmdir(const std::string& remoteDir) {
    if (m_sftpSession.get() == nullptr) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP session");
    }
    int rc = sftp_rmdir(m_sftpSession.get(), remoteDir.c_str());
    if (rc != SSH_OK) {
        return SFTPError(
            ssh_get_error_code(m_sshSession.get()), sftp_get_error(m_sftpSession.get()),
            "Failed to remove remote dir [" + remoteDir + "] " + ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

void SFTPClient::SSHSessionDeleter::operator()(ssh_session session) const {
    if (session != nullptr) {
        ssh_disconnect(session);
        ssh_free(session);
    }
}

void SFTPClient::SFTPSessionDeleter::operator()(sftp_session session) const {
    if (session != nullptr) {
        sftp_free(session);
    }
}

void SFTPClient::SFTPFileDeleter::operator()(sftp_file file) const {
    if (file) {
        sftp_close(file);
    }
}
