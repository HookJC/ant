//
// Created by sunnysab on 2021/6/28.
//

#include <iostream>
#include <filesystem.h>
#if defined(WIN32)
#include <winsock2.h>
#endif
#if defined(PROGRESS)
#include <progress_bar.h>
#endif
#include "AntServer.h"


int main(int argc, char *argv[]) {
#if defined(WIN32)
    WSAData data{};
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
    AntServer server;

    port_t port = 16006;
    if (argc >= 2) port = atoi(argv[1]);
    if (port == 0) port = 16006;

    server.listen("0.0.0.0", port);
    printf("server listen at %d\n", port);

    do {
        auto request = server.wait();

        std::cout << "Received sender request: " << std::endl;
        std::cout << "File name: " << request.file_name << std::endl;
        std::cout << "File size: " << request.file_size << std::endl;
        std::cout << "File id: " << request.file_id << std::endl;

    #if defined(CONFIRM)
        std::cout << "Accept or not? [Y/N]: ";
        char opt = getchar();
        if (opt != 'Y' && opt != 'y') {
            std::cout << "User cancelled.";
            server.decline();
            return 0;
        }
    #endif

        size_t rpos = request.file_name.find_last_of("\\/");
        if (rpos == std::string::npos || fs::is_directory(request.file_name)) {
            if (!fs::exists(request.file_name)){
                fs::create_directories(request.file_name);
            }
            request.file_name += "/tempfile";
        } else {
            std::string dir = request.file_name.substr(0, rpos);
            if (!fs::exists(dir)){
                fs::create_directories(dir);
            }
        }
        if (fs::exists(request.file_name)) {
            fs::remove(request.file_name);
        }
        server.fopen(request.file_name);

#if defined(PROGRESS)
        bmc::progress_bar pb{
                0,  // min, also initial value
                request.file_size,  // max
                80,  // width in percent of screen (including percentage and time)
                true,  // auto increment when outputting
                true   // show time at the end
        };
        server.accept();
        server.write([&pb](TransferProcess process) -> bool {
            pb.set(process.completed_size);
            std::cout << pb;
            return true;
        });
        std::cout << pb;
#else
        server.accept();
        server.write(
#if 1
            [](TransferProcess process) -> bool {
                fprintf(stdout, "\r[%zu/%zu] %d%%", process.completed_size, process.total_size, process.completed_size*100/process.total_size);
                fflush(stdout);
                if (process.completed_size >= process.total_size) {
                    printf("\n");
                }
            return true;
        }
#else
            nullptr
#endif
        );
#endif
        server.fclose();
        std::cout << "Transfer finished." << std::endl;

    }while(true);

#if defined(WIN32)
    WSACleanup();
#endif
    return 0;
}
