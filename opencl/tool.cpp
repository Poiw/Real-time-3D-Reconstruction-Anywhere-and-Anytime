#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include "tool.hpp"
using namespace std;

/** convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
    size_t size;
    char*  str;
    std::fstream f(filename, (std::fstream::in | std::fstream::binary));

    if(f.is_open())
    {
        size_t fileSize;
        f.seekg(0, std::fstream::end);
        size = fileSize = (size_t)f.tellg();
        f.seekg(0, std::fstream::beg);
        str = new char[size+1];
        if(!str)
        {
            f.close();
            return 0;
        }

        f.read(str, fileSize);
        f.close();
        str[size] = '\0';
        s = str;
        delete[] str;
        return 0;
    }
    cout<<"Error: failed to open file\n:"<<filename<<endl;
    return -1;
}

/**Getting platforms and choose an available one.*/
int getPlatform(cl_platform_id &platform)
{
    platform = NULL;//the chosen platform

    cl_uint numPlatforms;//the NO. of platforms
    cl_int    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if (status != CL_SUCCESS)
    {
        cout<<"Error: Getting platforms!"<<endl;
        return -1;
    }

    /**For clarity, choose the first available platform. */
    if(numPlatforms > 0)
    {
        cl_platform_id* platforms =
            (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        platform = platforms[0];
        free(platforms);
        return 0;
    }
    else
        return -1;
}

/**Step 2:Query the platform and choose the first GPU device if has one.*/
cl_device_id *getCl_device_id(cl_platform_id &platform)
{
    cl_uint numDevices = 0;
    cl_device_id *devices=NULL;
    cl_int status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    std::cout << "devices: " <<  numDevices << "\n";
    if (numDevices > 0) //GPU available.
    {
        devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    }
    return devices;
}