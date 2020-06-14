#include "pointcloud.hpp"
#include <iostream>

const int SampleStep = 4;

// void generateCoordMapCL(const cv::Mat &depth_img, const cv::Mat &pose, cv::Mat &coord_map, std::shared_ptr<Camera> camera)
// {
    
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

//     /**Step 12: Clean the resources.*/
//     status = clReleaseKernel(kernel);//*Release kernel.
//     status = clReleaseProgram(program);    //Release the program object.
//     status = clReleaseMemObject(inputBuffer);//Release mem object.
//     status = clReleaseMemObject(outputBuffer);
//     status = clReleaseCommandQueue(commandQueue);//Release  Command queue.
//     status = clReleaseContext(context);//Release context.
// }

void generateCoordMap(const cv::Mat &depth_img, const cv::Mat &pose, cv::Mat &coord_map, std::shared_ptr<Camera> camera)
{
    coord_map = cv::Mat(depth_img.rows, depth_img.cols, CV_32FC3);

    for (int r = 0; r < depth_img.rows; r += SampleStep) {
        // const u_int16_t* depth_rows = depth_img.ptr<u_int16_t>(r);
        const float* depth_rows = depth_img.ptr<float>(r);
        cv::Vec3f* coord_rows = coord_map.ptr<cv::Vec3f>(r);

        #pragma omp parallel for
        for (int c = 0; c < depth_img.cols; c += SampleStep) {

            float depth = depth_rows[c];

            if(depth < 0.5 || depth > 65534.5) {
                coord_rows[c] = cv::Vec3f(0, 0, 0);
            }
            else {

                // convert depth to meters
                float z = 1.0 * depth / 1000;
                cv::Mat v = (cv::Mat_<float>(4, 1) << 1.0 * (c - camera->m_tx) * z / camera->m_fx,
                                                        1.0 * (r - camera->m_ty) * z / camera->m_fy,
                                                        z,
                                                        1);
                                                
                cv::Mat gv = pose * v;
                coord_rows[c] = cv::Vec3f(gv.at<float>(0,0), gv.at<float>(0,1), gv.at<float>(0,2));
            }
            
        }
    }
}

bool checkPoint(const cv::Vec3f &coord)
{
    double eps = 1e-15;
    return !(fabs(coord[0]) < eps && fabs(coord[1]) < eps && fabs(coord[2]) < eps);
}

void registration(const cv::Mat &color_img, const cv::Mat &coord_map, const cv::Mat &pose, PointCloud* cloud)
{
    PointCloud *tmpCloud = new PointCloud(cloud->getvoxelSize());

#ifdef VERBOSE
    std::cout << "Start Registration\n";
#endif
    int Rows = color_img.rows, Cols = color_img.cols;
    int rowStep = Rows / 8, colStep = Cols / 8;

    std::queue<int> qx, qy;

    bool *vis = new bool[Rows * Cols + 5];
    memset(vis, 0, sizeof(bool) * (Rows * Cols + 5));

    for (int r = 0; r < Rows; r += rowStep) 
        for (int c = 0; c < Cols; c += colStep) {
            cv::Vec3f coord = coord_map.at<cv::Vec3f>(r, c);
            if ( checkPoint(coord) && (!(cloud->isExists(coord))) ) {
                qx.push(r);
                qy.push(c);
                vis[r * Cols + c] = 1;
                if (!(tmpCloud->isExists(coord))) {
                    tmpCloud->addPoint(coord, SinglePoint(color_img.at<cv::Vec3b>(r, c)));
                    // tmpCloud->addPoint(coord, SinglePoint(cv::Vec3b(128, 128, 128)));
                }
            }
        }

    int dx[] = {0, 0, -SampleStep, SampleStep };
    int dy[] = {SampleStep, -SampleStep, 0, 0};

    while(!qx.empty()) {
        int r = qx.front(), c = qy.front();
        qx.pop(); qy.pop();

        #pragma omp parallel for
        for (int k = 0; k < 4; k++) {
            int rr = r + dx[k], cc = c + dy[k];
            if (rr < 0 || rr >= Rows || cc < 0 || cc >= Cols) continue;
            cv::Vec3f coord = coord_map.at<cv::Vec3f>(rr, cc);

            int index = rr * Cols + cc;

            if ( checkPoint(coord) && (!(cloud->isExists(coord)))) {
                #pragma omp critical 
                {
                    if(!vis[index]) 
                    {
                        vis[index] = 1;
                        qx.push(rr);
                        qy.push(cc);
                        if (!(tmpCloud->isExists(coord))) {
                            tmpCloud->addPoint(coord, SinglePoint(color_img.at<cv::Vec3b>(rr, cc)));
                            // tmpCloud->addPoint(coord, SinglePoint(cv::Vec3b(128, 128, 128)));
                        }
                    }
                }
            }
        }
    }

    cloud->m_lock();
    cloud->merge(tmpCloud);
    cloud->setIncrementCloud(tmpCloud->getClouds());
    cloud->setPose(pose);
    cloud->m_unclock();

    delete tmpCloud;
    delete [] vis;
    
}