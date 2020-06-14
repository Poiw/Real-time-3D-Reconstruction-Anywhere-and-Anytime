#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>

// Time function, sockets, htons... file stat
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

// File function and bzero
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>

#include <mutex>
#include <vector>
#include <cstring>
#include <thread>
#include <fstream>

#include "dataStructure.hpp"

#define TRANSFER_PORT_NUM "12580"
#define TRANSFER_ERR_NETWORK -1
#define TRANSFER_ERR_PROTOCOL -2
#define TEST_SIZE 100000


// A simple protocol for transfer data over TCP
class Transfer
{
    public:
        /*
         * role 0, 1 => client, server
         */
        Transfer() {
            socket_id = -1;
        }

        Transfer(int _socket_id, int _role) {
            bind(_socket_id, _role);
        }
        
        void send_content(const void *buffer, size_t length);
        void *recv_content(void *buffer, size_t *buf_length);
        void bind(int _socket_id, int _role) {
            socket_id = _socket_id;
            role = _role;

            handshake();
#ifdef VERBOSE
            printf("Handshake pass!\n");
#endif
        }

    private:
        int socket_id;
        int role;
        int handshake();
        bool is_client() { return this -> role == 0; }
        bool is_server() { return this -> role == 1; }
        int send_raw(const void *, size_t, int);
        int recv_raw(void *, size_t, int);
        template<class T> void send_T(const T *);
        template<class T> void recv_T(T *);
};


class SocketClient
{
    public:
        SocketClient() {}
        static int open_clientfd(const char *hostname, const char *port);
};


// Listen a specified port;
class SocketServer
{
    public:
        SocketServer() {
            listen_fd = -1;
        }
        bool open_listenfd(const char *port);
        int accept_client();

    private:
        int listen_fd;
        std::vector<int> com_fd;
};
