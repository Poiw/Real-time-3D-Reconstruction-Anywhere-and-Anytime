#include "common/transfer.hpp"
#include <cstdlib>

#define HANDSHAKE_MAGICNUMBER 19260817
#define LISTENQ_SIZE 10
#define MAXLINE 1000

#define CHUNK_SIZE 8192

int Transfer::handshake()
{
    int content;
    if (is_client()) {
        content = HANDSHAKE_MAGICNUMBER;
        send_T(&content);
        recv_T(&content);
        if (content != HANDSHAKE_MAGICNUMBER * 2) {
            printf("NOT a VALID Transfer server.\n");
            throw TRANSFER_ERR_PROTOCOL;
        }
    } else {
        recv_T(&content);
        if (content != HANDSHAKE_MAGICNUMBER) {
            printf("NOT a VALID Transfer client.\n");
            throw TRANSFER_ERR_PROTOCOL;
        }
        content *= 2;
        send_T(&content);
    }
    return 0;
}

void Transfer::send_content(const void *buff, size_t length)
{
    send_T(&length);
    send_raw(buff, length, 0);
#ifdef VERBOSE
    std::cout << "Send size " << length << "\n";
#endif
}

// if buffer is NULL, allocate new memory
// if buffer is not NULL, buf_length should represent the size of buffer
// if buffer is not NULL and the buffer is long enough, store results in buffer
// if buffer is not NULL and the buffer is not long enough, allocate new memory
// this function returns the buffer address
// received length is stored in buf_length
void *Transfer::recv_content(void *buffer, size_t *buf_length)
{
    size_t length;
    recv_T(&length);
#ifdef VERBOSE
    std::cout << "Recv size " << length << "\n";
#endif
    if (buffer == NULL || *buf_length < length) {
#ifdef VERBOSE
        std::cout << *buf_length << " " <<  length << "\n";
#endif
        buffer = new char[length];
    }
    recv_raw(buffer, length, 0);
    *buf_length = length;
    return buffer;
}

// Unprotocoled sender; wrapped send function
template<class T>
void Transfer::send_T(const T *content)
{
    int r = this -> send_raw((const void *)content, sizeof(T), 0);
    if (r != sizeof(T)) {
        printf("Network Error\n");
        throw TRANSFER_ERR_NETWORK;
    }
}

// Unprotocoled receiver; wrapped recv function
template<class T>
void Transfer::recv_T(T *content)
{
    int r = this -> recv_raw((void *)content, sizeof(T), 0);
    if (r != sizeof(T)) {
        printf("Network Error\n");
        throw TRANSFER_ERR_NETWORK;
    }
}

#define void_shift(p, i) ((void *)(((char *)(p)) + (i)))

int Transfer::send_raw(const void *buffer, size_t length, int flags)
{
    int res = 0;
    /*
    for(int i = 0; i < length; i += CHUNK_SIZE) {
        if (i + CHUNK_SIZE < length) res += send(this -> socket_id, void_shift(buffer, i), CHUNK_SIZE, flags);
        else res += send(this -> socket_id, void_shift(buffer, i), length - i, flags);
    }
    */
    return send(this -> socket_id, buffer, length, flags);
    // return res;
}

int Transfer::recv_raw(void *buffer, size_t length, int flags)
{
    int res = 0;
    flags |= MSG_WAITALL;
    /*
    for(int i = 0; i < length; i += CHUNK_SIZE) {
        if (i + CHUNK_SIZE < length) res += recv(this -> socket_id, void_shift(buffer, i), CHUNK_SIZE, flags);
        else res += recv(this -> socket_id, void_shift(buffer, i), length - i, flags);
    }
    */
    return recv(this -> socket_id, buffer, length, flags);
    // return res;
}

static void set_socket_buffer_size(int socket_fd)
{
    int size;
    socklen_t len;
    size = 1000000;
    //setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (void *)&size, sizeof(size));
    getsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (void *)&size, &len);
#ifdef VERBOSE
    printf("Socket with recv buffer size %d\n", size);
#endif
    //setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (void *)&size, sizeof(size));
    getsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (void *)&size, &len);
#ifdef VERBOSE
    printf("Socket with send buffer size %d\n", size);
#endif
}


int SocketClient::open_clientfd(const char *hostname, const char *port)
{
    int clientfd;
    addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    getaddrinfo(hostname, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next) {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        
        close(clientfd);
    }

    freeaddrinfo(listp);
    if (!p) return -1;

    set_socket_buffer_size(clientfd);

    return clientfd;
}


bool SocketServer::open_listenfd(const char *port)
{
    addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p) return false;

    if (listen(listenfd, LISTENQ_SIZE) < 0) {
        close(listenfd);
        return false;
    }

    this -> listen_fd = listenfd;
    return true;
}

int SocketServer::accept_client()
{
    if (listen_fd == 1) return -1;
    socklen_t clientlen;
    sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    int comfd;
    clientlen = sizeof(sockaddr_storage);
    comfd = accept(listen_fd, (sockaddr *)&clientaddr, &clientlen);
    getnameinfo((sockaddr*) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
#ifdef VERBOSE
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
#endif
    com_fd.push_back(comfd);
    set_socket_buffer_size(comfd);
    return comfd;
}
