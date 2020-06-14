#include "rendering/render.hpp"
#include <assert.h>
#include <algorithm>
#include <random>

static void GLClearError()
{
    while (glGetError());
}

static void GLCheckError(const char *filename, const int line)
{
    int flag = 0;
    while (GLenum error = glGetError()) {
        std::cout << "######## [OpenGL Error] " << error << " at file" << filename << ":" << line << " Error Code:" << error << " ##########" << std::endl;
        flag = 1;
    }
    assert(flag == 0);
}

#define GLCall(x) GLClearError();\
    x;\
    GLCheckError(__FILE__, __LINE__)

int Render::init()
{
    /************** Initialize OpenGL Environment ***********/
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW.\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 1); //4x MSAA
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //Make macOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_VISIBLE, GL_FALSE); //hide the window

    window = glfwCreateWindow(width, height, "Server Render Output", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW.\n");
        glfwTerminate();
        return -1;
    }

    glClearColor(0.f, 0.f, 0.f, 0.f);

    // Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

    /************** Initialize Rendering Object ***********/
    GLCall(glGenVertexArrays(1, &VertexArrayID));
    GLCall(glBindVertexArray(VertexArrayID));

    programID = LoadShaders("../rendering/vertex.glsl", "../rendering/segment.glsl");

    GLCall(glUseProgram(programID));
    GLCall(mvpID = glGetUniformLocation(programID, "s_mvp"));

    GLCall(glGenBuffers(1, &vertexbuffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, MAX_POINT_CLOUD_SIZE * 6 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float))));

    std::cout << glGetString(GL_VERSION) << std::endl;

    return 0;
}

struct RenderingPoint
{
    GLfloat p[3];
    GLfloat c[3];
};

void Render::render_loop()
{
    GLCall(glPointSize(2));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    std::random_device rd;
    std::mt19937 g(rd());

    while(1) {
        if (!increment_rendering) GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        auto Projection = glm::perspective(2*(float)atan(240.0 / 585.0), 1.0f * width / height, 0.1f, 30.0f);
        // auto Projection = glm::perspective(glm::radians(45.0f), 1.0f * width / height, 0.1f, 10.0f);
        glm::mat4 pose;
        auto view = glm::lookAt(
            glm::vec3(0, -2, -1),
            glm::vec3(0, 0, 2),
            glm::vec3(0, -1, 0)
        );

        recstr_server -> m_render_image_mutex.lock();

        auto t_start = std::chrono::high_resolution_clock::now(); 
        int new_point_num;
        
        // recstr_server -> m_curCloud.getIncrementRenderInfo(pose, vertex_data, new_point_num);
        // std::cout << "Add point num: " << new_point_num << std::endl;
        // GLCall(glBufferSubData(GL_ARRAY_BUFFER, point_num * 6 * sizeof(GLfloat), new_point_num * 6 * sizeof(GLfloat), vertex_data));
        // point_num += new_point_num;
        
        recstr_server -> m_curCloud.getRenderInfo(pose, vertex_data, point_num);

        // Random draw point to render. Doesn't work well.
        if (threshold_point != -1 && point_num > threshold_point) {
            RenderingPoint *begin = (RenderingPoint *)vertex_data;
            std::shuffle(begin, begin + point_num, g);
            point_num = threshold_point;
        }

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, point_num * 6 * sizeof(GLfloat), vertex_data));

        pose = glm::inverse(pose);
        glm::mat4 tmp = Projection * view;

        GLCall(glUniformMatrix4fv(mvpID, 1, GL_FALSE, &tmp[0][0]));

        if (increment_rendering) {
            std::cout << "Incremental rendering unimplemented" << std::endl;
            throw 0;
            std::cout << "Rendered point num:" << new_point_num << std::endl;
            GLCall(glDrawArrays(GL_POINTS, point_num - new_point_num, new_point_num));
        } else {
            std::cout << "Rendered point num:" << point_num << std::endl;
            GLCall(glDrawArrays(GL_POINTS, 0, point_num));
        }

        glfwSwapBuffers(window);
        GLCall(glReadPixels(0, 0, width * X_SCALE, height * Y_SCALE, GL_RGB, GL_UNSIGNED_BYTE, colors));

        auto img = cv::Mat(height * Y_SCALE, width * X_SCALE, CV_8UC3, colors);
        cv::flip(img, img, 0);
        recstr_server -> m_send_end_mutex.lock();
        recstr_server -> showImg = img;
        recstr_server -> m_sending_mutex.unlock();

        glfwPollEvents();

        auto t_end = std::chrono::high_resolution_clock::now();

        std::cout << "[Rendering] Process time: " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << "ms\n";
    }
}

Render::~Render()
{
    GLCall(glDeleteBuffers(1, &vertexbuffer));
    GLCall(glDisableVertexAttribArray(0));
    GLCall(glDisableVertexAttribArray(1));
    GLCall(glDeleteVertexArrays(1, &VertexArrayID));
    if (vertex_data != NULL) delete vertex_data;
    glfwTerminate();
}
