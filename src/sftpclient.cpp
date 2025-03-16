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

#include "sftpclient.h"

#include <algorithm>  // min

SFTPClient::~SFTPClient() { disconnect(); }

SFTPError SFTPClient::connect(const std::string& host, const std::string& user,
                              const std::string& pw, const uint16_t port,
                              const bool onlyKnownServers) {
    m_sshSession = SSHSessionPtr(ssh_new());
    if (!m_sshSession) {
        return SFTPError(SSH_ERROR, SSH_FX_OK, "Failed to create ssh session.");
    }

    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_HOST, host.c_str());
    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_PORT, &port);
    ssh_options_set(m_sshSession.get(), SSH_OPTIONS_USER, user.c_str());

    int rc = ssh_connect(m_sshSession.get());
    if (rc != SSH_OK) {
        return SFTPError(rc, SSH_FX_OK, ssh_get_error(m_sshSession.get()));
    }

    if (onlyKnownServers) {
        const auto res = ssh_session_is_known_server(m_sshSession.get());
        if (res != SSH_KNOWN_HOSTS_OK) {
            return SFTPError(ssh_get_error_code(m_sshSession.get()),
                             sftp_get_error(m_sftpSession.get()),
                             ssh_get_error(m_sshSession.get()));
        }
    }

    rc = ssh_userauth_password(m_sshSession.get(), nullptr, pw.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        return SFTPError(rc, SSH_FX_OK, ssh_get_error(m_sshSession.get()));
    }

    m_sftpSession = SFTPSessionPtr(sftp_new(m_sshSession.get()));
    if (!m_sftpSession) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()), SSH_FX_FAILURE,
                         "Failed to create a new sftp session.");
    }

    rc = sftp_init(m_sftpSession.get());
    if (rc < 0) {
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
                          unsigned int chunkSize) const {
    if (!m_sftpSession || !m_sshSession) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
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

SFTPError SFTPClient::get(const std::string& localFileName, const std::string& remoteFileName,
                          unsigned int chunkSize) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
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

SFTPError SFTPClient::mkdir(const std::string& remoteDir, const mode_t permissions) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
    }

    int rc = sftp_mkdir(m_sftpSession.get(), remoteDir.c_str(), permissions);
    if (rc < 0) {
        return SFTPError(rc, sftp_get_error(m_sftpSession.get()),
                         ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

std::pair<SFTPError, std::vector<SFTPAttributes>> SFTPClient::ls(
    const std::string& remoteDir) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return {SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session"), {}};
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

    while (sftp_attributes attributes = sftp_readdir(m_sftpSession.get(), dir.get())) {
        attributesList.emplace_back(attributes);
    }

    if (!sftp_dir_eof(dir.get())) {
        return {
            SFTPError(ssh_get_error_code(m_sftpSession.get()), sftp_get_error(m_sftpSession.get()),
                      "Failed to read directory: " + remoteDir),
            {}};
    }

    return {SFTPError(SSH_OK, SSH_FX_OK), std::move(attributesList)};
}

SFTPError SFTPClient::rename(const std::string& oldRemoteName,
                             const std::string& newRemoteName) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
    }

    int rc = sftp_rename(m_sftpSession.get(), oldRemoteName.c_str(), newRemoteName.c_str());
    if (rc < 0) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()), ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

SFTPError SFTPClient::rm(const std::string& remoteFileName) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
    }

    int rc = sftp_unlink(m_sftpSession.get(), remoteFileName.c_str());
    if (rc < 0) {
        return SFTPError(ssh_get_error_code(m_sshSession.get()),
                         sftp_get_error(m_sftpSession.get()),
                         "Failed to remove remote file [" + remoteFileName + "] " +
                             ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

SFTPError SFTPClient::rmdir(const std::string& remoteDir) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session");
    }

    int rc = sftp_rmdir(m_sftpSession.get(), remoteDir.c_str());
    if (rc < 0) {
        return SFTPError(
            ssh_get_error_code(m_sshSession.get()), sftp_get_error(m_sftpSession.get()),
            "Failed to remove remote dir [" + remoteDir + "] " + ssh_get_error(m_sshSession.get()));
    }

    return SFTPError();
}

std::pair<SFTPError, SFTPAttributes> SFTPClient::stat(const std::string& remotePath) const {
    if (!m_sftpSession.get() || !m_sshSession.get()) {
        return {SFTPError(SSH_ERROR, SSH_FX_FAILURE, "Invalid SFTP or SSH session"), {}};
    }

    sftp_attributes attr = sftp_lstat(m_sftpSession.get(), remotePath.c_str());
    if (!attr) {
        return {
            SFTPError(ssh_get_error_code(m_sshSession.get()), sftp_get_error(m_sftpSession.get()),
                      "Failed to stat remote dir [" + remotePath + "] " +
                          ssh_get_error(m_sshSession.get())),
            {}};
    }

    std::cout << "In stat() " << attr->name;

    return {SFTPError(), {attr}};
}

void SFTPClient::SSHSessionDeleter::operator()(ssh_session session) const {
    if (session) {
        ssh_disconnect(session);
        ssh_free(session);
    }
}

void SFTPClient::SFTPSessionDeleter::operator()(sftp_session session) const {
    if (session) {
        sftp_free(session);
    }
}

void SFTPClient::SFTPFileDeleter::operator()(sftp_file file) const {
    if (file) {
        sftp_close(file);
    }
}
