#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>

// File function and bzero
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <mutex>
#include <vector>
#include <cstring>
#include <thread>
#include <fstream>

#include "common/dataStructure.hpp"
#include "common/transfer.hpp"

class RGBDPClient
{
    private:
        Transfer m_send;
        Transfer m_recv;

        std::thread m_sender;
        std::thread m_receiver;

        std::string m_data_path;
        int m_endIndex;

        std::mutex m_sending_mutex;
        std::mutex m_start_receiving_mutex;

        Camera m_camera;

        int m_rows, m_cols;

        std::string m_windowsName;
        cv::Mat m_showImg;
        std::mutex m_write_img_mutex;
        std::mutex m_show_img_mutex;

        void m_sending_loop(Transfer *transfer);
        void m_receiving_loop(Transfer *transfer);
        void m_7scenes_dataLoading(const int index, cv::Mat &color_img, cv::Mat &depth_img, cv::Mat &pose);
        void m_image_show_loop();

    public:
        RGBDPClient(const char* address, const char* _port, const std::string& _datapath, int _endIndex, const Camera& _camera);
        ~RGBDPClient();
};