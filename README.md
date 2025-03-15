# sftppp
A C++11 OOP wrapper around the sftp api provided by libssh. 

I noticed there weren't any open source OOP C++ SFTP Clients, so I built my own.

It may be used as header only (see /single_header) or it may be built as a shared library. Header only requires linking libssh (-lssh). The shared library links libssh already.

To build the shared library: clone the repositoy and use CMake. In the root directory run the following commands,

cmake -B build && cmake --build build --config Release

The shared library will appear in /target

To compile the example cd /examples and run the compile command

g++ main.cpp -o main -lssh

Contributions are welcome. Star it if you like it, thanks.
