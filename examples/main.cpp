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

int main(int argc, char **argv)
{
    SFTPClient client;
    SFTPError ret = client.connect("127.0.0.1", "username", "password");
    if (ret.isOk()) {
        std::cout << "Connected!" << std::endl;
    }

    ret = client.mkdir("test", 0777);
    if (ret.isOk()) {
        std::cout << "Remote directory created!" << std::endl;
    } else {
        std::cout << "Failed to create remote directory: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl
        ;
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
                  << std::endl
        ;
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
                  << std::endl
        ;
    }

    ret = client.rm("/my/remote/file.txt");
    if (ret.isOk()) {
        std::cout << "Remote file deleted!" << std::endl;
    } else {
        std::cout << "Failed to upload file: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl
        ;
    }

    auto pair = client.ls("/my/remote/dir");
    if (pair.first.isOk()) {
        std::cout << "Listing remote directory!" << std::endl;
        for (const auto &item: pair.second) {
            std::cout << "File name: " << item.get()->name << std::endl;
            std::cout << "File size: " << item.get()->size << std::endl;
            std::cout << "File permissions: " << item.get()->permissions << std::endl;
            std::cout << "File owner: " << item.get()->owner << std::endl;
            std::cout << "File uid: " << item.get()->uid << std::endl;
            std::cout << "File group: " << item.get()->group << std::endl;
            std::cout << "File gid: " << item.get()->gid << std::endl;

            std::cout << std::endl;
            std::cout << std::endl;
        }
    } else {
        std::cout << "Failed to upload file: " 
                  << ret.getSSHErrorCode() 
                  << " " 
                  << ret.getSFTPErrorCode()
                  << " "
                  << ret.getSSHErrorMsg()
                  << std::endl
        ;
    }

    client.disconnect(); // RAII will take care of this in the destructor, but it shows intent.
    return 0;
}