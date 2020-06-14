#include "dataStructure.hpp"

Camera::Camera(int _fx, int _fy, int _tx, int _ty) : m_fx(_fx), m_fy(_fy), m_tx(_tx), m_ty(_ty)
{
    m_cameraIntrinsic = (cv::Mat_<float>(3, 3) << _fx, 0, _tx,
                         0, _fy, _ty,
                         0, 0, 1);
}

void Camera::reModel()
{
    m_cameraIntrinsic = (cv::Mat_<float>(3, 3) << m_fx, 0, m_tx,
                                                    0, m_fy, m_ty,
                                                    0, 0, 1);
}


SinglePoint::SinglePoint(const cv::Vec3b &color) : m_rgb(color), m_count(1) {}

void SinglePoint::merge(const SinglePoint b)
{
    float beta = 1.0 / (1 + m_count), alpha = 1.0 * m_count * beta;
    m_rgb = alpha * m_rgb + beta * b.m_getColor();
    m_count += 1;
}

cv::Vec3b SinglePoint::m_getColor() const
{
    return m_rgb;
}

PointIndex::PointIndex(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}

int PointIndex::operator[](const int i) const
{
    switch (i)
    {
    case 0:
        return x;
        break;
    case 1:
        return y;
        break;

    default:
        return z;
        break;
    }
}

bool PointIndex::operator <(const PointIndex &b) const 
{
    if (x != b.x) return x < b.x;
    if (y != b.y) return y < b.y;
    return z < b.z;
}

PointCloud::PointCloud(double voxelSize) : m_voxelSize(voxelSize) {}

void PointCloud::setvoxelSize(double _voxelSize)
{
    m_voxelSize = _voxelSize;
}

PointIndex PointCloud::getIndex(const cv::Vec3f &coord)
{
    return PointIndex(coord[0] / m_voxelSize, coord[1] / m_voxelSize, coord[2] / m_voxelSize);
}

void PointCloud::addPoint(const cv::Vec3f &coord, const SinglePoint &p)
{
    addPoint(getIndex(coord), p);
}

void PointCloud::addPoint(const PointIndex &idx, const SinglePoint &p)
{
    if (m_clouds.count(idx) > 0) {
        m_clouds[idx].merge(p);
    }
    else {
        m_clouds[idx] = p;
    }
}

cv::Vec3f PointCloud::calCoord(const PointIndex &idx)
{
    return cv::Vec3f((idx[0] + 0.5) * m_voxelSize, (idx[1] + 0.5) * m_voxelSize, (idx[2] + 0.5) * m_voxelSize);
}

bool PointCloud::isExists(const cv::Vec3f &coord)
{
    return m_clouds.count(getIndex(coord)) > 0;
}

int PointCloud::Size()
{
    return m_clouds.size();
}

double PointCloud::getvoxelSize()
{
    return m_voxelSize;
}

std::map<PointIndex, SinglePoint>::iterator PointCloud::m_begin()
{
    return m_clouds.begin();
}

std::map<PointIndex, SinglePoint>::iterator PointCloud::m_end()
{
    return m_clouds.end();
}

void PointCloud::merge(PointCloud *_cloud)
{
    std::map<PointIndex, SinglePoint>::iterator it, begin = _cloud->m_begin(), end = _cloud->m_end();
    for (it = begin; it != end; ++it) {
        m_clouds[it->first] = it->second;
    }
}

void PointCloud::m_lock()
{
    m_mutex.lock();
}

void PointCloud::m_unclock()
{
    m_mutex.unlock();
}

void PointCloud::setPose(const cv::Mat &pose)
{
    m_pose = pose;
}

cv::Mat PointCloud::getPose()
{
    return m_pose;
}

void PointCloud::getRenderInfo(glm::mat4 &pose, GLfloat* vertex_buffer, int &point_num)
{
    m_lock();
    point_num = m_clouds.size();

    int i = 0;
    for (auto &it : m_clouds) {
        auto p = calCoord(it.first);
        for(int j = 0; j < 3; j++) {
            //printf("COOL %d %d %d %d\n", i, j, point_num * 3, 3*i + j);
            vertex_buffer[6*i + j] = p[j];
            vertex_buffer[6*i + 3 + j] = it.second.m_getColor()[j] / 255.f;
            //printf("DONE\n");
        }
        i++;
    }

    for (int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            pose[j][i] = m_pose.at<GLfloat>(i, j);
        }
    }

    m_unclock();
}

void PointCloud::getIncrementRenderInfo(glm::mat4 &pose, GLfloat* vertex_buffer, int &point_num)
{
    m_lock();
    point_num = m_add_clouds.size();

    int i = 0;
    for (auto &it : m_add_clouds) {
        auto p = calCoord(it.first);
        for(int j = 0; j < 3; j++) {
            //printf("COOL %d %d %d %d\n", i, j, point_num * 3, 3*i + j);
            vertex_buffer[6*i + j] = p[j];
            vertex_buffer[6*i + 3 + j] = it.second.m_getColor()[j] / 255.f;
            //printf("DONE\n");
        }
        i++;
    }

    for (int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            pose[j][i] = m_pose.at<GLfloat>(i, j);
        }
    }

    m_unclock();
}


void PointCloud::setIncrementCloud(std::map<PointIndex, SinglePoint> &inc)
{
    m_add_clouds = inc;
}


std::map<PointIndex, SinglePoint> & PointCloud::getClouds()
{
    return m_clouds;
}


int ImagePNGCompression(cv::Mat &img, uchar* &data)
{
    std::vector<uchar> data_enocode;
    std::vector<int> quality;

    quality.push_back(cv::IMWRITE_PNG_COMPRESSION);
    quality.push_back(5); // image compression quality

    cv::imencode(".png", img, data_enocode, quality);

    int nSize = data_enocode.size();
    data = new uchar[nSize+5];

    std::copy(data_enocode.begin(), data_enocode.end(), data);

    return nSize;
}

int ImageJPEGCompression(cv::Mat &img, uchar* &data)
{
    std::vector<uchar> data_enocode;
    std::vector<int> quality;

    quality.push_back(cv::IMWRITE_JPEG_QUALITY);
    quality.push_back(50); // image compression quality

    cv::imencode(".jpg", img, data_enocode, quality);

    int nSize = data_enocode.size();
    data = new uchar[nSize+5];

    std::copy(data_enocode.begin(), data_enocode.end(), data);

    return nSize;
}

void ImageDecompression(cv::Mat &img, uchar *data, int nSize, int flag)
{
    std::vector<uchar> data_encode(data, data+nSize);
    img = cv::imdecode(data_encode, flag);
}