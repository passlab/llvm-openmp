#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "rex.h"

/*
 * Intel compiler support for mic is required
 */

int rex_probe_dev_micomp() {
    return 0;
}

void rex_init_dev_micomp(rex_device_t *dev, int id, int sysid, int num_cores) {
    dev->type = REX_DEVICE_ITLMIC_OPENMP;
    dev->id = id;
    dev->sysid = sysid;
    dev->default_stream = NULL;
    dev->mem_type = REX_DEVICE_MEM_DISCRETE;
    dev->num_cores = num_cores;
}

void rex_warmup_dev_micomp(rex_device_t *dev) {
    int i;
    int SIZE = 1024;
    int dummy[SIZE];
    #pragma offload target(mic:dev->sysid) in (dummy:length(SIZE)) out(dummy:length(SIZE))
    {
	#pragma omp parallel for simd
    	for (i=0; i<SIZE; i++){
	    dummy[i] = dummy[i] * 7;
    	}
    }
}

int rex_set_current_dev_micomp(rex_device_t *d) {
    return d->id;
}

void rex_fini_dev_micomp(rex_device_t * dev) {
}


