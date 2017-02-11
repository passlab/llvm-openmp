#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "rex.h"

int rex_probe_dev_hostcpu() {
    return 0;
}

void rex_init_dev_hostcpu(rex_device_t *dev, int id, int sysid, int num_cores) {
    dev->type = REX_DEVICE_HOSTCPU_KMP;
    dev->id = id;
    dev->sysid = sysid;
    dev->default_stream = NULL;
    dev->mem_type = REX_DEVICE_MEM_SHARED_CC_NUMA;
    dev->num_cores = num_cores;
    //dev->num_cores = sysconf( _SC_NPROCESSORS_ONLN );
    //double dummy = cpu_sustain_gflopss(&dev->flopss_percore);
    dev->total_real_flopss = dev->num_cores * dev->flopss_percore;
    dev->bandwidth = 600 * 1000; /* GB/s */
    dev->latency = 0.02; /* us, i.e. 20 ns */
}

void rex_warmup_dev_hostcpu(rex_device_t *dev) {

}

int rex_set_current_dev_hostcpu(rex_device_t *d) {
    return d->id;
}

void rex_fini_dev_hostcpu(rex_device_t * dev) {
}

/* hostcpu specific datamap mm APIs */
void *rex_malloc_hostcpu(rex_device_t * dev, size_t size) {
    void * ptr = NULL;
    ptr = malloc(size);
    return ptr;
}

void rex_free_hostcpu(rex_device_t * dev, void *ptr) {
    free(ptr);
}

void *rex_memcpy_btw_hostcpu(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_memcpy_to_hostcpu(void *dest, rex_device_t * dest_dev, const void *src, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_memcpy_from_hostcpu(void *dest, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_asyncmemcpy_btw_hostcpu(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_asyncmemcpy_to_hostcpu(void *dest, rex_device_t * dest_dev, const void *src, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_asyncmemcpy_from_hostcpu(void *dest, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

