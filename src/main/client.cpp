//
// Created by sunnysab on 2021/6/29.
//

#include <iostream>
#include <filesystem.h>
#if defined(WIN32)
#include <winsock2.h>
#endif
#include "progress_bar.h"
#include "AntClient.h"

int main(int argc, char *argv[]) {
#if defined(WIN32)
    WSAData data{};
    WSAStartup(MAKEWORD(2, 2), &data);
#endif

    if (argc < 4) {
        printf("need input addr_info[127.0.0.1:16006] and src_file and dst_file!\n");
        return -1;
    }

    port_t port = 16006;
    std::string ip;
    std::string addr = argv[1];
    size_t mpos = addr.find_first_of(":");
    if (mpos != std::string::npos) {
        ip = addr.substr(0, mpos);
        port = atoi(addr.substr(mpos + 1).c_str());
    } else {
        ip = addr;
    }
    if (port == 0) port = 16006;

    std::string file_path = argv[2];
    size_t file_size = fs::file_size(file_path);

    bmc::progress_bar pb{
            0,  // min, also initial value
            file_size,  // max
            80,  // width in percent of screen (including percentage and time)
            true,  // auto increment when outputting
            true   // show time at the end
    };
    AntClient client(file_path, argv[3]);

    client.connect(ip, port);
    if (!client.try_send()) {
        std::cout << "Server refused the send request." << std::endl;
        return -1;
    }
#if 1
    client.send([&pb](TransferProcess process) -> bool {
        pb.set(process.completed_size);
        std::cout << pb;
        return true;
    });
    std::cout << pb;
#else
    client.send(nullptr);
#endif

    client.close();
#if defined(WIN32)
    WSACleanup();
#endif
    return 0;
}
