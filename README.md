# Real-time 3D Reconstruction: Anywhere and Anytime

## Introduction

We implemented a single client to single server real time 3D reconstruction by RGD-D image and pose flow. This project can run in real networks.



## Environment Requirement

- OpenCV
- OpenGL
- OpenMP
- GLEW



## Dataset

Currently we **only support** the input from the *7-scenes* dataset. You can download the *7-scenes* dataset [here](https://www.microsoft.com/en-us/research/project/rgb-d-dataset-7-scenes/) and unzip the scene sequence which you want to test the performance.



## Build the System

### Configure the system

You need to configure the parameters in `client/7scenes_main.cpp` to fit your environment.

`data_path`: the data sequence you want to test. e.g. `<7-scenes-rootdir>/chess/seq-01`

`RGBDPClient`:   
- The first parameter indicates the server's IP address, e.g. `"localhost"`. 

- The forth parameter indicates the number of testing frame, e.g. `1000`. 

- The other parameters can maintain default.

### Compile the System

```bash
mkdir build

cd build

cmake ..

make
```



## Run the System

- **Server** 

````
./build/server
````

- **Client**

````
./build/client
````

Please run the server first.

## Acknowledge

- Glocker, Ben, et al. "Real-time RGB-D camera relocalization." *2013 IEEE International Symposium on Mixed and Augmented Reality (ISMAR)*. IEEE, 2013.

