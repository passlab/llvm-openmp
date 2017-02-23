
/**
 * Contains device-independent code for bridging into device-specific code
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "rex.h"
#include "rex_thirdparty/iniparser.h"

char * rex_get_device_typename(rex_device_t * dev) {
    int i;
    for (i=0; i<REX_NUM_DEVICE_TYPES; i++) {
        if (rex_device_types[i].type == dev->type) return rex_device_types[i].shortname;
    }
    return NULL;
}

/* OpenMP 4.0 support */
int default_device_var = -1;

void rex_set_default_device(int device_num) {
    default_device_var = device_num;
}

int rex_get_default_device(void) {
    return default_device_var;
}

int rex_get_num_devices() {
    return rex_num_devices;
}

int rex_num_devices;
rex_device_t *rex_devices;

volatile int rex_printf_turn = 0;
/* a simple mechanism to allow multiple dev shepherd threads to print in turn so the output do not scramble together */
rex_device_type_info_t rex_device_types[REX_NUM_DEVICE_TYPES] = {
        {REX_DEVICE_HOSTCPU_KMP,      "REX_DEVICE_HOSTCPU_KMP",     "HOSTCPU",      1},
        {REX_DEVICE_NVGPU_CUDA,       "REX_DEVICE_NVGPU_CUDA",      "NVCUDA",       0},
        {REX_DEVICE_NVGPU_OPENCL,     "REX_DEVICE_NVGPU_OPENCL",    "NVCL",         0},
        {REX_DEVICE_ITLGPU_OPENCL,    "REX_DEVICE_ITLGPU_OPENCL",   "ITLGPUCL",     0},
        {REX_DEVICE_AMDGPU_OPENCL,    "REX_DEVICE_AMDGPU_OPENCL",   "AMDGPUCL",     0},
        {REX_DEVICE_AMDAPU_OPENCL,    "REX_DEVICE_AMDAPU_OPENCL",   "AMDAPUCL",     0},
        {REX_DEVICE_ITLMIC_OPENMP,    "REX_DEVICE_ITLMIC_OPENMP",   "ITLMICOMP",    0},
        {REX_DEVICE_ITLMIC_OPENCL,    "REX_DEVICE_ITLMIC_OPENCL",   "ITLMICCL",     0},
        {REX_DEVICE_THSIM_PTHREAD,    "REX_DEVICE_THSIM_PTHREAD",   "THSIM",        0},
        {REX_DEVICE_TIDSP,            "REX_DEVICE_TIDSP",           "TIDSP",        0},
        {REX_DEVICE_REMOTE_MPI,       "REX_DEVICE_REMOTE_MPI",      "MPI",          0},
};

/* APIs to support multiple devices: */
char *rex_supported_device_types() { /* return a list of devices supported by the compiler in the format of TYPE1:TYPE2 */
    /* FIXME */
    return "REX_DEVICE_HOSTCPU";
}

rex_device_type_t rex_get_device_type(int devid) {
    return rex_devices[devid].type;
}

char *rex_get_device_type_as_string(int devid) {
    return rex_device_types[rex_devices[devid].type].name;
}

/* current omp has rex_get_num_devices(); */
int rex_get_num_devices_of_type(rex_device_type_t type) {
    return rex_device_types[type].num_devs;
}

int rex_set_current_device(rex_device_t *d) {
    return d->id;
}

/*
 * return the first ndev device IDs of the specified type, the function returns the actual number of devices
 * in the array (devnum_array), it is <= to the total number of devices of the specified type
 *
 * before calling this function, the caller should allocate the devnum_array[ndev]
 */
int rex_get_devices(rex_device_type_t type, int *devnum_array, int ndev) { /* return a list of devices of the specified type */
    int i;
    int num = 0;
    for (i = 0; i < rex_num_devices; i++)
        if (rex_devices[i].type == type) {
            devnum_array[num] = rex_devices[i].id;
            num++;
            if (num == ndev) break;
        }
    return num;
}

rex_device_t *rex_get_device(int id) {
    return &rex_devices[id];
}

int rex_get_num_active_devices() {
    int num_dev;
    char *ndev = getenv("REX_NUM_ACTIVE_DEVICES");
    if (ndev != NULL) {
        num_dev = atoi(ndev);
        if (num_dev == 0 || num_dev > rex_num_devices) num_dev = rex_num_devices;
    } else {
        num_dev = rex_num_devices;
    }
    return num_dev;
}

static void rex_util_copy_device_object(rex_device_t *newone, rex_device_t *src, int newid, int newsysid) {
    memcpy(newone, src, sizeof(rex_device_t));
    newone->id = newid;
    newone->sysid = newsysid;
    newone->default_stream = NULL;
}

static int num_hostcpu_dev = 0;
static int num_thsim_dev = 0; /* the thsim actually */
/* for NVDIA GPU devices */
static int num_nvgpu_dev = 0;
static int num_itlgpu_dev = 0;
static int num_itlmic_dev = 0;

/* we allow the same type of device to be specified using a simple form, e.g.
 *
 *
[cpu]
num = 10;
id = 0
type = cpu
ncores = 8
FLOPss = 2 # GFLOPS/s
Bandwidth = 600000 # MB/s
Latency = 0 #us
 */
static void rex_read_device_spec(char *dev_spec_file) {
    dictionary *ini;
    ini = iniparser_load(dev_spec_file);
    if (ini == NULL) {
        fprintf(stderr, "cannot parse file: %s\n", dev_spec_file);
        abort();
    }
    //iniparser_dump(ini, stderr);
    int num_sections = iniparser_getnsec(ini);
    int i;
    /* count total number of devices */
    rex_num_devices = 0;
    char devname[32];
    char keyname[48];
    for (i = 0; i < num_sections; i++) {
        sprintf(devname, "%s", iniparser_getsecname(ini, i));
        sprintf(keyname, "%s:%s", devname, "num");
        int num_devs = iniparser_getint(ini, keyname, 1);
        if (num_devs > 0) rex_num_devices += num_devs;
    }
    rex_devices = (rex_device_t*)malloc(sizeof(rex_device_t) * (rex_num_devices));

    int devid = 0;
    for (i = 0; i < num_sections; i++) {
        sprintf(devname, "%s", iniparser_getsecname(ini, i));
        sprintf(keyname, "%s:%s", devname, "num");
        int num_devs = iniparser_getint(ini, keyname, 1);
        if (num_devs <= 0) continue;

        rex_device_t *dev = &rex_devices[devid];

        sprintf(devname, "%s", iniparser_getsecname(ini, i));
        sprintf(keyname, "%s:%s", devname, "sysid");
        int devsysid = iniparser_getint(ini, keyname, -1);
        sprintf(dev->name, "%s:%d", devname, devsysid);
        sprintf(keyname, "%s:%s", devname, "ncores");
        int num_cores = iniparser_getint(ini, keyname, 1);
        char *devtype;
        sprintf(keyname, "%s:%s", devname, "type");
        devtype = iniparser_getstring(ini, keyname, "NULL");

        if (strcasecmp(devtype, "cpu") == 0 || strcasecmp(devtype, "hostcpu") == 0) {
            rex_init_dev_hostcpu(dev, devid, devsysid, num_cores);
            num_hostcpu_dev += num_devs;
#ifdef REX_NVCUDA_SUPPORT
        } else if (strcasecmp(devtype, "nvgpu") == 0) {
            rex_init_dev_nvcuda(dev, devid, devsysid);
            num_nvgpu_dev += num_devs;
#endif
        } else if (strcasecmp(devtype, "thsim") == 0) {
            rex_init_dev_thsim(dev, devid, devsysid, num_cores);
            num_thsim_dev += num_devs;
#ifdef REX_ITLGPU_SUPPORT
        } else if (strcasecmp(devtype, "itlgpu") == 0) {
            rex_init_dev_opencl(dev, devid, devsysid, num_cores);
            num_itlgpu_dev += num_devs;
#endif
#ifdef REX_ITLMIC_SUPPORT
        } else if (strcasecmp(devtype, "itlmic") == 0) {
            rex_init_dev_micomp(dev, devid, devsysid, num_cores);
            num_itlmic_dev += num_devs;
#endif
        } else {
            printf("unknow device type error: %s \n, default to be hostcpu\n", devtype);
            /* unknow device type error */
        }

        sprintf(keyname, "%s:%s", devname, "flopss");
        dev->total_real_flopss = iniparser_getdouble(ini, keyname, -1);

        sprintf(keyname, "%s:%s", devname, "Bandwidth");
        dev->bandwidth = iniparser_getdouble(ini, keyname, -1);

        sprintf(keyname, "%s:%s", devname, "Latency");
        dev->latency = iniparser_getdouble(ini, keyname, 0.00000000001);

        sprintf(keyname, "%s:%s", devname, "Memory");
        char *mem = iniparser_getstring(ini, keyname, "default"); /* or shared */
        if (strcasecmp(mem, "shared") == 0) {
            dev->mem_type = REX_DEVICE_MEM_SHARED;
        } else if (strcasecmp(mem, "discrete") == 0) {
            dev->mem_type = REX_DEVICE_MEM_DISCRETE;
        } else {
            /* using default, already done in init_*_device call */
        }

        devid++;
        /* repeating the same type of devices */
        int j;
        for (j = 1; j < num_devs; j++) {
            rex_device_t *newd = &rex_devices[devid];
            rex_util_copy_device_object(newd, dev, devid, devsysid + j);
            sprintf(newd->name, "%s:%d", devname, devsysid + j);
            devid++;
        }
    }

    iniparser_freedict(ini);
}

/* init the device objects, num_of_devices, helper threads, default_device_var ICV etc
 *
 */
int rex_init_devices() {
    /* query hardware device */
    rex_num_devices = 0; /* we always have at least host device */

    int i;

    char *dev_spec_file = getenv("REX_DEV_SPEC_FILE");
    if (dev_spec_file != NULL) {
        rex_read_device_spec(dev_spec_file);
    } else { /* probe devices is not supported yet */
        fprintf(stderr, "REX_DEV_SPEC_FILE needs to be set to point to INI file for device specification\n");
        return -1;
    }
    if (rex_num_devices) {
        default_device_var = 0;
    } else {
        default_device_var = -1;
    }
    rex_device_types[REX_DEVICE_HOSTCPU_KMP].num_devs = num_hostcpu_dev;
    rex_device_types[REX_DEVICE_THSIM_PTHREAD].num_devs = num_thsim_dev;
    rex_device_types[REX_DEVICE_NVGPU_CUDA].num_devs = num_nvgpu_dev;
    rex_device_types[REX_DEVICE_ITLGPU_OPENCL].num_devs = num_itlgpu_dev;
    rex_device_types[REX_DEVICE_ITLMIC_OPENMP].num_devs = num_itlmic_dev;

    for (i = 0; i < rex_num_devices; i++) {
        rex_device_t *dev = &rex_devices[i];

        dev->status = 1;
        dev->offload_request = NULL;
        dev->offload_stack_top = -1;
    }

    printf("============================================================================================================================\n");
    printf("Total %d devices: %d HOSTCPU, %d NVGPU, %d ITLMIC, %d ITLGPU and %d THSIM; default dev: %d.\n",
           rex_num_devices,
           num_hostcpu_dev, num_nvgpu_dev, num_itlmic_dev, num_itlgpu_dev, num_thsim_dev, default_device_var);
    for (i = 0; i < rex_num_devices; i++) {
        rex_device_t *dev = &rex_devices[i];
        char *mem_type = "SHARED";
        if (dev->mem_type == REX_DEVICE_MEM_DISCRETE) {
            mem_type = "DISCRETE";
        }
        printf("  %d|sysid: %d, type: %s, name: %s, ncores: %d, mem: %s, flops: %0.2fGFLOPS/s, bandwidth: %.2fMB/s, latency: %.2fus\n",
               dev->id, dev->sysid, rex_get_device_typename(dev), dev->name, dev->num_cores, mem_type,
               dev->total_real_flopss, dev->bandwidth,
               dev->latency);
        //printf("\t\tstream dev: %s\n", dev->default_stream.dev->name);
        if (dev->type == REX_DEVICE_NVGPU_CUDA) {
            if (dev->mem_type == REX_DEVICE_MEM_DISCRETE) {
#if defined(DEVICE_NVGPU_CUDA_VSHAREDM)
			printf("\t\tUnified Memory is supported in the runtime, but this device is not set to use it. To use it, enable shared mem in the dev spec(Memory=shared)\n");
#endif
            }
            if (dev->mem_type == REX_DEVICE_MEM_SHARED) {
#if defined(DEVICE_NVGPU_CUDA_VSHAREDM)
#else
                printf("\t\tUnified Memory is NOT supported in the runtime, fall back to discrete memory for this device. To enable shared mem support in runtime, set the DEVICE_NVGPU_CUDA_VSHAREDM macro.\n");
                dev->mem_type = REX_DEVICE_MEM_DISCRETE;
#endif
            }
        }
    }
    return rex_num_devices;
}

// terminate helper threads
void rex_fini_devices() {
    int i;

    for (i = 0; i < rex_num_devices; i++) {
        rex_device_t *dev = &rex_devices[i];
        rex_device_type_t devtype = dev->type;
        switch (devtype) {
            case REX_DEVICE_HOSTCPU_KMP :
                rex_fini_dev_hostcpu(dev);
                break;
#ifdef REX_NVCUDA_SUPPORT
            case REX_DEVICE_NVGPU_CUDA :
                rex_fini_dev_nvcuda(dev);
                break;
#endif
#ifdef REX_ITLMIC_SUPPORT
            case REX_DEVICE_ITLMIC_OPENMP :
                rex_fini_dev_micomp(dev);
                break;
#endif
#ifdef REX_ITLGPU_SUPPORT
            case REX_DEVICE_ITLGPU_OPENCL :
                rex_fini_dev_opencl(dev);
                break;
#endif
            default :
                break;
        }
        free(rex_devices);
    }

}


