#include "sftpclient.h"

#include <algorithm>  // min

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
