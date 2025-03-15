#ifndef SFTP_ATTRIBUTE_H
#define SFTP_ATTRIBUTE_H

#include <libssh/sftp.h>  // sftp_attribute

#include <memory>

class SFTPAttributes {
   public:
    using SFTPAttributesPtr = std::shared_ptr<sftp_attributes_struct>;

    SFTPAttributes(sftp_attributes attr) : m_sftpAttributes(attr, SFTPAttributesDeleter()) {}
    ~SFTPAttributes() = default;

    SFTPAttributes(const SFTPAttributes& other) = default;
    SFTPAttributes(SFTPAttributes&& other) = default;

    SFTPAttributes& operator=(const SFTPAttributes& other) = default;
    SFTPAttributes& operator=(SFTPAttributes&& other) = default;

    sftp_attributes get() const { return m_sftpAttributes.get(); }

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

#endif /* SFTP_ATTRIBUTE_H */