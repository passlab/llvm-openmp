#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include "rex.h"

inline void devcall_nvgpu_cuda_errchk(cudaError_t code, const char *file, int line, int ab) {
	if (code != cudaSuccess) {
		fprintf(stderr,"NVGPU_CUDA assert: %s %s %d\n", cudaGetErrorString(code), file, line);
		if (ab) { abort();}
	}
}
#define devcall_nvgpu_cuda_assert(ecode) { devcall_nvgpu_cuda_errchk((ecode), __FILE__, __LINE__, 1); }

/**
 * probe the system using CUDA SDK and return the total number of CUDA-capable NVIDIA GPU
 * @return
 */
int rex_probe_dev_nvcuda() {
	int total_nvcuda = 0;

	cudaError_t result = cudaGetDeviceCount(&total_nvcuda);
	devcall_nvgpu_cuda_assert(result);
	return total_nvcuda;
}

void rex_init_dev_nvcuda(rex_device_t *dev, int id, int sysid) {
    dev->type = REX_DEVICE_NVGPU_CUDA;
    dev->id = id;
    dev->sysid = sysid;
    dev->default_stream = NULL;
    dev->mem_type = REX_DEVICE_MEM_DISCRETE;
}

void rex_warmup_dev_nvcuda(rex_device_t *dev) {
    rex_device_type_t devtype = dev->type;
	dev->dev_properties = (struct cudaDeviceProp*)malloc(sizeof(struct cudaDeviceProp));
	cudaSetDevice(dev->sysid);
	cudaGetDeviceProperties((cudaDeviceProp*)dev->dev_properties, dev->sysid);

	/* warm up the device */
	void * dummy_dev;
	char dummy_host[1024];
	cudaMalloc(&dummy_dev, 1024);
	cudaMemcpy(dummy_dev, dummy_host, 1024, cudaMemcpyHostToDevice);
	cudaMemcpy(dummy_host, dummy_dev, 1024, cudaMemcpyDeviceToHost);
	cudaFree(dummy_dev);
}

int rex_set_current_dev_nvcuda(rex_device_t *d) {
	cudaError_t result = cudaSetDevice(d->sysid);
	devcall_nvgpu_cuda_assert (result);
	return d->id;
}

// terminate helper threads
void rex_fini_dev_nvcuda(rex_device_t *dev) {
    free(dev->dev_properties);
}

