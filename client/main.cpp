// This is a file for testing our transfer protocol

#include "common/transfer.hpp"

#include <sys/socket.h>
#include <netdb.h>

#include <cstdio>
#include <cstring>

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>


void send_loop(Transfer *transfer)
{
    char buf[TEST_SIZE];
    time_t s_time;
    memset(buf, 0, sizeof(buf));

    for (int i = 0; i < 256; i++) {
        buf[TEST_SIZE-1] = i % 256;
        transfer -> send_content(buf, TEST_SIZE);
        s_time = rand() % 1000;
        s_time = 100000;
        printf("Send to server, sleep %ld\n", s_time);
        usleep(s_time);
    }
}
/*
void recv_loop(Transfer *transfer)
{
    char buf[TEST_SIZE];
    void *res;
    size_t length = TEST_SIZE;
    time_t s_time;
    
    for (int i = 0; ; i++) {
        res = transfer -> recv_content((void *)buf, &length);
        s_time = rand() % 1000;
        s_time = 1000000;
        printf("Received %ld bytes from server, sleep %ld\n", length, s_time);
        usleep(s_time);
    }
}
*/
int main()
{
    Transfer send(SocketClient::open_clientfd("localhost", TRANSFER_PORT_NUM), 0);
    Transfer recv(SocketClient::open_clientfd("localhost", TRANSFER_PORT_NUM), 0);

    sleep(10);

    std::thread sender(send_loop, &send);
    //std::thread receiver(recv_loop, &recv);
    
    sender.join();
    //receiver.join();

    return 0;
}
