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


/* nvcuda specific datamap mm APIs */
void *rex_malloc_nvcuda(rex_device_t * dev, size_t size) {
	void * ptr = NULL;
	ptr = malloc(size);
	cudaError_t errno = cudaMalloc(&ptr, size);
	devcall_nvgpu_cuda_assert(errno);

	return ptr;
}

void rex_free_nvcuda(rex_device_t * dev, void *ptr) {
	cudaError_t result = cudaFree(ptr);
	devcall_nvgpu_cuda_assert(result);
}

void *rex_memcpy_btw_nvcuda(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size) {
	cudaError_t result;
	result = cudaMemcpy((void *)dest,(const void *)src, size, cudaMemcpyDeviceToDevice);
//		result = cudaMemcpyPeer(dst, dstdev->sysid, src, srcdev->sysid, size);
	devcall_nvgpu_cuda_assert(result);
	return dest;
}

void *rex_memcpy_to_nvcuda(void *dest, rex_device_t * dest_dev, const void *src, size_t size) {
	cudaError_t result;
	result = cudaMemcpy((void *)dest,(const void *)src, size, cudaMemcpyHostToDevice);
	devcall_nvgpu_cuda_assert(result);

	return dest;
}

void *rex_memcpy_from_nvcuda(void *dest, const void *src, rex_device_t * src_dev, size_t size) {
	cudaError_t result;
	result = cudaMemcpy((void *)dest,(const void *)src, size, cudaMemcpyDeviceToHost);
	devcall_nvgpu_cuda_assert(result);
	return dest;
}

int rex_asyncmemcpy_btw_nvcuda(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size, rex_dev_stream_t stream) {
	cudaError_t result;
	result = cudaMemcpyAsync((void *)dest,(const void *)src, size, cudaMemcpyDeviceToDevice, cudaStream_t(stream));
	//result = cudaMemcpyPeerAsync(dst, dstdev->sysid, src, srcdev->sysid, size, srcstream->systream.cudaStream);
	devcall_nvgpu_cuda_assert(result);
	return 0;
}

int rex_asyncmemcpy_to_nvcuda(void *dest, rex_device_t * dest_dev, const void *src, size_t size, rex_dev_stream_t stream) {
	cudaError_t result;
	result = cudaMemcpyAsync((void *)dest,(const void *)src, size, cudaMemcpyHostToDevice, cudaStream_t(stream));
	devcall_nvgpu_cuda_assert(result);
	return 0;
}

int rex_asyncmemcpy_from_nvcuda(void *dest, const void *src, rex_device_t * src_dev, size_t size, rex_dev_stream_t stream) {
	cudaError_t result;
	result = cudaMemcpyAsync((void *)dest,(const void *)src,size, cudaMemcpyDeviceToHost, cudaStream_t(stream));
//	printf("memcpyfrom_async: dev: %d, %X->%X\n", srcdev->id, src, dst);
	devcall_nvgpu_cuda_assert(result);
	return 0;
}

int rex_enable_memcpy_btw_nvcuda(rex_device_t *destdev, rex_device_t *srcdev) {
	int can_access = 0;
	cudaError_t result;
	result = cudaDeviceCanAccessPeer(&can_access, srcdev->sysid, destdev->sysid);
	devcall_nvgpu_cuda_assert(result);
	if (can_access) {
		result = cudaDeviceEnablePeerAccess(destdev->sysid, 0);
		if(result != cudaErrorPeerAccessAlreadyEnabled) {
			return 0;
		} else return 1;
	} else return 1;
}

/** stream related APIs for nvcuda */
void rex_stream_create_nvcuda(rex_device_t *d, rex_dev_stream_t *stream) {
	cudaError_t result;
	result = cudaStreamCreateWithFlags((cudaStream_t *)stream, cudaStreamNonBlocking);
	devcall_nvgpu_cuda_assert(result);
}

/**
 * sync device by syncing the stream so all the pending calls the stream are completed
 *
 * if destroy_stream != 0; the stream will be destroyed.
 */
void rex_stream_sync_nvcuda(rex_dev_stream_t stream) {
	cudaError_t result;
	result = cudaStreamSynchronize((cudaStream_t)stream);
	devcall_nvgpu_cuda_assert(result);
}

void rex_stream_destroy_nvcuda(rex_dev_stream_t stream) {
	cudaError_t result;
	result = cudaStreamDestroy(cudaStream_t(stream));
	devcall_nvgpu_cuda_assert(result);
}