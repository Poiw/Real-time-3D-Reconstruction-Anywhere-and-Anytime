cmake_minimum_required(VERSION 3.0)

project(ReconstructionSystem)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenCL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "-O2 -g")
set(CMAKE_CXX_STANDARD 14)

if (APPLE)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMACOS")
endif()

include_directories(
        ./
        /usr/local/include
        /usr/local/include/opencv4
        ${OpenCL_INCLUDE_DIR}
)

link_directories(
        /usr/local/lib
)

set(RENDERING_LIBS
        ${OPENGL_LIBRARIES} 
        glfw
        GLEW
)

if(FALSE)
add_executable(test_client client/main.cpp 
                       common/transfer.cpp)

add_executable(test_server server/main.cpp 
                       common/transfer.cpp)

target_link_libraries(test_server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(test_client ${CMAKE_THREAD_LIBS_INIT})
endif()

add_executable(rendering rendering/main.cpp
                          rendering/shader.cpp)

add_executable(client client/7scene_main.cpp
                      common/transfer.cpp
                      common/dataStructure.cpp
                      client/ImageClient.cpp)

add_executable(server server/reconstructionServer_main.cpp
                      common/transfer.cpp
                      common/dataStructure.cpp
                      rendering/render.cpp
                      rendering/shader.cpp
                      reconstruction/pointcloud.cpp
                      reconstruction/reconstruction.cpp
                      opencl/tool.cpp)


target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server ${OpenCV_LIBRARIES})
target_link_libraries(server ${RENDERING_LIBS})
target_link_libraries(server ${OpenCL_LIBRARY})

target_link_libraries(client ${OpenCV_LIBRARIES})
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(rendering ${RENDERING_LIBS})
target_link_libraries(rendering ${OpenCV_LIBRARIES})