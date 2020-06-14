#pragma once

#include "pointcloud.hpp"
#include "common/transfer.hpp"
#include "opencl/tool.hpp"
#include <thread>
#include <mutex>
#include <iostream>
#include "opencl/opencl_header.h"

class ReconstructionServer
{
    private:
        int m_type;
        Transfer m_recv;
        Transfer m_send;
        std::thread m_sender;
        std::thread m_receiver;
        std::thread m_reconstructor;

        std::mutex m_load_image_mutex;
        std::mutex m_process_image_mutex;
        
        std::mutex m_start_sending_mutex;
        

        RGBDP m_curFrame;
        Camera m_camera;

        int m_cols, m_rows;

        void m_sending_loop(Transfer *transfer);
        void m_receiving_loop(Transfer *transfer);
        void m_reconstruction_loop(float voxelSize);
        // void m_reconstruction_cl_loop(float voxelSize);
        void m_rendering_loop();
        void m_rendering(cv::Mat &showImg);
        void calib_color(cv::Mat &raw);

    public:
        ReconstructionServer(SocketServer &socket_pool, int _type, float _voxelSize);
        ~ReconstructionServer();
        cv::Mat showImg;
        PointCloud m_curCloud;
        std::mutex m_render_image_mutex;
        std::mutex m_send_end_mutex;
        std::mutex m_sending_mutex;
};
