#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include "opencl/opencl_header.h"
#include <memory>
#include "common/dataStructure.hpp"
#include <queue>

void generateCoordMap(const cv::Mat &depth_img, const cv::Mat &pose, cv::Mat &coord_map, std::shared_ptr<Camera> camera);
// void generateCoordMapCL(const cv::Mat &depth_img, const cv::Mat &pose, cv::Mat &coord_map, std::shared_ptr<Camera> camera);
bool checkPoint(const cv::Vec3f &coord);
void registration(const cv::Mat &color_img, const cv::Mat &coord_map, const cv::Mat &pose, PointCloud* cloud);
