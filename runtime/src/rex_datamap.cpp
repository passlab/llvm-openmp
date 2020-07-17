//
// Created by yan on 1/6/17.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "rex.h"

// Initialize datamap_info
void rex_datamap_init_info(const char *symbol, rex_datamap_info_t *info, rex_wunit_info_t *wunit_info,
                            void *source_ptr, int num_dims, int sizeof_element, rex_datamap_direction_t map_direction,
                            rex_datamap_type_t map_type) {
	if (num_dims > 3) {
		fprintf(stderr, "%d dimension array is not supported in this implementation!\n", num_dims);
		exit(1);
	}
	info->symbol = symbol;
	info->wunit_info = wunit_info;
	info->source_ptr = (char *)source_ptr;
	info->num_dims = num_dims;
	//memset(info->maps, 0, sizeof(rex_datamap_t) * wunit_info->top->nnodes);
	info->map_direction = map_direction;
	info->map_type = map_type;
	info->num_halo_dims = 0;
	info->sizeof_element = sizeof_element;
	info->remap_needed = 0;
	int i;
	for (i=0; i<num_dims; i++) {
		info->dist_info[i].target_type = REX_DIST_TARGET_DATA_MAP;
		info->dist_info[i].target = info;
		info->dist_info[i].target_dim = i; /* we donot know which dim this dist is applied to, also dist is an array, making default here */
	}
}

void rex_datamap_info_set_dims_1d(rex_datamap_info_t * info, long dim0) {
	info->dims[0] = dim0;
}

void rex_datamap_info_set_dims_2d(rex_datamap_info_t * info, long dim0, long dim1) {
	info->dims[0] = dim0;
	info->dims[1] = dim1;
}
void rex_datamap_info_set_dims_3d(rex_datamap_info_t * info, long dim0, long dim1, long dim2) {
	info->dims[0] = dim0;
	info->dims[1] = dim1;
	info->dims[2] = dim2;
}

void rex_datamap_info_set_dims(rex_datamap_info_t * info, long *dims, int num_dims) {
	int i;
	for (i=0; i<num_dims; i++)
	info->dims[0] = dims[i];
}

rex_datamap_t *rex_datamap_offcache_iterator(rex_wunit_t *off, int index, int * inherited) {
	if (index >= off->num_maps) return NULL;
	*inherited = off->map_cache[index].inherited;
	return off->map_cache[index].map;
}

static rex_datamap_t *rex_datamap_get_from_cache(rex_wunit_t *off, void *host_ptr) {
	int i;
	for (i=0; i<off->num_maps; i++) {
		rex_datamap_t * map = off->map_cache[i].map;
		if (map->info->source_ptr == host_ptr) return map;
	}

	return NULL;
}

void rex_datamap_append_cache(rex_wunit_t *wunit, rex_datamap_t *map, int inherited) {
	if (wunit->num_maps >= OFF_MAP_CACHE_SIZE) { /* error, report */
		fprintf(stderr, "map cache is full for off (%X), cannot add map %X\n", wunit, map);
		exit(1);
	}
	wunit->map_cache[wunit->num_maps].map = map;
	wunit->map_cache[wunit->num_maps].inherited = inherited;

	wunit->num_maps++;
}


int rex_datamap_is_inherited(rex_wunit_t *off, rex_datamap_t *map) {
	int i;
	for (i=0; i<off->num_maps; i++) {
		if (off->map_cache[i].map == map) return off->map_cache[i].inherited;
	}

	fprintf(stderr, "map %X is not in the map cache of off %X, wrong info and misleading return value of this function\n", map, off);
	return 0;
}

/* get map from inheritance (off stack) */
rex_datamap_t * rex_datamap_get_inheritance (rex_device_t * dev, void * host_ptr) {
	int i;
	for (i=dev->offload_stack_top; i>=0; i--) {
		rex_wunit_t * ancestor_off = dev->offload_stack[i];
		rex_datamap_t * map = rex_datamap_get_from_cache(ancestor_off, host_ptr);
		if (map != NULL) {
			return map;
		}
	}
	return NULL;
}

/**
 * Given the host pointer (e.g. an array pointer), find the data map of the array onto a specific device,
 * which is provided as a off_loading_t object (the off_loading_t has devseq id as well as pointer to the
 * offloading_info object that a search may need to be performed. If a map_index is provided, the search will
 * be simpler and efficient, otherwise, it may be costly by comparing host_ptr with the source_ptr of each stored map
 * in the offloading stack (call chain)
 *
 * The call also put a map into off->map_cache if it is not in the cache
 */
rex_datamap_t * rex_datamap_get(rex_wunit_t *off, void * host_ptr, int map_index) {
	/* STEP 1: search from the cache first */
	rex_datamap_t * map = rex_datamap_get_from_cache(off, host_ptr);
	if (map != NULL) {
		return map;
	}

	/* STEP 2: if not in cache, do quick search if given by a map_index, and then do a thorough search. put in cache if finds it */
	rex_wunit_info_t * wunit_info = off->wunit_info;
	int devseqid = off->devseqid;
	if (map_index >= 0 && map_index <= wunit_info->num_mapped_vars) {  /* the fast and common path */
		map = &wunit_info->datamap_info[map_index].maps[devseqid];
		if (host_ptr == NULL || map->info->source_ptr == host_ptr) {
			/* append to the off->map_cache */
			rex_datamap_append_to_cache(off, map, 0);
			return map;
		}
	} else { /* thorough search for all the mapped variables */
		int i; rex_datamap_info_t * dm_info;
		for (i=0; i<wunit_info->num_mapped_vars; i++) {
			dm_info = &wunit_info->datamap_info[i];
			if (dm_info->source_ptr == host_ptr) { /* we found */
				map = &dm_info->maps[devseqid];
				//printf("find a match: %X\n", host_ptr);
				rex_datamap_append_to_cache(off, map, 0);
				//rex_print_datamap(map);
				return map;
			}
		}
	}

	/* STEP 3: seach the offloading stack if this inherits data map from previous data offloading */
	//printf("rex_datamap_get_map: off: %X, devid: %d, wunit_info: %X, host_ptr: %X\n", off, off->devseqid, wunit_info, host_ptr);
	map = rex_datamap_get_inheritance (off->dev, host_ptr);
	if (map != NULL) rex_datamap_append_to_cache(off, map, 1);

	return map;
}

void rex_datamap_add_halo_region(rex_datamap_info_t *info, int dim, int left, int right, rex_dist_halo_edging_type_t edging) {
	info->num_halo_dims++;
	info->halo_info[dim].left = left;
	info->halo_info[dim].right = right;
	info->halo_info[dim].edging = edging;
	info->halo_info[dim].topdim = info->dist_info[dim].dim_index;
}

/**
 * after initialization
 */
void rex_datamap_init(rex_datamap_t *map, rex_datamap_info_t *info, rex_device_t *dev) {
//	if (map->access_level >= REX_DATA_MAP_ACCESS_LEVEL_INIT) return;
	map->dev = dev; /* mainly use as cache so we save one pointer deference */
	map->mem_noncontiguous = 0;
	map->map_type = info->map_type;
	map->dist_counter = 0;
	map->next = NULL;
	map->total_map_size = 0;
	map->total_map_wextra_size = 0;

	if (map->map_type == REX_DATA_MAP_AUTO) {
		if (rex_device_mem_discrete(map->dev->mem_type)) {
			map->map_type = REX_DATA_MAP_COPY;
			//printf("COPY data map: %X, dev: %d\n", map, dev->id);
		} else { /* we can make it shared and we will do it */
			map->map_type = REX_DATA_MAP_SHARED;
			//printf("SHARED data map: %X, dev: %d\n", map, dev->id);
		}
	} else if (map->map_type == REX_DATA_MAP_SHARED && rex_device_mem_discrete(dev->mem_type)) {
		fprintf(stderr, "direct sharing data between host and the dev: %d is not possible with discrete mem space, we use COPY approach now\n", dev->id);
		map->map_type = REX_DATA_MAP_COPY;
	}
	int i;
	for (i=0; i<info->num_dims; i++) {
		map->map_dist[i].counter = 0;
		map->map_dist[i].next = NULL;
		if (map->info == NULL) map->map_dist[i].acc_total_length = 0; /* a strange way to use this as a flag */
		map->map_dist[i].total_length = 0;
		map->map_dist[i].info = &info->dist_info[i];
	}
	map->info = info;
	map->access_level = REX_DATA_MAP_ACCESS_LEVEL_INIT;
}

long rex_loop_get_range(rex_wunit_t *off, int loop_level, long *offset, long *length) {
	if (!off->loop_dist_done)
		rex_loop_iteration_dist(off);
	*offset = 0;
	*length = off->loop_dist[loop_level].length;
	return off->loop_dist[loop_level].offset;
}

/**
 * Calculate row-major array element offset from the beginning of an array given the multi-dimensional index of an element,
 */
long rex_datamap_element_offset(rex_datamap_t * map) {
	int ndims = map->info->num_dims;
	long * dims = map->info->dims;
	int i;
	long off = 0;
	long mt = 1;
	for (i=ndims-1; i>=0; i--) {
		off += mt * map->map_dist[i].offset;
		mt *= dims[i];
	}
	return off;
}

/**
 * this function creates host buffer, if needed, and marshall data to the host buffer,
 *
 * it will also create device memory region (both the array region memory and halo region memory
 */
void rex_datamap_malloc(rex_datamap_t *map, rex_wunit_t *off) {
    int i;
	rex_datamap_info_t *map_info = map->info;
	rex_wunit_info_t * wunit_info = map_info->wunit_info;
	rex_grid_topology_t * top = wunit_info->top;
	int sizeof_element = map_info->sizeof_element;

	if (map->map_type == REX_DATA_MAP_SHARED) {
		map->map_dev_ptr = map->map_source_ptr;
		map->map_dev_wextra_ptr = map->map_source_wextra_ptr;
	} else if (map->map_type == REX_DATA_MAP_COPY) {
		map->map_dev_wextra_ptr = (char *)rex_datamap_malloc_dev(map->dev, map->map_source_wextra_ptr, map->map_wextra_size);
		map->map_dev_ptr = map->map_dev_wextra_ptr; /* this will be updated if there is halo region */
	} else {
		printf("unknown map type at this stage: map: %X, %d\n", map, map->map_type);
		abort();
	}

	/*
	* The halo memory management use an attached approach, i.e. the halo region is part of the main computation subregion, and those
	* left/right in/out are buffers for gathering and scattering halo region elements to its correct location.
	*
	* We may use a detached approach, i.e. just use the halo buffer for computation, however, that will involve more complicated
	* array index calculation.
	*/

	/*********************************************************************************************************************************************************/
	/************************* Barrier may be needed among all participating devs since we are now using neighborhood devs **********************************/
	/*********************************************************************************************************************************************************/

	/* TODO: so far, we only handle row-wise halo region in the first dimension of an array, i.e. contiginous memory space */
	if (map_info->num_halo_dims) {
		/* TODO: assert map->map_size != map->map_wextra_size; */
		if (map->mem_noncontiguous) {
			printf("So far, we only handle row-wise halo region in the first dimension of an array\n");
			abort();
		}
		//		BEGIN_SERIALIZED_PRINTF(off->devseqid);
		/* mem size of a row */
		long row_size = sizeof_element;
		for (i = 1; i < map_info->num_dims; i++) {
			row_size *= map_info->dims[i];
		}
		rex_datamap_halo_region_info_t *halo_info = &map_info->halo_info[0];
		rex_datamap_halo_region_mem_t *halo_mem = &map->halo_mem[0];
		halo_mem->left_in_size = halo_info->left * row_size;
		map->map_dev_ptr = map->map_dev_wextra_ptr + halo_mem->left_in_size;
		halo_mem->left_in_ptr = map->map_dev_wextra_ptr;
		halo_mem->left_out_size = halo_info->right * row_size;
		halo_mem->left_out_ptr = map->map_dev_ptr;
		//printf("dev: %d, halo left in size: %d, left in ptr: %X, left out size: %d, left out ptr: %X\n", off->devseqid,
		//	   halo_mem->left_in_size,halo_mem->left_in_ptr,halo_mem->left_out_size,halo_mem->left_out_ptr);
		if (halo_mem->left_dev_seqid >= 0) {
			rex_device_t *leftdev = &rex_devices[top->idmap[halo_mem->left_dev_seqid]];
			if (!rex_datamap_enable_memcpy_DeviceToDevice(leftdev, map->dev)) { /* no peer2peer access available, use host relay */
#define USE_HOSTMEM_AS_RELAY 1
#ifndef USE_HOSTMEM_AS_RELAY
				/** FIXME, mem leak here and we have not thought where to free */
				halo_mem->left_in_host_relay_ptr = (char *) malloc(halo_mem->left_in_size);
#else
				/* we can use the host memory for the halo region as the relay buffer */
				halo_mem->left_in_host_relay_ptr = map->map_source_wextra_ptr;
#endif
				halo_mem->left_in_data_in_relay_pushed = 0;
				halo_mem->left_in_data_in_relay_pulled = 0;

			//	printf("dev: %d, map: %X, left: %d, left host relay buffer allocated\n", off->devseqid, map, halo_mem->left_dev_seqid);
			} else {
			//	printf("dev: %d, map: %X, left: %d, left dev: p2p enabled\n", off->devseqid, map, halo_mem->left_dev_seqid);
				halo_mem->left_in_host_relay_ptr = NULL;
			}
		} //else map->map_dev_ptr = map->map_dev_wextra_ptr;

		halo_mem->right_in_size = halo_info->right * row_size;
		halo_mem->right_in_ptr = &((char *) map->map_dev_ptr)[map->map_size];
		halo_mem->right_out_size = halo_info->left * row_size;
		halo_mem->right_out_ptr = &((char *) halo_mem->right_in_ptr)[0 - halo_mem->right_out_size];
		//printf("dev: %d, halo right in size: %d, right in ptr: %X, right out size: %d, right out ptr: %X\n", off->devseqid,
		//	   halo_mem->right_in_size,halo_mem->right_in_ptr,halo_mem->right_out_size,halo_mem->right_out_ptr);
		if (halo_mem->right_dev_seqid >= 0) {
			rex_device_t *rightdev = &rex_devices[top->idmap[halo_mem->right_dev_seqid]];
			if (!rex_datamap_enable_memcpy_DeviceToDevice(rightdev, map->dev)) { /* no peer2peer access available, use host relay */
#ifndef USE_HOSTMEM_AS_RELAY
				/** FIXME, mem leak here and we have not thought where to free */
				halo_mem->right_in_host_relay_ptr = (char *) malloc(halo_mem->right_in_size);
#else
				/* we can use the host memory for the halo region as the relay buffer */
				halo_mem->right_in_host_relay_ptr = &((char *) map->map_source_ptr)[map->map_size];
#endif
				halo_mem->right_in_data_in_relay_pushed = 0;
				halo_mem->right_in_data_in_relay_pulled = 0;

			//	printf("dev: %d, map: %X, right: %d, right host relay buffer allocated\n", off->devseqid, map, halo_mem->right_dev_seqid);
			} else {
			//	printf("dev: %d, map: %X, right: %d, right host p2p enabled\n", off->devseqid, map, halo_mem->right_dev_seqid);
				halo_mem->right_in_host_relay_ptr = NULL;
			}
		}
		//		END_SERIALIZED_PRINTF();
	}

	map->access_level = REX_DATA_MAP_ACCESS_LEVEL_MALLOC;
}

/**
 * seqid is the sequence id of the device in the top, it is also used as index to access maps
 */
void rex_datamap_free(rex_datamap_t *map, rex_wunit_t *off) {
	if (map->map_type == REX_DATA_MAP_COPY)
		rex_datamap_free_dev(map->dev, map->map_dev_wextra_ptr, 0);
}

void rex_datamap_print_info(rex_datamap_info_t * info) {
	int i;
	printf("MAPS for %s", info->symbol);
	for (i=0; i<info->num_dims;i++) printf("[%ld]", info->dims[i]);
	printf(", ");
	if (info->num_halo_dims) {
		printf("halo");
		rex_datamap_halo_region_info_t * halo_info = info->halo_info;
		for (i=0; i<info->num_dims;i++) printf("[%d|%d]", halo_info[i].left, halo_info[i].right);
		printf(", ");
	}
	printf("direction: %d, map_type: %d, ptr: %X\n", info->map_direction, info->map_type, info->source_ptr);
	for (i=0; i<info->wunit_info->top->nnodes; i++) {
		printf("\t%d, ", i);
		rex_datamap_print(&info->maps[i]);
	}
}

void rex_datamap_print(rex_datamap_t * map) {
	rex_datamap_info_t * info = map->info;
	char * mem = "COPY";
	if (map->map_type == REX_DATA_MAP_SHARED) {
		mem = "SHARED";
	}
	printf("dev %d(%s), %s, ", map->dev->id, rex_get_device_typename(map->dev), mem);
	int soe = info->sizeof_element;
	int i;
	//for (i=0; i<info->num_dims;i++) printf("[%d:%d]", map->map_dist[i].offset, map->map_dist[i].offset+map->map_dist[i].length-1);
	for (i=0; i<info->num_dims;i++) printf("[%ld:%ld]", map->map_dist[i].offset, map->map_dist[i].total_length);

	printf(", size: %ld, size wextra: %ld (accumulated %d times)\n",map->total_map_size, map->total_map_wextra_size, map->dist_counter);
	printf("\t\tsrc ptr: %X, src wextra ptr: %X, dev ptr: %X, dev wextra ptr: %X (last)\n", map->map_source_ptr, map->map_source_wextra_ptr, map->map_dev_ptr, map->map_dev_wextra_ptr);
	if (info->num_halo_dims) {
		//printf("\t\thalo memory:\n");
		rex_datamap_halo_region_mem_t * all_halo_mems = map->halo_mem;
		for (i=0; i<info->num_dims; i++) {
			rex_datamap_halo_region_mem_t * halo_mem = &all_halo_mems[i];
			printf("\t\t%d-d halo, L_IN: %X[%ld], L_OUT: %X[%ld], R_OUT: %X[%ld], R_IN: %X[%ld]", i,
				   halo_mem->left_in_ptr, halo_mem->left_in_size/soe, halo_mem->left_out_ptr, halo_mem->left_out_size/soe,
				   halo_mem->right_out_ptr, halo_mem->right_out_size/soe, halo_mem->right_in_ptr, halo_mem->right_in_size/soe);
			//printf(", L_IN_relay: %X, R_IN_relay: %X", halo_mem->left_in_host_relay_ptr, halo_mem->right_in_host_relay_ptr);
			printf("\n");
		}
	}
}

void rex_print_off_maps(rex_wunit_t * off) {
	int i;
	printf("off %X maps: ", off);
	for (i=0; i<off->num_maps; i++) printf("%X, ", off->map_cache[i]);
	printf("\n");
}

/* do a halo regin pull for data map of devid. If top is not NULL, devid will be translated to coordinate of the
 * virtual topology and the halo region pull will be based on this coordinate.
 * @param: int dim[ specify which dimension to do the halo region update.
 *      If dim < 0, do all the update of map dimensions that has halo region
 * @param: from_left_right, to do in which direction

 *
 */
#define CORRECTNESS_CHECK 1
#undef CORRECTNESS_CHECK
void rex_halo_region_pull(rex_datamap_t * map, int dim, rex_datamap_exchange_direction_t from_left_right) {
	rex_datamap_info_t * info = map->info;
	/*FIXME: let us only handle 2-D array now */
	if (dim != 0 || map->mem_noncontiguous) {
		//fprintf(stderr, "we only handle noncontiguous distribution and halo at dimension 0 so far!\n");
		rex_datamap_print_info(map->info);
		return;
	}

	rex_datamap_halo_region_mem_t * halo_mem = &map->halo_mem[dim];
#if CORRECTNESS_CHECK
    BEGIN_SERIALIZED_PRINTF(map->dev->id);
	printf("dev: %d, map: %X, left: %d, right: %d\n", map->dev->id, map, halo_mem->left_dev_seqid, halo_mem->right_dev_seqid);
#endif

	if (halo_mem->left_dev_seqid >= 0 && (from_left_right == REX_DATA_MAP_EXCHANGE_FROM_LEFT_ONLY || from_left_right == REX_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT)) { /* pull from left */
		rex_datamap_t * left_map = &info->maps[halo_mem->left_dev_seqid];
		rex_datamap_halo_region_mem_t * left_halo_mem = &left_map->halo_mem[dim];

		/* if I need to push right_out data to the host relay buffer for the left_map, I should do it first */
		if (left_halo_mem->right_in_host_relay_ptr != NULL) {
			/* wait make sure the data in the right_in_host_relay buffer is already pulled */
			while (left_halo_mem->right_in_data_in_relay_pushed > left_halo_mem->right_in_data_in_relay_pulled);
			//if (map->map_type == REX_DATA_MAP_COPY || left_map->map_type == REX_DATA_MAP_COPY)
			rex_datamap_memcpy_from((void*)left_halo_mem->right_in_host_relay_ptr, (void*)halo_mem->left_out_ptr, map->dev, halo_mem->left_out_size);
			left_halo_mem->right_in_data_in_relay_pushed ++;
		} else {
			/* do nothing here because the left_map helper thread will do a direct device-to-device pull */
		}

		if (halo_mem->left_in_host_relay_ptr == NULL) { /* no need host relay */
			rex_datamap_memcpy_DeviceToDevice((void*)halo_mem->left_in_ptr, map->dev, (void*)left_halo_mem->right_out_ptr, left_map->dev, halo_mem->left_in_size);
#if CORRECTNESS_CHECK
			printf("dev: %d, dev2dev memcpy from left: %X <----- %X\n", map->dev->id, halo_mem->left_in_ptr, left_halo_mem->right_out_ptr);
#endif
		} else { /* need host relay */
			/*
			rex_set_current_device_dev(left_map->dev);
			rex_datamap_memcpy_from(halo_mem->left_in_host_relay_ptr, left_map->halo_mem[dim].right_out_ptr, left_map->dev, halo_mem->left_in_size);
			rex_set_current_device_dev(map->dev);
			*/
			while (halo_mem->left_in_data_in_relay_pushed <= halo_mem->left_in_data_in_relay_pulled); /* wait for the data to be ready in the relay buffer on host */
			/* wait for the data in the relay buffer is ready */
			rex_datamap_memcpy_to((void*)halo_mem->left_in_ptr, map->dev, (void*)halo_mem->left_in_host_relay_ptr, halo_mem->left_in_size);
			halo_mem->left_in_data_in_relay_pulled++;
		}
	}
	if (halo_mem->right_dev_seqid >= 0 && (from_left_right == REX_DATA_MAP_EXCHANGE_FROM_RIGHT_ONLY || from_left_right == REX_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT)) {
		rex_datamap_t * right_map = &info->maps[halo_mem->right_dev_seqid];
		rex_datamap_halo_region_mem_t * right_halo_mem = &right_map->halo_mem[dim];

		/* if I need to push left_out data to the host relay buffer for the right_map, I should do it first */
		if (right_halo_mem->left_in_host_relay_ptr != NULL) {
			while (right_halo_mem->left_in_data_in_relay_pushed > right_halo_mem->left_in_data_in_relay_pulled);
			rex_datamap_memcpy_from((void*)right_halo_mem->left_in_host_relay_ptr, (void*)halo_mem->right_out_ptr, map->dev, halo_mem->right_out_size);
			right_halo_mem->left_in_data_in_relay_pushed ++;
		} else {
			/* do nothing here because the left_map helper thread will do a direct device-to-device pull */
		}

		if (halo_mem->right_in_host_relay_ptr == NULL) {
			rex_datamap_memcpy_DeviceToDevice((void*)halo_mem->right_in_ptr, map->dev, (void*)right_halo_mem->left_out_ptr, right_map->dev, halo_mem->right_in_size);
#if CORRECTNESS_CHECK
			printf("dev: %d, dev2dev memcpy from right: %X <----- %X\n", map->dev->id, halo_mem->right_in_ptr, right_halo_mem->left_out_ptr);
#endif

		} else {
			/*
			rex_set_current_device_dev(right_map->dev);
			rex_datamap_memcpy_from(halo_mem->right_in_host_relay_ptr, right_map->halo_mem[dim].left_out_ptr, right_map->dev, halo_mem->right_in_size);
			rex_set_current_device_dev(map->dev);
			*/
			while (halo_mem->right_in_data_in_relay_pushed <= halo_mem->right_in_data_in_relay_pulled); /* wait for the data to be ready in the relay buffer on host */
			rex_datamap_memcpy_to((void*)halo_mem->right_in_ptr, map->dev, (void*)halo_mem->right_in_host_relay_ptr, halo_mem->right_in_size);
			halo_mem->right_in_data_in_relay_pulled++;
		}
	}
#if CORRECTNESS_CHECK
	END_SERIALIZED_PRINTF();
#endif
	return;
}

#undef CORRECTNESS_CHECK

void rex_datamap_unmarshal(rex_datamap_t * map) {
	if (!map->mem_noncontiguous) return;
	rex_datamap_info_t * info = map->info;
	int sizeof_element = info->sizeof_element;
	int i;
	switch (info->num_dims) {
		case 1: {
			fprintf(stderr, "data unmarshall can only do 2-d or 3-d array, currently is 1-d\n");
			break;
		}
		case 2: {
			long region_line_size = map->map_dist[1].length*sizeof_element;
			long full_line_size = info->dims[1]*sizeof_element;
			long region_off = 0;
			long full_off = 0;
			char * src_ptr = &info->source_ptr[sizeof_element*info->dims[1]*map->map_dist[0].offset + sizeof_element*map->map_dist[1].offset];
			for (i=0; i<map->map_dist[0].length; i++) {
				memcpy((void*)&src_ptr[full_off], (void*)&map->map_source_ptr[region_off], region_line_size);
				region_off += region_line_size;
				full_off += full_line_size;
			}
			break;
		}
		case 3: {
			break;
		}
		default: {
			fprintf(stderr, "data unmarshall can only do 2-d or 3-d array\n");
			break;
		}
	}
//	printf("total %ld bytes of data unmarshalled\n", region_off);
}

/**
 *  so far works for at most 2D
 */
void * rex_datamap_marshal(rex_datamap_t *map) {
	rex_datamap_info_t * info = map->info;
	int sizeof_element = info->sizeof_element;
	int i;
	switch (info->num_dims) {
		case 1: {
			fprintf(stderr,
					"data marshall can only do 2-d or 3-d array, currently is 1-d\n");
			break;
		}
		case 2: {
			char *map_buffer = (char*) malloc(map->map_size);
			long region_line_size = map->map_dist[1].length * sizeof_element;
			long full_line_size = info->dims[1] * sizeof_element;
			long region_off = 0;
			long full_off = 0;
			char * src_ptr = &info->source_ptr[sizeof_element * info->dims[1] * map->map_dist[0].offset
											   + sizeof_element * map->map_dist[1].offset];
			for (i = 0; i < map->map_dist[0].length; i++) {
				memcpy((void*)&map_buffer[region_off], (void*)&src_ptr[full_off], region_line_size);
				region_off += region_line_size;
				full_off += full_line_size;
			}
			return map_buffer;
			break;
		}
		case 3: {
			break;
		}
		default: {
			fprintf(stderr, "data marshall can only do 2-d or 3-d array\n");
			break;
		}
	}

	return NULL;
//	printf("total %ld bytes of data marshalled\n", region_off);
}

