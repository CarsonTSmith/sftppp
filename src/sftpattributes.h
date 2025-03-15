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

#ifndef SFTP_ATTRIBUTE_H
#define SFTP_ATTRIBUTE_H

#include <libssh/sftp.h>  // sftp_attribute

#include <memory>

class SFTPAttributes {
   public:
    using SFTPAttributesPtr = std::shared_ptr<sftp_attributes_struct>;

    SFTPAttributes(sftp_attributes attr) : m_sftpAttributes(attr, SFTPAttributesDeleter()) {}
    SFTPAttributes() = default;
    ~SFTPAttributes() = default;

    SFTPAttributes(const SFTPAttributes& other) = default;
    SFTPAttributes(SFTPAttributes&& other) = default;

    SFTPAttributes& operator=(const SFTPAttributes& other) = default;
    SFTPAttributes& operator=(SFTPAttributes&& other) = default;

    const SFTPAttributesPtr& get() const { return m_sftpAttributes; }

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