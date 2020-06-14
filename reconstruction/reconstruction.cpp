#include "reconstruction.hpp"

ReconstructionServer::ReconstructionServer(SocketServer &socket_pool, int _type, float _voxelSize) : m_type(_type), 
m_recv(socket_pool.accept_client(), 1), m_send(socket_pool.accept_client(), 1)                                                                     
{
    m_process_image_mutex.lock();
    m_render_image_mutex.lock();
    m_start_sending_mutex.lock();
    m_sending_mutex.lock();
    m_sender = std::thread(&ReconstructionServer::m_sending_loop, this, &m_send);
    m_receiver = std::thread(&ReconstructionServer::m_receiving_loop, this, &m_recv);
    m_reconstructor = std::thread(&ReconstructionServer::m_reconstruction_loop, this, _voxelSize);
}

void ReconstructionServer::m_receiving_loop(Transfer *transfer) 
{
    size_t length;

    int tmp[4];
    length = sizeof(tmp);
    transfer -> recv_content(tmp, &length);
    assert(length == sizeof(tmp));
    m_camera = Camera(tmp[0], tmp[1], tmp[2], tmp[3]);

    length = sizeof(int);
    transfer -> recv_content(&m_rows, &length);
    assert(length == sizeof(int));

    length = sizeof(int);
    transfer -> recv_content(&m_cols, &length);
    assert(length == sizeof(int));

    m_start_sending_mutex.unlock();

    size_t rgbSize = m_cols * m_rows * 3, depthSize = m_cols * m_rows * 2, poseLength = 4*4*4, rgblength, depthlength;

    
    // std::cout << "rgbLength: " << rgbLength << "\n";
    // std::cout << "depthLength: " << depthLength << "\n";
    // std::cout << "poseLength: " << poseLength << "\n";

    void *rgb_buf = new uchar[rgbSize];
    void *depth_buf = new uchar[depthSize];
    void *pose_buf = new uchar[poseLength];

#ifdef VERBOSE
    std::cout << "parameter loading finished\n";
    
    std::cout << m_rows << " " << m_cols << "\n";
#endif
    m_curFrame.rgb = cv::Mat(m_rows, m_cols, CV_8UC3);
    m_curFrame.depth = cv::Mat(m_rows, m_cols, CV_16U);
    m_curFrame.pose = cv::Mat(4, 4, CV_32F);

    while (true) {
        // auto t_start = std::chrono::high_resolution_clock::now(); 

        rgblength = rgbSize;
        transfer -> recv_content(rgb_buf, &rgblength);

        auto t_start = std::chrono::high_resolution_clock::now(); 

        depthlength = depthSize;
        transfer -> recv_content(depth_buf, &depthlength);

        length = poseLength;
        transfer -> recv_content(pose_buf, &length);
        assert(length == poseLength);

        auto t_end = std::chrono::high_resolution_clock::now();

        std::cout << "[Receiving] Process time: " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << "ms\n";

        m_load_image_mutex.lock();

        ImageDecompression(m_curFrame.rgb, (uchar*)rgb_buf, rgblength, cv::IMREAD_COLOR);
        ImageDecompression(m_curFrame.depth, (uchar*)depth_buf, depthlength, cv::IMREAD_ANYDEPTH);
        // memcpy(m_curFrame.rgb.data, rgb_buf, rgbLength);
        // memcpy(m_curFrame.depth.data, depth_buf, depthLength);
        memcpy(m_curFrame.pose.data, pose_buf, poseLength);

        calib_color(m_curFrame.rgb);
        
#ifdef VERBOSE
        std::cout << "Receiving a set...\n";
#endif
        m_process_image_mutex.unlock();

    }
}

void ReconstructionServer::m_reconstruction_loop(float voxelSize)
{
    
    std::cout << "Start reconstruction...\n";
    m_curCloud.setvoxelSize(voxelSize);
    cv::Mat coordMap, smoothdepth, tempdepth;

    while(true) {
        m_process_image_mutex.lock();
#ifdef VERBOSE
        std::cout << "start processing a image\n";
#endif
        auto t_start = std::chrono::high_resolution_clock::now();
        m_curFrame.depth.convertTo(tempdepth, CV_32F);
        cv::bilateralFilter(tempdepth, smoothdepth, 10, 200, 5);
        generateCoordMap(smoothdepth, m_curFrame.pose, coordMap, std::make_shared<Camera>(m_camera));
        registration(m_curFrame.rgb, coordMap, m_curFrame.pose, &m_curCloud);
        auto t_end = std::chrono::high_resolution_clock::now();
        std::cout << "[Reconstruction] Process time: " <<  std::chrono::duration<double, std::milli>(t_end-t_start).count() << "ms\n";
#ifdef VERBOSE
        std::cout << "Current point cloud size: " << m_curCloud.Size() << "\n";
#endif
        m_render_image_mutex.unlock();
        m_load_image_mutex.unlock();
    }
}

// void ReconstructionServer::m_reconstruction_cl_loop(float voxelSize)
// {
    
//     std::cout << "Start reconstruction...\n";
//     m_curCloud.setvoxelSize(voxelSize);
//     cv::Mat coordMap;

//     cl_int    status;
//     /**Step 1: Getting platforms and choose an available one(first).*/
//     cl_platform_id platform;
//     getPlatform(platform);

//     /**Step 2:Query the platform and choose the first GPU device if has one.*/
//     cl_device_id *devices=getCl_device_id(platform);

//     /**Step 3: Create context.*/
//     cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);

//     /**Step 4: Creating command queue associate with the context.*/
//     cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

//     /**Step 5: Create program object */
//     const char *filename = "pointcloud_kernel.cl";
//     string sourceStr;
//     status = convertToString(filename, sourceStr);
//     const char *source = sourceStr.c_str();
//     size_t sourceSize[] = {strlen(source)};
//     cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);

//     /**Step 6: Build program. */
//     status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);

//     /**Step 7: Initial input,output for the host and create memory objects for the kernel*/
//     const int NUM=512000;
//     double* input = new double[NUM];
//     for(int i=0;i<NUM;i++)
//         input[i]=i;
//     double* output = new double[NUM];

//     cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, (NUM) * sizeof(double),(void *) input, NULL);
//     cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY , NUM * sizeof(double), NULL, NULL);

//     /**Step 8: Create kernel object */
//     cl_kernel kernel = clCreateKernel(program,"generateCoordMap", NULL);

//     /**Step 9: Sets Kernel arguments.*/
//     status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
//     status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputBuffer);

//     /**Step 10: Running the kernel.*/
//     size_t global_work_size[2] = {m_rows, m_cols};
//     cl_event enentPoint;
//     status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, &enentPoint);
//     clWaitForEvents(1,&enentPoint); ///wait
//     clReleaseEvent(enentPoint);

//     /**Step 11: Read the cout put back to host memory.*/
//     status = clEnqueueReadBuffer(commandQueue, outputBuffer, CL_TRUE, 0, NUM * sizeof(double), output, 0, NULL, NULL);
//     cout<<output[NUM-1]<<endl;

//     /**Step 12: Clean the resources.*/
//     status = clReleaseKernel(kernel);//*Release kernel.
//     status = clReleaseProgram(program);    //Release the program object.
//     status = clReleaseMemObject(inputBuffer);//Release mem object.
//     status = clReleaseMemObject(outputBuffer);
//     status = clReleaseCommandQueue(commandQueue);//Release  Command queue.
//     status = clReleaseContext(context);//Release context.

//     if (output != NULL)
//     {
//         free(output);
//         output = NULL;
//     }

//     if (devices != NULL)
//     {
//         free(devices);
//         devices = NULL;
//     }

//     // while(true) {
//     //     m_process_image_mutex.lock();
//     //     std::cout << "start processing a image\n";
//     //     auto t_start = std::chrono::high_resolution_clock::now();
//     //     generateCoordMap(m_curFrame.depth, m_curFrame.pose, coordMap, std::make_shared<Camera>(m_camera));
//     //     registration(m_curFrame.rgb, coordMap, m_curFrame.pose, &m_curCloud);
//     //     auto t_end = std::chrono::high_resolution_clock::now();
//     //     std::cout << "[Reconstruction] Process time: " <<  std::chrono::duration<double, std::milli>(t_end-t_start).count() << "ms\n";
//     //     std::cout << "Current point cloud size: " << m_curCloud.Size() << "\n";
//     //     m_render_image_mutex.unlock();
//     //     m_load_image_mutex.unlock();
//     // }
// }

void ReconstructionServer::m_rendering(cv::Mat &showImg)
{
    //render -> getImg(showImg);
}

void ReconstructionServer::m_rendering_loop()   
{
    cv::Mat renderImg;
    while(true) {
        m_render_image_mutex.lock();
        m_rendering(renderImg);
        m_send_end_mutex.lock();
        showImg = renderImg;
        m_sending_mutex.unlock();
    }
}

void ReconstructionServer::m_sending_loop(Transfer * transfer)
{
    m_start_sending_mutex.lock();
    // std::cout << "Start sending...\n";
    // std::cout << m_rows << " " << m_cols << "\n";
    //cv::Mat showImg(m_rows, m_cols, CV_8UC3);
    size_t rgbLength = m_rows * m_cols * 3;
    uchar *color_img_data;

    // int cnt = 0;
    while(true) {
        // cnt++;

        // if (cnt == 800) {
        //     std::ofstream fout;
        //     fout.open("pcloud.xyz");
        //     std::map<PointIndex, SinglePoint>::iterator it, begin = m_curCloud.m_begin(), end = m_curCloud.m_end();
        //     for (it = begin; it != end; ++it) {
        //         cv::Vec3f coord = m_curCloud.calCoord(it->first);
        //         fout << coord[0] << " " << coord[1] << " " << coord[2] << " "
        //                 << it->second.m_getColor()[0] << " " << it->second.m_getColor()[1] << " " << it->second.m_getColor()[2] << "\n";
        //     }
        //     fout.close();
        // }
        m_sending_mutex.lock();
        rgbLength = ImageJPEGCompression(showImg, color_img_data);
        // printf("Sending %d bytes\n", rgbLength);
        transfer -> send_content(color_img_data, rgbLength);
        delete [] color_img_data;
        m_send_end_mutex.unlock();
    }
}

ReconstructionServer::~ReconstructionServer()
{   
    m_sender.join();
    m_receiver.join();
    m_reconstructor.join();
}

// calib color to depth
void ReconstructionServer::calib_color(cv::Mat &raw)
{
    int image_rows = raw.rows, image_cols = raw.cols;
    float scale = 1 / 0.89;
    cv::Mat result(image_rows, image_cols, CV_8UC3);
    int height = image_rows;
    int width = image_cols;
    int h_new = height * scale;
    int w_new = width * scale;
    cv::Size size(w_new, h_new);
    int dr = (h_new - height) / 2;
    int dc = (w_new - width) / 2;
    // resize
    cv::Mat color_resized;
    cv::resize(raw, color_resized, size, cv::INTER_AREA);
    // shift

    for (int r = 0; r < image_rows; r++)
    {
        for (int c = 0; c < image_cols; c++)
        {
            result.ptr<cv::Vec3b>(r)[c] = color_resized.ptr<cv::Vec3b>(r + dr)[c + dc];
        }
    }
    raw = result;
}