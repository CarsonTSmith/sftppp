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

#include "../single_header/sftpclientpp.h"

#include <iostream>

int main(int argc, char **argv) {
    SFTPClient client;
    SFTPError ret = client.connect("127.0.0.1", "username", "password");
    if (ret.isOk()) {
        std::cout << "Connected!" << std::endl;
    } else {
        std::cout << "Failed to connect: "
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    ret = client.mkdir("/my/remote/dir", 0777);
    if (ret.isOk()) {
        std::cout << "Remote directory created!" << std::endl;
    } else {
        std::cout << "Failed to create remote directory: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    ret = client.put("/my/local/file.txt", "/my/remote/file.txt");
    if (ret.isOk()) {
        std::cout << "File uploaded!" << std::endl;
    } else {
        std::cout << "Failed to create remote directory: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    auto statPair = client.stat("/my/remote/file.txt");
    if (statPair.first.isOk()) {
        std::cout << "Stat successful!" << std::endl;

        const auto &attr = statPair.second;

        // name, group, owner fields are not included and won't work with sftp_stat()
        std::cout << "size: " << attr.get()->size << std::endl;
        std::cout << "permissions: " << attr.get()->permissions << std::endl;
        std::cout << "uid: " << attr.get()->uid << std::endl;
        std::cout << "gid: " << attr.get()->gid << std::endl;
        std::cout << std::endl;
    } else {
        std::cout << "Failed to stat remote path: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    ret = client.get("/my/local/file.txt", "/my/remote/file.txt");
    if (ret.isOk()) {
        std::cout << "File downloaded!" << std::endl;
    } else {
        std::cout << "Failed to download file: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    ret = client.rm("/my/remote/file.txt");
    if (ret.isOk()) {
        std::cout << "Remote file deleted!" << std::endl;
    } else {
        std::cout << "Failed to remove file: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    auto lsPair = client.ls("/my/remote/dir");
    if (lsPair.first.isOk()) {
        std::cout << "Listing remote directory!" << std::endl;
        for (const auto &item: lsPair.second) {
            std::cout << "name: " << item.get()->name << std::endl;
            std::cout << "size: " << item.get()->size << std::endl;
            std::cout << "permissions: " << item.get()->permissions << std::endl;
            std::cout << "owner: " << item.get()->owner << std::endl;
            std::cout << "uid: " << item.get()->uid << std::endl;
            std::cout << "group: " << item.get()->group << std::endl;
            std::cout << "gid: " << item.get()->gid << std::endl;

            std::cout << std::endl;
        }
    } else {
        std::cout << "Failed to list remote directory: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl;
    }

    client.disconnect(); // RAII will take care of this in the destructor, but it shows intent.
    return 0;
}