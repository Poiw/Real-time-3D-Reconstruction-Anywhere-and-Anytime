#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>

#include <GL/glew.h> // This must be done before glfw

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <opencv2/opencv.hpp>

#include "rendering/shader.hpp"

#include <thread>

#define width_init 320
#define height_init 240

GLFWwindow *window;

int init()
{
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW.\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 1); //4x MSAA
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //Make macOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width_init, height_init, "Hello World", NULL, NULL);
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

	std::cout << glGetString(GL_VERSION) << std::endl;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(0.f, 0.f, 0.f, 0.f);

    // Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

    return 0;
}

int load_point_cloud(const char *file_name, GLfloat **vertext_buffer, GLfloat **color_buffer)
{
	FILE* file = fopen(file_name, "r");
	int res;
	std::vector<GLfloat> pos;
	std::vector<GLfloat> color;

	while(1) {
		GLfloat a, b, c;
		res = fscanf(file, "%f %f %f", &a, &b, &c);
		if (res != 3) break;
		pos.push_back(a);
		pos.push_back(b);
		pos.push_back(c);

		fscanf(file, "%f %f %f", &a, &b, &c);
		color.push_back(a / 128);
		color.push_back(b / 128);
		color.push_back(c / 128);
	}

	res = pos.size();
	*vertext_buffer = new GLfloat[res];
	*color_buffer = new GLfloat[res];

	memcpy(*vertext_buffer, pos.data(), sizeof(GLfloat) * res);
	memcpy(*color_buffer, color.data(), sizeof(GLfloat) * res);

#ifdef VERBOSE
	printf("read %d points\n", res / 3);
#endif
	return res / 3;
}

void save_image(char* color)
{
	auto img = cv::Mat(height_init * 2, width_init * 2, CV_8UC3, color);
	cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
	cv::flip(img, img, 0); 
	cv::imwrite("./res.png", img);
}

int main()
{
    if (init() != 0) return -1;

	glfwMakeContextCurrent(window);
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint programID = LoadShaders("../rendering/vertex.glsl", "../rendering/segment.glsl");

    GLuint mvpID = glGetUniformLocation(programID, "s_mvp");

	GLfloat *vertex_data, *color_data;
	int point_num;

	//point_num = load_point_cloud("../rendering/scene.txt", &vertex_data, &color_data);
	point_num = load_point_cloud("/Users/lipei/Downloads/pcloud.txt", &vertex_data, &color_data);

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, point_num * 3 * sizeof(GLfloat), vertex_data, GL_STATIC_DRAW);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, point_num * 3 * sizeof(GLfloat), color_data, GL_STATIC_DRAW);

	double last_time = glfwGetTime();

	glm::vec3 position = glm::vec3( 3, 3, 0 );
	// horizontal angle : toward -Z
	float horizontalAngle = 3.14f;
	// vertical angle : 0, look at the horizon
	float verticalAngle = 0.0f;
	// Initial Field of View
	float initialFoV = 45.0f;

	float speed = 3.0f; // 3 units / second
	float mouseSpeed = 0.3f;

	int nbFrames = 0;
	double last_nb_time = glfwGetTime();

	char *colors=new char[width_init * height_init * 3 * 4];
	glfwSetCursorPos(window, width_init / 2, height_init / 2);

    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		double current_time = glfwGetTime();
		float delta_time = current_time - last_time;
		last_time = current_time;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwSetCursorPos(window, width / 2, height / 2);

		horizontalAngle += mouseSpeed * delta_time * float(width / 2 - xpos);
		verticalAngle += mouseSpeed * delta_time * float(height / 2 - ypos);

		glm::vec3 direction(
			cos(verticalAngle) * sin (horizontalAngle),
			sin(verticalAngle),
			cos(verticalAngle) * cos(horizontalAngle)
		);

		glm::vec3 right(
			sin(horizontalAngle - 3.14f/2.0f),
			0,
			cos(horizontalAngle - 3.14f/2.0f)
		);

		glm::vec3 up = glm::cross(right, direction);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			position += direction * delta_time * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			position -= direction * delta_time * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			position += right * delta_time * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			position -= right * delta_time * speed;
		}

		glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f * width / height, 0.1f, 100.0f);
    	glm::mat4 View       = glm::lookAt(
								position, // Camera is at (4,3,3), in World Space
								position + direction, //position + direction, // and looks at the origin
								up  // Head is up (set to 0,-1,0 to look upside-down)
						   );
		/*View = glm::lookAt(
			glm::vec3(3, 3, 0), 
			glm::vec3(0, 0, 0),
			glm::vec3(0, 0, 1)
		);*/
		View = glm::lookAt(
            glm::vec3(0, -5, -3.5),
            glm::vec3(0, -1, 1),
            glm::vec3(0, -1, 0)
        );
    	glm::mat4 Model      = glm::mat4(1.0f);
    	glm::mat4 mvp        = Projection * View * Model;

        glUseProgram(programID);

        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvp[0][0]);

		glPointSize(2);
		
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
            0, 
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0
        );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0
        );

        glDrawArrays(GL_POINTS, 0, point_num * 3);
        
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);
        glfwPollEvents();

		nbFrames += 1;
		double current_nb_time = glfwGetTime();
		if (current_nb_time - last_nb_time >= 1) {
			printf("%f ms/frame\n%d fps\n", 1000.0/double(nbFrames), nbFrames);
         	nbFrames = 0;
         	last_nb_time += 1.0;
		}

		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			glReadPixels(0, 0, width_init * 2, height_init * 2, GL_RGB, GL_UNSIGNED_BYTE, colors);
			printf("P pressed\n");
			save_image(colors);
		}

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && 
        glfwWindowShouldClose(window) == 0);
    
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    glfwTerminate();

    return 0;
}
