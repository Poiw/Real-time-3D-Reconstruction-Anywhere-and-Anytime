#include "common/transfer.hpp"
#include "ImageClient.hpp"
#include <sys/socket.h>
#include <netdb.h>

#include <cstdio>
#include <cstring>

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>




int main()
{
    Camera camera(585, 585, 320, 240);
    std::string data_path("/Users/lipei/Downloads/seq-01/seq-01");

    RGBDPClient *rgbdp_client = new RGBDPClient("localhost", TRANSFER_PORT_NUM, data_path, 1000, camera);

    delete rgbdp_client;

    return 0;
}
