#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "rex.h"

int rex_probe_dev_thsim() {
    return 0;
}

void rex_init_dev_thsim(rex_device_t *dev, int id, int sysid, int num_cores) {
    dev->type = REX_DEVICE_THSIM_PTHREAD;
    dev->id = id;
    dev->sysid = sysid;
    dev->default_stream = NULL;
    dev->mem_type = REX_DEVICE_MEM_DISCRETE;
    dev->num_cores = num_cores;
    /*
     dev->num_cores = rex_host_dev->num_cores;
    dev->flopss_percore = rex_host_dev->flopss_percore;
    dev->total_real_flopss = rex_host_dev->total_real_flopss*(1+dev->id);
    dev->bandwidth = (2*(1+dev->id))*rex_host_dev->bandwidth / 100;
    dev->latency = (1+dev->id)*rex_host_dev->latency * 1000;
    */
}


void rex_warmup_dev_thsim(rex_device_t *dev) {

}

int rex_set_current_dev_thsim(rex_device_t *d) {
    return d->id;
}

void rex_fini_dev_thsim(rex_device_t * dev) {
}

/* thsim specific datamap mm APIs */
void *rex_malloc_thsim(rex_device_t * dev, size_t size) {
    void * ptr = NULL;
    ptr = malloc(size);
    return ptr;
}

void rex_free_thsim(rex_device_t * dev, void *ptr) {
    free(ptr);
}

void *rex_memcpy_btw_thsim(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_memcpy_to_thsim(void *dest, rex_device_t * dest_dev, const void *src, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

void *rex_memcpy_from_thsim(void *dest, const void *src, rex_device_t * src_dev, size_t size) {
    return memcpy((void *) dest, (const void *) src, size);
}

int rex_asyncmemcpy_btw_thsim(void *dest, rex_device_t * dest_dev, const void *src, rex_device_t * src_dev, size_t size) {
    memcpy((void *) dest, (const void *) src, size);
    return 0;
}

int rex_asyncmemcpy_to_thsim(void *dest, rex_device_t * dest_dev, const void *src, size_t size) {
    memcpy((void *) dest, (const void *) src, size);
    return 0;
}

int rex_asyncmemcpy_from_thsim(void *dest, const void *src, rex_device_t * src_dev, size_t size) {
    memcpy((void *) dest, (const void *) src, size);
    return 0;
}


