#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#if defined(__APPLE__) || defined(__MACOSX__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include "rex.h"

inline void devcall_opencl_errchk(int code, const char *file, int line, int ab) {
	if (code != CL_SUCCESS) {
		fprintf(stderr,"OpenCL assert: %d %s %d\n", code, file, line);
		if (ab) { abort();}
	}
}
#define devcall_opencl_assert(ecode) { devcall_opencl_errchk((ecode), __FILE__, __LINE__, 1); }

int rex_probe_dev_opencl() {
    return 0;
}

void rex_init_dev_opencl(rex_device_t *dev, int id, int sysid, int num_cores) {
    dev->type = REX_DEVICE_ITLGPU_OPENCL;
    dev->id = id;
    dev->sysid = sysid;
    dev->default_stream = NULL;
    dev->mem_type = REX_DEVICE_MEM_SHARED;
    dev->num_cores = num_cores;

    // Default substring for platform name
    const char* required_platform_subname = "Intel";
    cl_int err = CL_SUCCESS;

    // Query for all available OpenCL platforms on the system
    cl_uint num_of_platforms = 0;
    // get total number of available platforms:
    err = clGetPlatformIDs(0, 0, &num_of_platforms);
    devcall_opencl_assert(err);
    printf("Number of available platforms: %d\n", num_of_platforms);

    cl_platform_id platforms[num_of_platforms];
    // get IDs for all platforms:
    err = clGetPlatformIDs(num_of_platforms, platforms, 0);
    devcall_opencl_assert(err);

    // List all platforms and select one.
    // We use platform name to select needed platform.
    cl_uint selected_platform_index = num_of_platforms;
    int i;
    for(i = 0; i < num_of_platforms; ++i)
    {
        // Get the length for the i-th platform name
        size_t platform_name_length = 0;
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, 0, &platform_name_length);
        devcall_opencl_assert(err);

        // Get the name itself for the i-th platform
        char platform_name[platform_name_length];
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, platform_name_length, platform_name, 0);
        devcall_opencl_assert(err);

        // decide if this i-th platform is what we are looking for
        // we select the first one matched skipping the next one if any
        if(strstr(platform_name, required_platform_subname) &&
            selected_platform_index == num_of_platforms) { // have not selected yet
            selected_platform_index = i;
            break;
        }
    }

    if(selected_platform_index == num_of_platforms) {
        fprintf(stderr, "There is no found platform with name containing %s as a substring.\n", required_platform_subname);
        return;
    }

    cl_platform_id platform = platforms[selected_platform_index];

    // check intel GPU
    int num_itlgpu;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, 0, (cl_uint*) &num_itlgpu);
    if(CL_DEVICE_NOT_FOUND == err) {
        num_itlgpu = 0;
        err = CL_SUCCESS;
    }
    devcall_opencl_assert(err);

    if (sysid >= num_itlgpu) {
        fprintf(stderr, "The specified sysid %d for Intel GPU is not avaialble. Total Intel GPU: %d\n", id, num_itlgpu);
        return;
    } else {

        // get a piece of useful capabilities information for each device.
        // Retrieve a list of device IDs with type selected by type_index
        cl_device_id itlgpu_dev_ids[num_itlgpu];
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_itlgpu, itlgpu_dev_ids, 0);
        devcall_opencl_assert(err);
        dev->dev_properties = (void*)itlgpu_dev_ids[sysid];
        //dev->dev_properties = (cl_device_id*)malloc(sizeof(cl_device_id));
        //memcpy(dev->dev_properties, &itlgpu_dev_ids[sysid], sizeof(cl_device_id));
    }
}


void rex_warmup_dev_opencl(rex_device_t *dev) {
    rex_device_type_t devtype = dev->type;
    cl_int err;
    // Create a context
    cl_context context;
    context = clCreateContext(0, 1, (cl_device_id*)&dev->dev_properties, NULL, NULL, &err);
    dev->default_context = (void*)context;
}

int rex_set_current_dev_opencl(rex_device_t *d) {
    return d->id;
}

// terminate helper threads
void rex_fini_dev_opencl(rex_device_t * dev) {
    rex_device_type_t devtype = dev->type;
    clReleaseCommandQueue(*((cl_command_queue*)dev->default_stream));
    clReleaseContext((cl_context)dev->default_context);
}
