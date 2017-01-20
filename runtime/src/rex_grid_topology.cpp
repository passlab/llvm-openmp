//
// Created by yan on 1/5/17.
//

#include <stdlib.h>
#include "rex.h"

/**
 * factor n into dims number of numbers whose multiplication equals to n
 */
void rex_factor(int n, int factor[], int dims) {
    switch (dims) {
        case 1:
        {
            factor[0] = n;
            return;
        }
        case 2:
        {
            switch (n) {
                case 1:
                case 2:
                case 3:
                case 5:
                case 7:
                case 11:
                case 13:
                {
                    factor[0] = n;
                    factor[1] = 1;
                    return;
                }
                case 4:
                case 6:
                case 8:
                case 10:
                case 14:
                {
                    factor[0] = n/2;
                    factor[1] = 2;
                    return;
                }
                case 9:
                case 15:
                {
                    factor[0] = n/3;
                    factor[1] = 3;
                    return;
                }
                case 12:
                case 16:
                {
                    factor[0] = n/4;
                    factor[1] = 4;
                    return;
                }
            }
            break;
        }
        case 3:
        {
            switch (n) {
                case 1:
                case 2:
                case 3:
                case 5:
                case 7:
                case 11:
                case 13:
                {
                    factor[0] = n;
                    factor[1] = 1;
                    factor[2] = 1;
                    return;
                }
                case 4:
                case 6:
                {
                    factor[0] = n/2;
                    factor[1] = 2;
                    factor[2] = 1;
                    return;
                }
                case 8:
                {
                    factor[0] = 2;
                    factor[1] = 2;
                    factor[2] = 2;
                    return;
                }
                default: break;
            }
            break;
        }
        default:
            fprintf(stderr, "more than 3 dimensions are not supported\n");
            break;
    }
}

/**
 * for a ndims-dimensional array, the dimensions are stored in dims array.
 * Given an element with index stored in idx array, this function return the offset
 * of that element in row-major. E.g. for an array [3][4][5], element [2][2][3] has offset 53
 */
int rex_top_offset(int ndims, int * dims, int * idx) {
    int i;
    int off = 0;
    int mt = 1;
    for (i=ndims-1; i>=0; i--) {
        off += mt * idx[i];
        mt *= dims[i];
    }
    return off;
}

/**
 * given a sequence id, return the top coordinates
 * the function return the actual number of dimensions
 */
int rex_topology_get_coords(rex_grid_topology_t * top, int sid, int ndims, int coords[]) {
    if (top->ndims > ndims) {
        fprintf(stderr, "the given ndims and array are too small\n");
        return -1;
    }
    int i, nnodes;
    nnodes = top->nnodes;
    for ( i=0; i < top->ndims; i++ ) {
        nnodes    = nnodes / top->dims[i];
        coords[i] = sid / nnodes;
        sid  = sid % nnodes;
    }

    ndims = i;
    return i;
}

/* return the sequence id of the coord
 */
int rex_grid_topology_get_seqid_coords(rex_grid_topology_t * top, int coords[]) {
/*
	// TODO: currently only for 2D
	if (top->ndims == 1) return top->idmap[coords[0]].devid;
	else if (top->ndims == 2) return top->idmap[coords[0]*top->dims[1] + coords[1]].devid;
	else return -1;
	*/
    return rex_top_offset(top->ndims, top->dims, coords);
}

int rex_grid_topology_get_seqid(rex_grid_topology_t * top, int devid) {
    int i;
    for (i=0; i<top->nnodes; i++)
        if (top->idmap[i] == devid) return i;

    return -1;
}

/**
 * simple and common topology setup, i.e. devid is from 0 to nnodes-1
 */
rex_grid_topology_t * rex_grid_topology_init_simple(int ndevs, int ndims) {
    /* idmaps array is right after the object in mem */
    rex_grid_topology_t * top = (rex_grid_topology_t*) malloc(sizeof(rex_grid_topology_t)+ sizeof(int)* ndevs);
    top->nnodes = ndevs;
    top->ndims = ndims;
    top->idmap = (int*)&top[1];
    rex_factor(ndevs, top->dims, ndims);
    int i;
    for (i=0; i<ndims; i++) {
        top->periodic[i] = 0;
    }

    for (i=0; i< ndevs; i++) {
        top->idmap[i] = i; /* rex_devices[i].id; */
    }
    return top;
}

/**
 * Init a topology of devices froma a list of devices identified in the array @devs of @ndevs devices
 */
rex_grid_topology_t * rex_grid_topology_init(int ndevs, int *devs, int ndims) {
    rex_grid_topology_t * top = (rex_grid_topology_t*) malloc(sizeof(rex_grid_topology_t)+ sizeof(int)*ndevs);
    top->nnodes = ndevs;
    top->ndims = ndims;
    top->idmap = (int*)&top[1];
    rex_factor(ndevs, top->dims, ndims);
    int i;
    for (i=0; i<ndims; i++) {
        top->periodic[i] = 0;
    }

    for (i=0; i<ndevs; i++) {
        top->idmap[i] = devs[i];
    }
    return top;
}

void rex_grid_topology_fini(rex_grid_topology_t * top) {
    free(top);
}

void rex_topology_print(rex_grid_topology_t * top) {
    printf("top: %p (%d): ", top, top->nnodes);
    int i;
    for(i=0; i<top->ndims; i++)
        printf("[%d]", top->dims[i]);
    printf("\n");
    for (i=0; i<top->nnodes; i++) {
        printf("%d->%d\n", i, top->idmap[i]);
    }
    printf("\n");
}

void rex_topology_pretty_print(rex_grid_topology_t * top, char * buffer) {
    int i, offset;
    for (i=0; i<top->nnodes; i++) {
        int devid = top->idmap[i];
        //rex_device_t * dev = &rex_devices[devid];
        offset = strlen(buffer);
        sprintf(buffer + offset, "%d(%s:%d)-", devid, /* rex_get_device_typename(dev) */ "device name", devid /* dev->sysid */);
    }
    offset = strlen(buffer);
    buffer[offset-1] = '\0'; /* remove the last - */
}

void rex_topology_get_neighbors(rex_grid_topology_t * top, int seqid, int topdim, int cyclic, int* left, int* right) {
    if (seqid < 0 || seqid > top->nnodes) {
        *left = -1;
        *right = -1;
        return;
    }
    int coords[top->ndims];
    rex_topology_get_coords(top, seqid, top->ndims, coords);

    int dimcoord = coords[topdim];
    int dimsize = top->dims[topdim];

    int leftdimcoord = dimcoord - 1;
    int rightdimcoord = dimcoord + 1;
    if (cyclic) {
        if (leftdimcoord < 0)
            leftdimcoord = dimsize - 1;
        if (rightdimcoord == dimsize)
            rightdimcoord = 0;
        coords[topdim] = leftdimcoord;
        *left = rex_grid_topology_get_seqid_coords(top, coords);
        coords[topdim] = rightdimcoord;
        *right = rex_grid_topology_get_seqid_coords(top, coords);
        return;
    } else {
        if (leftdimcoord < 0) {
            *left = -1;
            if (rightdimcoord == dimsize) {
                *right = -1;
                return;
            } else {
                coords[topdim] = rightdimcoord;
                *right = rex_grid_topology_get_seqid_coords(top, coords);
                return;
            }
        } else {
            coords[topdim] = leftdimcoord;
            *left = rex_grid_topology_get_seqid_coords(top, coords);
            if (rightdimcoord == dimsize) {
                *right = -1;
                return;
            } else {
                coords[topdim] = rightdimcoord;
                *right = rex_grid_topology_get_seqid_coords(top, coords);
                return;
            }
        }
    }
}
