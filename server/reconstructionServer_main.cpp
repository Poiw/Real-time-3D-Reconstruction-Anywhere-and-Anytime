#include "common/transfer.hpp"
#include "reconstruction/reconstruction.hpp"
#include "rendering/render.hpp"

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
    SocketServer socket_pool;
    socket_pool.open_listenfd(TRANSFER_PORT_NUM);

    auto *render = new Render(640, 480);
    ReconstructionServer *reconstruction_server = new ReconstructionServer(socket_pool, 0, 0.001);
    render -> set_ReconstructionServer(reconstruction_server);
    render -> render_loop();

    delete reconstruction_server;

    return 0;
}