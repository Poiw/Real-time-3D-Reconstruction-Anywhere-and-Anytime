#pragma once

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>

#include <GL/glew.h> // This must be done before glfw

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <opencv2/opencv.hpp>

#include "reconstruction/reconstruction.hpp"
#include "common/dataStructure.hpp"
#include "rendering/shader.hpp"

#define ERR_RENDER_CREATE 10

#define MAX_POINT_CLOUD_SIZE 50000000

#define X_SCALE 1
#define Y_SCALE 1

class Render
{
    public:
        Render(u_int _width, u_int _height) : width(_width), height(_height) {
            if (init() != 0) {
                printf("Failed to create rendering env\n");
                throw -ERR_RENDER_CREATE;
            }
            vertex_data = new float[6 * MAX_POINT_CLOUD_SIZE];
            colors = new char[width * height * 3 * X_SCALE * Y_SCALE];
            increment_rendering = 0;
            threshold_point = -1;
            point_num = 0;
        }
        ~Render();
        void getImg(cv::Mat &showImg);
        void render_loop();
        void set_ReconstructionServer(ReconstructionServer *_rec) {
            recstr_server = _rec;
        }

    private:
        GLFWwindow *window;
        ReconstructionServer *recstr_server;

        u_int width, height;
        GLuint VertexArrayID;
        GLuint programID;
        GLuint mvpID;
        GLfloat *vertex_data;
        GLuint vertexbuffer;
        GLuint colorbuffer;
        glm::mat4 mvp;
        bool increment_rendering;
        char *colors;
        int point_num;
        int threshold_point;
        int init();
        void render();
        //void render_loop();
};
