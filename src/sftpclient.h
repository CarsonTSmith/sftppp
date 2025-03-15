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
#define O_RDONLY _O_RDONLY  // Write permission for user
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

#endif /* SFTP_CLIENT_H */