#include "ImageClient.hpp"

// RGBDPClient 

auto t_begin = std::chrono::high_resolution_clock::now();
auto t_end = std::chrono::high_resolution_clock::now();

void RGBDPClient::m_image_show_loop()
{
    // cv::namedWindow("depth");
    // cv::namedWindow("rgb");
    // cv::namedWindow("rendering");

    while(true) {
        m_show_img_mutex.lock();
        // cv::imshow(m_windowsName, m_showImg);
        m_write_img_mutex.unlock();
        // cv::waitKey(1);
    }
}

void RGBDPClient::m_sending_loop(Transfer *transfer)
{
    // std::cout << "Start sending...\n";
    cv::Mat color_img, depth_img, pose, bgr_img;
    size_t length;
    size_t rgbLength, depthLength, poseLength = 4*4*4;

    // std::cout << "Start sending 2 ...\n";

    //cv::namedWindow("depth");
    //cv::namedWindow("rgb");

    for (int idx = 0; idx < m_endIndex; idx++)
    {
#ifdef VERBOSE
        printf("Sending frame %d\n", idx);
#endif
        m_7scenes_dataLoading(idx, color_img, depth_img, pose);
        if (idx == 0) {
#ifdef VERBOSE
            printf("Send initial data\n");
#endif
            int a[] = {m_camera.m_fx, m_camera.m_fy, m_camera.m_tx, m_camera.m_ty};
            length = sizeof(a);
#ifdef VERBOSE
            printf("%zu\n", length);
#endif
            transfer -> send_content(a, length);

            m_rows = color_img.rows;
            m_cols = color_img.cols;

            m_start_receiving_mutex.unlock();

            length = sizeof(int);
            transfer -> send_content(&m_rows, length);
            transfer -> send_content(&m_cols, length);

            // rgbLength = m_rows * m_cols * 3;
            // depthLength = m_rows * m_cols * 2;
        }

        cv::cvtColor(color_img, bgr_img, cv::COLOR_RGB2BGR);
        m_write_img_mutex.lock();
        m_windowsName = "rgb";
        m_showImg = bgr_img;
        m_show_img_mutex.unlock();

        m_write_img_mutex.lock();
        m_windowsName = "depth";
        m_showImg = depth_img;
        m_show_img_mutex.unlock();
        //cv::imshow("rgb", bgr_img);
        //cv::imshow("depth", depth_img);
        //cv::waitKey(1);

        t_begin = std::chrono::high_resolution_clock::now();

        uchar *color_img_data;
        uchar *depth_img_data;

        rgbLength = ImageJPEGCompression(color_img, color_img_data);
        depthLength = ImagePNGCompression(depth_img, depth_img_data);
   
        m_sending_mutex.lock();
#ifdef VERBOSE
        std::cout << "rgbLength: " << rgbLength << "\n";
        std::cout << "depthLength: " << depthLength << "\n";
        std::cout << "poseLength: " << poseLength << "\n";
#endif
        transfer -> send_content(color_img_data, rgbLength);
        transfer -> send_content(depth_img_data, depthLength);
        transfer -> send_content(pose.data, poseLength);

        delete [] color_img_data;
        delete [] depth_img_data;
    }
}

void RGBDPClient::m_receiving_loop(Transfer *transfer)
{
    m_start_receiving_mutex.lock();
#ifdef VERBOSE
    std::cout << "Start receiving..." << std::endl;
#endif
    cv::Mat showImg(m_rows, m_cols, CV_8UC3), bgr_img;

    int imageSize = m_cols * m_rows * 3;
    size_t length;
    void *buf = new uchar[imageSize];

    //cv::namedWindow("Rendering");

    int i = 0;
    while (true) {
        length = imageSize;
        transfer -> recv_content(buf, &length);
        // assert(length == imageSize);
        // std::cout << "Receiving a image" << std::endl;
        // memcpy(showImg.data, buf, imageSize);

        ImageDecompression(showImg, (uchar*)buf, length, cv::IMREAD_COLOR);

        cv::cvtColor(showImg, bgr_img, cv::COLOR_RGB2BGR);
        std::string file_name = std::string("./") + std::to_string(i) + std::string(".png");

        t_end = std::chrono::high_resolution_clock::now();

        std::cout << "[RTT] Process time: " << std::chrono::duration<double, std::milli>(t_end-t_begin).count() << "ms\n";

        //std::cout << bgr_img << std::endl;
        //cv::imwrite(file_name.c_str(), bgr_img);
        //cv::imshow("Rendering", bgr_img);
        //cv::waitKey(1);
        m_write_img_mutex.lock();
        m_windowsName = "rendering";
        m_showImg = bgr_img;
        m_show_img_mutex.unlock();

        m_sending_mutex.unlock();
        i++;
    }
}

void RGBDPClient::m_7scenes_dataLoading(const int index, cv::Mat &color_img, cv::Mat &depth_img, cv::Mat &pose)
{
    char buf[10];
    sprintf(buf, "%06d", index);
    std::string _index(buf);
    std::string path(m_data_path + "/frame-" + _index);

    std::string color_img_path = path + ".color.png";
    std::string depth_img_path = path + ".depth.png";
    std::string pose_path = path + ".pose.txt";

#ifdef VERBOSE
    std::cout << color_img_path << std::endl;
    std::cout << depth_img_path << std::endl;
    std::cout << pose_path << std::endl;
#endif

    color_img = cv::imread(color_img_path);
    cv::cvtColor(color_img, color_img, cv::COLOR_RGB2BGR);

#ifdef VERBOSE
    std::cout << "loading rgb finished\n";
#endif

    depth_img = cv::imread(depth_img_path, cv::IMREAD_ANYDEPTH);

#ifdef VERBOSE
    std::cout << "loading depth finished\n";
#endif

    std::ifstream posefile;
    posefile.open(pose_path);
    pose = cv::Mat(4, 4, CV_32F);
    for (int i = 0; i < 4; i++) 
        for (int j = 0; j < 4; j++) 
            posefile >> pose.at<float>(i, j);
    posefile.close();   

#ifdef VERBOSE
    std::cout << "loading pose finished\n";
#endif
}

RGBDPClient::RGBDPClient(const char* address, const char* _port, const std::string& _datapath, int _endIndex, const Camera& _camera)
                 : m_data_path(_datapath), m_endIndex(_endIndex), m_camera(_camera)
{
    m_send = Transfer(SocketClient::open_clientfd(address, _port), 0);
    m_recv = Transfer(SocketClient::open_clientfd(address, _port), 0);
    std::cout << "Start multi threads\n";
    m_start_receiving_mutex.lock();
    m_show_img_mutex.lock();
    m_sender = std::thread(&RGBDPClient::m_sending_loop, this, &m_send);
    m_receiver = std::thread(&RGBDPClient::m_receiving_loop, this, &m_recv);
    m_image_show_loop();
}

RGBDPClient::~RGBDPClient()
{
    m_sender.join();
    m_receiver.join();
}