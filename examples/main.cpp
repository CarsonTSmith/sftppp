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
        ;
    }

    auto pair = client.ls("/my/remote/dir");
    if (ret.isOk()) {
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
        ;
    }

    client.disconnect(); // RAII will take care of this in the destructor, but it shows intent.
    return 0;
}