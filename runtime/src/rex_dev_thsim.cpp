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


