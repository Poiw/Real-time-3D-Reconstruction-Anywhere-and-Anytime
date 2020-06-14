#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp> 
#include<opencv2/opencv.hpp>   
#include<opencv2/highgui/highgui.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <mutex>

class Camera
{
    public:

    int m_fx, m_fy;
    int m_tx, m_ty;
    cv::Mat m_cameraIntrinsic;


    Camera(int _fx, int _fy, int _tx, int _ty);
    Camera() {}

    void reModel();
};


class SinglePoint
{
    private:
        cv::Vec3b m_rgb;
        int m_count;

    public:
        SinglePoint() {}
        SinglePoint(const cv::Vec3b &color);

        void merge(const SinglePoint b);

        cv::Vec3b m_getColor() const;

};

class PointIndex
{
    public:
        int x, y, z;
        PointIndex () {}
        PointIndex (int _x, int _y, int _z);
        int operator [](const int i) const;
        bool operator <(const PointIndex &b) const;
};

class PointCloud
{
    private:
        double m_voxelSize;
        std::map<PointIndex, SinglePoint> m_clouds;
        std::map<PointIndex, SinglePoint> m_add_clouds;
        std::mutex m_mutex;
        cv::Mat m_pose;

    public:

        PointCloud(double voxelSize);
        PointCloud() {}
        void setvoxelSize(double _voxelSize);
        PointIndex getIndex(const cv::Vec3f &coord);
        void addPoint(const cv::Vec3f &coord, const SinglePoint &p);
        void addPoint(const PointIndex &idx, const SinglePoint &p);
        cv::Vec3f calCoord(const PointIndex &idx);
        bool isExists(const cv::Vec3f &coord);
        int Size();
        double getvoxelSize();
        void merge(PointCloud *_cloud);
        std::map<PointIndex, SinglePoint>::iterator m_begin();
        std::map<PointIndex, SinglePoint>::iterator m_end();
        void setPose(const cv::Mat& pose);
        cv::Mat getPose();
        void m_lock();
        void m_unclock();
        void getRenderInfo(glm::mat4 &pose, GLfloat* vertex_buffer, int &point_num);
        void getIncrementRenderInfo(glm::mat4 &pose, GLfloat* vertex_buffer, int &point_num);
        void setIncrementCloud(std::map<PointIndex, SinglePoint> &inc);
        std::map<PointIndex, SinglePoint> & getClouds();
};

class RGBDP
{
    public:
        cv::Mat rgb;
        cv::Mat depth;
        cv::Mat pose;  
};

int ImagePNGCompression(cv::Mat &img, uchar* &data);
int ImageJPEGCompression(cv::Mat &img, uchar* &data);
void ImageDecompression(cv::Mat &img, uchar *data, int nSize, int flag);