#ifndef __REX_H__
#define __REX_H__

#define REX_VERSION_MAJOR    @REX_VERSION_MAJOR@
#define REX_VERSION_MINOR    @REX_VERSION_MINOR@

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/**************************** OpenMP standard support *******************************/
extern int default_device_var; /* -1 means no device, the runtime should be initialized this
 * to be 0 if there is at least one device */
/* the REX_DEFAULT_DEVICE env variable is also defined in 4.0, which set the default-device-var icv */
extern void omp_set_default_device(int device_num );
extern int omp_get_default_device(void);
extern int omp_get_num_devices();
extern int omp_is_initial_devices();
extern int omp_get_initial_devices(int);

/* stubs for device memory routines */
void * omp_target_alloc(size_t size, int device_num);
void omp_target_free(void * device_ptr, int device_num);
int omp_target_is_present(void * ptr, int device_num);
int omp_target_memcpy(void * dst, void * src, size_t length,
                      size_t dst_offset, size_t src_offset,
                      int dst_device_num, int src_device_num);
int omp_target_memcpy_rect(
        void * dst, void * src,
        size_t element_size,
        int num_dims,
        const size_t* volume,
        const size_t* dst_offsets,
        const size_t* src_offsets,
        const size_t* dst_dimensions,
        const size_t* src_dimensions,
        int dst_device_num, int src_device_num);
int omp_target_associate_ptr(void * host_ptr, void * device_ptr,
                             size_t size, size_t device_offset,
                             int device_num);
int omp_target_disassociate_ptr(void * ptr, int device_num);
/********************************************************************************************/

/* the max number of dimensions runtime support now for array and cart topology */
#define REX_MAX_NUM_DIMENSIONS 3
#define REX_ALL_DIMENSIONS -1
/* the LONG_MIN */
#define REX_ALIGNEE_OFFSET -2147483647
#define CACHE_LINE_SIZE 64

typedef struct rex_wunit rex_wunit_t;
typedef struct rex_wunit_info rex_wunit_info_t;
typedef struct rex_datamap rex_datamap_t;
typedef struct rex_datamap_info rex_datamap_info_t;
typedef struct rex_device rex_device_t;
typedef struct rex_place rex_place_t;
typedef struct rex_xthread rex_xthread_t;
typedef struct rex_event rex_event_t;

/** =================================================================================================================== *
 * a topology of devices, or threads or teams
 */
typedef struct rex_grid_topology {
	int nnodes;     /* Product of dims[*], gives the size of the topology */
	int ndims;
	int dims[REX_MAX_NUM_DIMENSIONS];
	int periodic[REX_MAX_NUM_DIMENSIONS];
	int * idmap; /* the seqid is the array index, each element is the mapped devid */
} rex_grid_topology_t;

/** ================================================================================================================== *
 * Places and HPT
 */

typedef enum rex_place_type {
	REX_PLACE_REGISTER,
	REX_PLACE_CACHE_L1D,
	REX_PLACE_CACHE_L1I,
	REX_PLACE_CACHE_L2I,
	REX_PLACE_CACHE_L2D,
	REX_PLACE_CACHE_L3,
	REX_PLACE_CACHE_L4,
	REX_PLACE_CACHE,
	REX_PLACE_SRAM_SCRATCHPAD,
	REX_PLACE_3DDRAM_SCRATCHPAD,
	REX_PLACE_SCRATCHPAD,
	REX_PLACE_3DDRAM,
	REX_PLACE_DRAM_NUMA_REGION,
    REX_PLACE_DRAM,
	REX_PLACE_NVDIMM,
	REX_PLACE_DEV_DRAM, /* for accelerator such as Nvidia GPU */
	REX_PLACE_REMOTEE, /* for remote place, such as an MPI process, etc */
	REX_PLACE_STORAGE_SSD,
	REX_PLACE_STORAGE_FS, /* general file system */
	NUM_OF_REX_PLACE_TYPES,
} rex_place_type_t;

struct rex_place {
	int id;
	int did; /* the mapping hardware module id */
	short type;
	int unitSize;
	int size;

    rex_device_t *dev; /* the hosting device*/

	/* the Hierarchical Place Trees */
	struct rex_place * parent;
	struct rex_place * child; /* the first child */
	struct rex_place * nnext; /* the sibling link of the HPT */

	/** another approach of forming the HPT
	 * Since we keep an array of all places and an array of all workers (each element is
	 * the pointer to the place/worker object, and the position in the array is the same as it array index.
	 * We can use place/worker id to form the HPT because. child_id is the id of the first place and all the
	 * children places are numbered consecutively, this is the same as how child workers are stored.
	 */
	int parent_id;
	int child_id;
	int worker_id;

	struct rex_deque ** deques; /* Each xth has a deque in each place. so the number of deque each place has is the same as the number of xth */
	int ndeques; /* number of deques. In current configuration, each worker thread has a deque in each place.  */

	/* the queue will be mostly scanned without locking, enqueue and dequeue needs to lock */
	//rex_ll_queue_t inbox_queue; /* use the rex_wunit_t's next field for a link-list queue for queuing incoming wunit, mainly parallel so far */

	/* the following fields are for caching the HPT info so runtime get information quick instead of traversaling the tree */
	int num_children; /* num of directly attached child places */
	int num_hpt_xth; /* number of worker threads in this sub HPT (this place is the root) */
	int num_xth; /* number of directly attached xth of this place */
	rex_xthread_t ** workers; /* directly attached xthread, the leaf place */
};

/** ================================================================================================================== *
 * dev support, particularly for offloading
 */

/**
 * multiple device support: the following should be a list of name agreed with vendors
 * NOTES: should we also have a device version number, e.g. a type of device
 * may have mulitple generations, thus versions.
 */
typedef enum rex_device_type {
	REX_DEVICE_HOSTCPU_KMP, /* the host cpu, KMP is the LLVM OpenMP runtime, this runtime*/
	REX_DEVICE_NVGPU_CUDA, /* NVIDIA GPGPUs accelerator, we only support CUDA, TODO: OpenCL */
	REX_DEVICE_NVGPU_OPENCL, /* NVIDIA GPGPUs accelerator, we only support CUDA, TODO: OpenCL */
	REX_DEVICE_ITLGPU_OPENCL, /* Intel integrated GPGPUs, TODO: we can only support OpenCL */
	REX_DEVICE_AMDGPU_OPENCL, /* AMD GPUs, TODO */
	REX_DEVICE_AMDAPU_OPENCL, /* AMD APUs, TODO */
	REX_DEVICE_ITLMIC_OPENMP, /* Intel MIC, only support icc */
	REX_DEVICE_ITLMIC_OPENCL, /* Intel MIC, only support icc */
	REX_DEVICE_THSIM_PTHREAD, /* a new thread of the same process, e.g. pthread */
	REX_DEVICE_TIDSP, /* TI DSP, TODO */
	REX_DEVICE_REMOTE_MPI, /* a remote node, TODO */
	REX_NUM_DEVICE_TYPES,  /* the total number of types of supported devices */
} rex_device_type_t;

typedef enum rex_device_mem_type { /* the mem type related to the host mem */
	REX_DEVICE_MEM_SHARED_CC_UMA, /* CC UMA */
	REX_DEVICE_MEM_SHARED_CC_NUMA, /* CC NUMA */
	REX_DEVICE_MEM_SHARED_NCC_UMA,
	REX_DEVICE_MEM_SHARED_NCC_NUMA,
	REX_DEVICE_MEM_SHARED,
	REX_DEVICE_MEM_VIRTUAL_AS, /* uniformed virtual address space */
	REX_DEVICE_MEM_DISCRETE, /* different memory and different memory space */
}rex_device_mem_type_t;

#define rex_device_mem_shared(mem_type) (mem_type <= REX_DEVICE_MEM_SHARED)
#define rex_device_mem_vas(mem_type) (mem_type == REX_DEVICE_MEM_VIRTUAL_AS)
#define rex_device_mem_discrete(mem_type) (mem_type == REX_DEVICE_MEM_DISCRETE)

extern char * rex_device_type_name[];
typedef struct rex_device_type_info {
	rex_device_type_t type;
	char name[64];
	char shortname[16];
	int num_devs;
} rex_device_type_info_t;
extern rex_device_type_info_t rex_device_types[];
/**
 ********************** Compiler notes *********************************************
 * The recommended compiler flag name to output the
 * list of supported device is "-omp-device-types". The use of "-omp-device-types"
 * could be also as follows to restrict the compiler to only support
 * compilation for the listed device types: -omp-device-types="TYPE1,TYPE2,...,TYPE3"
 */

/* APIs to support multiple devices: */
extern char * rex_supported_device_types(); /* return a list of devices supported by the compiler in the format of TYPE1:TYPE2 */
extern rex_device_type_t rex_get_device_type(int devid);
extern char * rex_get_device_type_as_string(int devid);
extern int rex_get_num_devices_of_type(rex_device_type_t type); /* current omp has rex_get_num_devices(); */
extern int rex_get_devices(rex_device_type_t type, int *devnum_array, int ndev); /* return a list of devices of the specified type */
extern rex_device_t * rex_get_device(int id);
extern int rex_num_devices;
extern rex_device_t * rex_devices; /* an array of all device objects */

extern int rex_init_devices();
extern void rex_fini_devices();

/* hostcpu specific functions, in rex_dev_hostcpu.cpp */
extern int rex_probe_dev_hostcpu();
extern void rex_init_dev_hostcpu(rex_device_t *dev, int id, int sysid, int num_cores);
extern void rex_warmup_dev_hostcpu(rex_device_t *dev);
extern int rex_set_current_dev_hostcpu(rex_device_t *d);
extern void rex_fini_dev_hostcpu(rex_device_t * dev);

/* thsim dev specific functions, in rex_dev_thsim.cpp */
extern int rex_probe_dev_thsim();
extern void rex_init_dev_thsim(rex_device_t *dev, int id, int sysid, int num_cores);
extern void rex_warmup_dev_thsim(rex_device_t *dev);
extern int rex_set_current_dev_thsim(rex_device_t *d);
extern void rex_fini_dev_thsim(rex_device_t * dev);

/* nvcuda dev specific functions, in rex_dev_nvcuda.cpp */
extern int rex_probe_dev_nvcuda();
extern void rex_init_dev_nvcuda(rex_device_t *dev, int id, int sysid);
extern void rex_warmup_dev_nvcuda(rex_device_t *dev);
extern int rex_set_current_dev_nvcuda(rex_device_t *d);
extern void rex_fini_dev_nvcuda(rex_device_t * dev);

/* micomp dev specific functions, in rex_dev_micomp.cpp */
extern int rex_probe_dev_micomp();
extern void rex_init_dev_micomp(rex_device_t *dev, int id, int sysid, int num_cores);
extern void rex_warmup_dev_micomp(rex_device_t *dev);
extern int rex_set_current_dev_micomp(rex_device_t *d);
extern void rex_fini_dev_micomp(rex_device_t * dev);

/* opencl dev specific functions, in rex_dev_opencl.cpp */
extern int rex_probe_dev_opencl();
extern void rex_init_dev_opencl(rex_device_t *dev, int id, int sysid, int num_cores);
extern void rex_warmup_dev_opencl(rex_device_t *dev);
extern int rex_set_current_dev_opencl(rex_device_t *d);
extern void rex_fini_dev_opencl(rex_device_t * dev);

/**
 * An opaque representation of a device-specific stream.
 * For NVCUDA, it is cudaStream_t
 * For OpenCL, it is cl_command_queue;
 */
typedef void * rex_dev_stream_t;

struct rex_device {
	int id; /* the id from rex view */
	int sysid; /* the handle from the system view, e.g.
			 device id for NVGPU cudaSetDevice(sysid),
			 or pthread_t for THSIM. Need type casting to become device-specific id */
	char name[64]; /* a short name for the sake of things */
	rex_device_type_t type;
	void * dev_properties; /* a pointer to the device-specific properties object */
	void * default_context;               // context

	void * software_driver; /* a pointer to the software driver support of this device, e.g. CUDA, OpenCL platform, etc */
	rex_device_mem_type_t mem_type; /* the mem access pattern relative to the host, e.g. shared or discrete */

	rex_place_t * hpt; /* the root place of the HPT subtree of the device */
    rex_place_t * default_place; /* the default place of the device, which may not be the root place of the device HPT */

	/* performance factor */
	unsigned long mem_size;
	unsigned long max_teams;
	unsigned long max_threads;

	unsigned long num_cores;
	unsigned long num_chips; /* SM for GPU, core pack for host */
	unsigned long core_frequency;
	unsigned int arch_factor; /* means to capture micro arch factor that impact performance, e.g. superscalar, deeper pipeline, etc */

	double bandwidth; /* between host memory and dev memory for profile data movement cost MB/s */
	double latency; /* us (10e-6 seconds) */

	double total_real_flopss; /* the sustained flops/s after testing, GFLOPs/s */
	double flopss_percore; /* per core performance GFLOPs/s */
#if defined(DEVICE_NVGPU_CUDA_SUPPORT)
	//cublasHandle_t cublas_handle;
	void * cublas_handle;
#endif
	int status;
	volatile rex_wunit_info_t * offload_request; /* this is the notification flag that the helper thread will pick up the offloading request */

	rex_wunit_t * offload_stack[4];
	/* the stack for keeping the nested but unfinished offloading request, we actually only need 2 so far.
	 * However, if we know the current offload (being processed) is one that the device will run to completion, we will not put into the stack
	 * for the purpose of saving the cost of push/pop. Thus the stack only keep those pending offload (e.g. inside a "target data" offload, we have
	 * a "target" offload, the "target data" offload will be pushed to the stack. The purpose of this stack is to help data mapping inheritance, i.e.
	 * reuse the data map created in the upper-level enclosing offloading operations (typically target data).
	 */
	int offload_stack_top;

	rex_dev_stream_t default_stream; /* per dev stream */

	rex_datamap_t ** resident_datamaps; /* a link-list or an array for resident data maps (data maps cross multiple offloading region */

	char padding[CACHE_LINE_SIZE];
};

/**
 * we organize record and timing as a sequence of event, recording could be done by host side or by device-specific approach
 */
typedef enum rex_event_record_method {
	REX_EVENT_DEV_NONE = 0,
	REX_EVENT_DEV_RECORD,
	REX_EVENT_HOST_RECORD,
	REX_EVENT_HOST_DEV_RECORD,
} rex_event_record_method_t;

#define REX_EVENT_MSG_LENGTH 96
#define REX_EVENT_NAME_LENGTH 12

typedef struct rex_event {
	rex_device_t * dev;
	rex_dev_stream_t stream;
	rex_event_record_method_t record_method;
	const char *event_name;
	char event_description[REX_EVENT_MSG_LENGTH];
	int count; /* a counter for accumulating recurring event */
	int recorded; /* everytime stop_record is called, this flag is set, and when a elapsed is calculated, this flag is reset */

#if defined (DEVICE_NVGPU_CUDA_SUPPORT)
	cudaEvent_t start_event_dev;
	cudaEvent_t stop_event_dev;
#endif
#if defined (DEVICE_OPENCL_SUPPORT)
	cl_event cl_start_event_dev;
	cl_event cl_stop_event_dev;
#endif

/** TODO: this could be a union for different devices
	union {
#if defined (DEVICE_NVGPU_CUDA_SUPPORT)
		cudaEvent_t cudaStream;
#endif
#if defined (DEVICE_OPENCL_SUPPORT)
		cl_command_queue clqueue;
#endif
	} start_event_dev, stop_event_dev;
**/

	double start_time_dev;
	double stop_time_dev;
	double start_time_host;
	double stop_time_host;
	double elapsed_dev;
	double elapsed_host;
} rex_event_t;

/* tracing
 * The event tracing are performed by each helper thread that writes traces to a file.
 * The master thread (who starts the offloading) will write a master trace file.
 * Traces are stored in multiple files (1 index + #targets trace files), the trace_name is:
 * <offloading_name>_<uid>_<recur_id>
 *
 * The master index file is named as <trace_name>.index
 * Each trace file is named as <trace_name>_vdevid_pdevid.txt
 *
 * The content of master index:
 * trace_name
 * vdevid pdevid (one per line)
 * ...
 * starter time_stamp
 * end time_stamp
 *
 * Trace file content:
 * event_name start_timestamp stop_timestamp
 * ....
 *
 */
//#define REX_BREAKDOWN_TIMING 1
#if defined (REX_BREAKDOWN_TIMING)
enum event_index {
	total_event_index = 0,       		/* host event */
	total_event_accumulated_index,  	/* host event */
	timing_init_event_index, 			/* host event */
	map_init_event_index,  				/* host event */
	map_dist_alloc_event_index,  		/* host event */
	runtime_dist_modeling_index,    	/* host event for measuring the runtime modeling overhead */

	acc_mapto_event_index, 				/* dev event */
	acc_kernel_exe_event_index,			/* dev event */
	acc_ex_pre_barrier_event_index, 	/* host event */
	acc_ex_event_index,  				/* host event for data exchange such as halo xchange */
	acc_ex_post_barrier_event_index,	/* host event */
	acc_mapfrom_event_index,			/* dev event */

	sync_cleanup_event_index,			/* host event */
	barrier_wait_event_index,			/* host event */
	profiling_barrier_wait_event_index,			/* host event */

	misc_event_index_start,      		/* other events, e.g. mapto/from for each array, start with 9*/
};

extern void rex_wunit_info_sum_profile(rex_wunit_info_t ** infos, int count, double start_time, double compl_time);

#endif

/** ================================================================================================================== *
 * xthread (an execution thread, e.g. pthread) which could be doing real computation work, perform data movement,
 * communicating with remote xthreads, or offloading
 */
typedef enum rex_xthread_type {
	REX_XTH_REX = 1, /* rexized as REX xth when pthread is created */
	REX_XTH_USER, /* rexized from user pthread */
} rex_xthread_type_t;

typedef enum rex_xthread_state {
	/* the following are pure xth state that tools or profiler can use to collect information about the runtime */
	REX_XTH_STATE_IDLE,		/* looping in the runtime to search for work  */
	REX_XTH_STATE_RUNTIME,	/* doing some work at the runtime, not idle */
	REX_XTH_STATE_WORK,		/* doing useful (user) work, now in user code/stack */

	/* the following are co-status of the last user code (task or parallel region) this xth is working on
	 * it is used to pass on information from user to runtime when switching stack
	 * It could be viewed as intermediate state of the xth when switching from one of the above state to another
	 */
} rex_xthread_state_t;

/**
 * The deque/queue system of the HPT and runtime is organized according to the HPT structures. There is outbox (deques) and inbox (sc_queues)
 * for xth and place. The outbox is implemented as deques, from which an xth pushes/pop wunit (mainly tasks) from its tail while other xths pull (steal)
 * wunit from it head. The inbox is implemented as sc_queues, from which multiple xths (others) may push wunit (mainly parallel region) to its tail,
 * and the owner xth only pull from its head.
 */

struct rex_xthread {
	/********* identity and position in the runtime HPT **********/
	unsigned int id;
	int did; /* the mapping device id, i.e. cpu core */

	rex_xthread_type_t type; /* make sure the pthread is initialized to participate REX, a user thread can participate REX only if it is rexized*/
	rex_xthread_state_t state;
	int complete; /* local detected complete, but not yet notify globally (rex_runtime.complete); */
	/* the scheduler context, rex_sched(). For xthread, which is created to run wunit, it uses its own pthread stack when
	 * the pthread for xth is created. For user thread that is migrated to run rex_sched, a new stack is created */

	struct rex_place * pl; /* the directly attached place */
	struct rex_place * binding_hpt; /* the binding place, a hpt subtree that this xth can only steal work from */
	struct rex_xthread * nnext; /* the link of other worker thread in the same place */

	rex_wunit_t * wunit; /* the current wunit the xthread is executing, a user code may move from one stack to another */
	FILE * log; /* the debug message log file */
	unsigned int rseed; /* for random number generator */
}__attribute__ ((__aligned__(CACHE_LINE_SIZE)));

/** =================================================================================================================== *
 * APIs to support data/array mapping and distribution
 */
typedef enum rex_datamap_direction {
	REX_DATA_MAP_TO,
	REX_DATA_MAP_FROM,
	REX_DATA_MAP_TOFROM,
	REX_DATA_MAP_ALLOC,
} rex_datamap_direction_t;

typedef enum rex_datamap_type {
	REX_DATA_MAP_AUTO, /* system choose, it is shared, but system will use either copy or shared depending on whether real sharing  is possible or not */
	REX_DATA_MAP_SHARED,
	REX_DATA_MAP_COPY,
} rex_datamap_type_t;

typedef enum rex_dist_policy {
	REX_DIST_POLICY_FULL,
	REX_DIST_POLICY_BLOCK,
	REX_DIST_POLICY_CYCLIC, /* user defined */
	REX_DIST_POLICY_ALIGN, /* used only between data or between iteration, but not between data and iteration */
	REX_DIST_POLICY_BIND, /* used only between a data and an iteration */
	REX_DIST_POLICY_FIX, /* fixed dist */
	REX_DIST_POLICY_SCHED_STATIC_RATIO, /* dist according to a user-specified ratio */
	REX_DIST_POLICY_SCHED_STATIC_CHUNK, /* dist according to a user-specified ratio */
	REX_DIST_POLICY_SCHED_DYNAMIC, /* schedule the iteration/elements so that the balance is automatically through
							   * scheduling of multiple small chunks of the same size identified using chunk_size field,
							   */
	REX_DIST_POLICY_SCHED_GUIDED, /* schedule the iteration/elements so that the balance is automatically through
							   * scheduling of multiple small chunks of gradulatelly changed sizes starting
							   * from chunk_size field. The algorithm of changing the chunk size is system-specific */
	REX_DIST_POLICY_SCHED_FEEDBACK, /* schedule based on previous performance to dynamically adjust the chunk sizes,
 									 * faster dev gets more and more while smaller gets fewers and fewers */
	REX_DIST_POLICY_MODEL_1_AUTO, /* the balanced loop distribution so computation is distributed using an analytical
                           * model for load balance. The model makes best-efforts and one-time decision
                           * to distribute all iterations, it only consider computation for distribution */
	REX_DIST_POLICY_MODEL_2_AUTO, /* the balanced loop distribution so computation is distributed using an analytical
                           * model for load balance. The model makes best-efforts and one-time decision
                           * to distribute all iterations, it consider both computation and data movement */
	REX_DIST_POLICY_SCHED_PROFILE_AUTO, /* use a small amount of iterations to profile and then do the AUTO based on the profiling info used with iteration dist */
	REX_DIST_POLICY_MODEL_PROFILE_AUTO, /* use model to allocate max number of chunks to each device, profile and then distribute the rest according to the normalized performance */
	REX_DIST_POLICY_TOTAL_NUMS,
} rex_dist_policy_t;

typedef struct rex_dist_policy_argument {
	rex_dist_policy_t type;
	float chunk; /* chunk size in integer or in percentage, if it is negative, it is percentage */
	float cutoff_ratio;
	char *name;
	char *shortname;
} rex_dist_policy_argument_t;
extern rex_dist_policy_argument_t rex_dist_policy_args[];

typedef enum rex_dist_target_type {
	REX_DIST_TARGET_DATA_MAP,
	REX_DIST_TARGET_LOOP_ITERATION,
} rex_dist_target_type_t;
/**
 * Info object for dist (array and iteration)
 */
typedef struct rex_dist_info {
	volatile long start; /* the next start index for the dist, starting from offset */
	char padding[CACHE_LINE_SIZE];
	long offset; /* the offset index for the dim of the original array */
	long end; /* the upper bound of the dist, not inclusive */
	long length;   /* the total # element to be distributed */
	long stride; /* stride between ele, default 1 of course */
	rex_dist_policy_t policy; /* the dist policy */
	float chunk_size; /* initial chunk size for REX_DIST_POLICY_SCHED_DYNAMIC policy. If it is negative, we use it as percentage
                       * the reason to use float is because we may use it as 0.1%, which will be stored in -0.1 in this variable
                       */

	int dim_index; /* the index of top dim to apply dist, for block, duplicate, auto. For ALIGN, this is dim at the alignee*/

	rex_dist_target_type_t alignee_type; /* a data map or a loop iteration */
	union alignee_t { /* The dist this dist_info aligns with, it could be a loop iteration or a data map. It uses dim_index to reference which dim */
		rex_wunit_info_t * loop_iteration;
		rex_datamap_info_t * datamap_info;
	} alignee;

	/* the following are the container so we know which array/loop uses this dist, a backtrack pointer of a dist_info */
	rex_dist_target_type_t target_type;
	int target_dim;
	void * target; /* opaque, see below union */
	/*
	union dist_target_t {
		rex_wunit_info_t * loop_iteration;
		rex_datamap_info_t * datamap_info;
	};
	*/

	/*
	 * A flag used to tell whether re-distribution is needed or not after the initial one if this dist is to be used again
	 * For example, if this dist aligns with another one that keep changing, e.g. SCHEDULE or PROFILE_AUTO policy
	 * we will set this flag so realignment will be performed */
	int redist_needed;
	volatile int dist_race; /* counter to see current position in the race for scheduling */
} rex_dist_info_t;

typedef enum rex_dist_halo_edging_type {
	REX_DIST_HALO_EDGING_NOHALO = 1,
	REX_DIST_HALO_EDGING_REFLECTING,
	REX_DIST_HALO_EDGING_PERIODIC,
} rex_dist_halo_edging_type_t;

/**
 * in each dimension, halo region have left and right halo region and also a flag for cyclic halo or not,
 */
typedef struct rex_datamap_halo_region {
	/* the info */
	int left; /* element size, it is the elements needed from left (not to provide) */
	int right; /* element size */
	rex_dist_halo_edging_type_t edging;
	int topdim; /* which dimension of the device topology this halo region is related to, it is the same as the dim_index of dist object of the same dimension */
} rex_datamap_halo_region_info_t;

/* for each mapped host array, we have one such object */
struct rex_datamap_info {
    rex_wunit_info_t *wunit_info;
    const char * symbol; /* the user symbol */
	char * source_ptr;
	int num_dims;
	long dims[REX_MAX_NUM_DIMENSIONS];

	int sizeof_element;
	rex_datamap_direction_t map_direction; /* the map type, to, from, or tofrom */
	rex_datamap_type_t map_type;
	rex_datamap_t * maps; /* a list of data maps of this array */

	/*
	 * The dist array maintains info on how the array should be distributed among dev topology, not including
	 * the halo region info.
	 */
	rex_dist_info_t dist_info[REX_MAX_NUM_DIMENSIONS];

	 /* the halo region: halo region is considered out-of-bound access of the main array region,
	  * thus the index could be -1, -2, or larger than the dimensions. Our memory allocation and pointer
	  * arithmetic will make sure we do not go out of memory bound
	  */
	int num_halo_dims; /* a simple flag for checking whether this map has halo or not */
	rex_datamap_halo_region_info_t halo_info[REX_MAX_NUM_DIMENSIONS]; /* it is an num_dims array */
	int remap_needed;
};

/** a data map can only be changed by the shepherd thread of the device that map is belong to, but
 * other shepherds can read when the info is ready. The access level is used to control when others can read
 * on what info
 */
typedef enum rex_datamap_access_level {
	REX_DATA_MAP_ACCESS_LEVEL_0, /* garbage value */
	REX_DATA_MAP_ACCESS_LEVEL_INIT, /* basic info, such as dev, info, is there */
	REX_DATA_MAP_ACCESS_LEVEL_DIST, /* map_dim, offset, etc info is there */
	REX_DATA_MAP_ACCESS_LEVEL_HALO, /* if halo, halo neightbors are set up, and halo buffers are allocated */
	REX_DATA_MAP_ACCESS_LEVEL_MALLOC, /* dev mem buffer is allocated */
} rex_datamap_access_level_t;

typedef struct rex_datamap_halo_region_mem {
	/* the mem for halo management */
	/* the in/out pointer is the buffer for the halo regions.
	 * The in ptr is the buffer for halo region that will be copied in,
	 * and the out is for those that will be copied out.
	 * In implementation, we put the in and out buffer into one mem space for each left and right halo region
	 */
	int left_dev_seqid; /* left devseqid, can be used to access left_map and left_dev */
	int right_dev_seqid;

	char * left_in_ptr; /* the halo region this needs */
	long left_in_size; /* for pull update, == right_out_size if push protocol is used */
	char * left_out_ptr; /* the halo region this will privide to the left */
	long left_out_size;

	char * right_in_ptr;
	long right_in_size; /* for pull update, == left_out_size if push protocol is used */
	char * right_out_ptr;
	long right_out_size;

	/* if p2p communication is not available, we will need buffer at host to relay the halo exchange.
	 * Each data map only maintains the relay pointers for halo that they need, i.e. a pull
	 * protocol should be applied for halo exchange.
	 */
	char * left_in_host_relay_ptr;
	volatile int left_in_data_in_relay_pushed;
	volatile int left_in_data_in_relay_pulled;
	/* the push flag is set when the data is pushed by the source to the host relay so the receiver side can pull,
	 * a simple busy-wait is used to wait for the data to arrive
	 */
	char * right_in_host_relay_ptr;
	volatile int right_in_data_in_relay_pushed;
	volatile int right_in_data_in_relay_pulled;
	char padding[CACHE_LINE_SIZE];
} rex_datamap_halo_region_mem_t;

/**
 * dist object, a subregion of the whole region defined in info object
 *
 */
typedef struct rex_dist {
	rex_dist_info_t * info; /* not yet used so far */
	long offset; /* offset from the original array for most alignment type except for align dist, offset is the relative offset from the alignee offset */
	long length;
	float ratio; /* a user specified distribution ratio */

	/*
	 * For the same data/loop, the proxy thread may apply mapping/dist multiple times.
	 * we use either a counter to keep track of how may mapping/dist operations have been applied, thus reuse the object,
	 * but we will loop the information since each mapping/dist overwrites previous info.
	 * Or we create a link-list using the next field to track all the mapping/dist operations.
	 *
	 * When we use counter, total_length accumulates all the lengths each time this dist is used
	 *
	 * The same approaches are used in data maps
	 */
	int counter;
	int redist_needed;
	struct rex_dist * next;
	long total_length;
	long acc_total_length; /* accumulated total length from multiple run */

	char padding[CACHE_LINE_SIZE];
} rex_dist_t;

/* for each device, we maintain a list such objects, each for one mapped array */
struct rex_datamap {
	rex_datamap_access_level_t access_level;
	rex_datamap_info_t * info;
    rex_device_t * dev;
	rex_place_t * place;
	rex_datamap_type_t map_type;

	/*
	 * For the same data/loop, the proxy thread may apply mapping/dist multiple times.
	 * we use either a counter to keep track of how may mapping/dist operations have been applied, thus reuse the object,
	 * but we will loop the information since each mapping/dist overwrites previous info.
	 * Or we create a link-list using the next field to track all the mapping/dist operations.
	 *
	 * total_map_size and total_map_wextra_size store the sizes of all the mapped data regions
	 */
	int dist_counter;
	struct rex_datamap * next;
	long total_map_size;
	long total_map_wextra_size;

	/* the subarray for the mapped region, including the offset and length */
	rex_dist_t map_dist[REX_MAX_NUM_DIMENSIONS];
	/* the sizes in bytes */
	long map_size; // = map_dim[0] * map_dim[1] * map_dim[2] * sizeof_element;
	long map_wextra_size; /* include extras, e.g. halo */

	/* source side */
	char * map_source_ptr; /* the mapped buffer on host. This pointer is either the	offset pointer from the source_ptr */
	char * map_source_wextra_ptr;

	/* dev side */
	char * map_dev_ptr; /* the mapped buffer on device, only for the mapped array region (not including halo region) */
	char * map_dev_wextra_ptr; /* the mapped buffer on device, include extras, such as halo region */

	rex_datamap_halo_region_mem_t halo_mem [REX_MAX_NUM_DIMENSIONS];

	int mem_noncontiguous;
	//rex_dev_stream_t stream; /* the stream operations of this data map are registered with, mostly it will be the stream created for an offloading */

	/* each map has pointers to its mapto and/or mapfrom event for timing individual operations. The two events objects are in off object */
	rex_event_t * mapto_event;
	rex_event_t * mapfrom_event;

	char padding[CACHE_LINE_SIZE];
};

/**
 * the data exchange direction, FROM is for pull, TO is for push
 */
typedef enum rex_datamap_exchange_direction {
	REX_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT,
	REX_DATA_MAP_EXCHANGE_FROM_LEFT_ONLY,
	REX_DATA_MAP_EXCHANGE_FROM_RIGHT_ONLY,
	REX_DATA_MAP_EXCHANGE_TO_LEFT_RIGHT,
	REX_DATA_MAP_EXCHANGE_TO_LEFT_ONLY,
	REX_DATA_MAP_EXCHANGE_TO_RIGHT_ONLY,
} rex_datamap_exchange_direction_t;

/**
 * the data exchange info, used for forwarding a request to shepherd thread to perform
 * parallel data exchange between devices, e.g. halo region exchange
 */
typedef struct rex_datamap_halo_exchange_info {
	rex_datamap_info_t * map_info; /* the map info the exchange needs to perform */
	int x_dim;
	rex_datamap_exchange_direction_t x_direction;
} rex_datamap_halo_exchange_info_t;

/** ====================================================================================================================*
 *  Offloading support
 */
typedef enum rex_offloading_type {
	REX_OFFLOADING_DATA, /* e.g. rex target data, i.e. only offloading data */
	REX_OFFLOADING_DATA_CODE, /* e.g. rex target, i.e. offloading both data and code, and all the data used by the code are specified in this one */
	REX_OFFLOADING_CODE, /* e.g. rex target, i.e. offloading code and partial data only, for other data, inherent data offloaded by the enclosing rex target data */

	/* data exchange offloading, while a regular offloading can carry data exchange that will
	 * be performed after finishing the offloading tasks, this type of offloading is a standalone data exchange offloading */
	REX_OFFLOADING_STANDALONE_DATA_EXCHANGE,
} rex_offloading_type_t;

/**
 * A sequence of calls in one offloading involve.
 * set up stream and event for queuing and sync purpose, we currently only one stream per
 * device at a time, i.e. multiple concurrent streams are not supported.
 * 1. compute data mapping region for each variables, allocate device memory to store mapped data
 * 2. copy data from host to device (could happen while allocating device memory)
 * 3. launch the kernel that will work on the data
 * 4. if any, do data exchange between host and devices, and between devices
 * 5. more kernel launch and data exchange
 * 6. copy data from device to host, deallocate memory that will not be used
 *
 * Between each of the above steps, a barrier may be needed.
 * The thread will can be used to a accelerator
 */
typedef enum rex_offloading_stage {
	REX_OFFLOADING_INIT,      /* initialization of device, e.g. stream, host barrier, etc */
	REX_OFFLOADING_MAPMEM,    /* compute data map and allocate memory/buffer on host and device */
	REX_OFFLOADING_COPYTO,    /* copy data from host to device */
	REX_OFFLOADING_KERNEL,    /* kernel execution */
	REX_OFFLOADING_EXCHANGE,  /* p2p (dev2dev) and h2d (host2dev) data exchange */
	REX_OFFLOADING_COPYFROM,  /* copy data from dev to host */
	REX_OFFLOADING_SYNC, 		/* sync*/
	REX_OFFLOADING_SYNC_CLEANUP, /* sync and clean up */
	REX_OFFLOADING_SECONDARY_OFFLOADING, /* secondary offloading of the original one because of the dist policy */
	REX_OFFLOADING_MDEV_BARRIER, /* mdev barrier wait to sync all participating devices */
	REX_OFFLOADING_COMPLETE,  /* anything else if there is */
	REX_OFFLOADING_NUM_STEPS, /* total number of steps */
} rex_offloading_stage_t;

/** ==================================================================================================================== *
 * Runtime modeling and profiling support
 */
/* a kernel profile keep track of info such as # of iterations, # nest loop, # load per iteration, # store per iteration, # FP per operations
 * data access pattern that has locality/cache access impact, etc
 */
typedef struct rex_kernel_profile_info {
	unsigned long num_load;
	unsigned long num_store;
	unsigned long num_fp_operations;
} rex_kernel_profile_info_t;

/** =====================================================================================================================*
 * Work unit
 */
/**
 * A work unit (wunit) could be a SPMD region (omp parallel), a data parallel region (OMP for), an asynchronous CPU tasks, an
 * asynchronous GPU tasks (?), an asynchronus data movement operation.
 *
 * There could be two (or more) work units, computational wunits, and communicational wunits. computational wunits could include
 * CPU and ACC wunits (or local and remote ones), or in another way, SPMD region, async tasks, etc.
 *
 * NOTE: we may also include sync wunit if this need special runtime attentions
 */
typedef enum {
	REX_WUNIT_PARALLEL = 1,/* SPMD parallel region */
	REX_WUNIT_PARALLEL_FOR,/* data parallel region, i.e. parallel for */
	REX_WUNIT_OFFLOADING_DATA,
	REX_WUNIT_OFFLOADING_DATA_CODE,
	REX_WUNIT_OFFLOADING_CODE,
	REX_WUNIT_ASYNC_TASK, /* async tasks */
	REX_WUNIT_MEMCPY, /* memory operations */
	REX_WUNIT_REMOTE, /* any remote operation, TODO */
} rex_wunit_type_t;

/**
 * a wunit could be suspended, running, completed, and to be continued. The status of a wunit will be checked by the runtime to
 * make scheduling decisions
 */
typedef enum {
	REX_WUNIT_STATUS_CREATED, /* created and before the first run */
	REX_WUNIT_STATUS_RUNNING, /* running */
	REX_WUNIT_STATUS_SUSPENDED_AT_CONTINUATION,
	REX_WUNIT_STATUS_SUSPENDED_AT_JOIN, /* suspended because of hiting a continuation point, such as taskwait */
	REX_WUNIT_STATUS_SUSPENDED_AT_COLLECTIVE, /* suspended because of encountering a collective operations such as barrier or reduction */
	REX_WUNIT_STATUS_WAIT_DEPENDENCY, /* suspended because of unsatisfied dependency, NOTE: not sure we need this or not */
	REX_WUNIT_STATUS_CONTINUE_COLLECTIVE,
	REX_WUNIT_STATUS_COMPLETE,
	REX_WUNIT_STATUS_UNKNOWN,
} rex_wunit_status_t;

/**
 * rex work unit is either a SPMD parallel region which then contains multiple individual wunits,
 * or an async task, or an offloading work. A wunit contains both computation and data maps.
 *
 * If needed, a wunit_info object describes one or multiple wunits. Particularly for parallel wunit, it is used to describe all the wunits by the threads.
 *
 * A SPMD parallel region (e.g. the omp parallel region). A SPMD region is created on a target place, partitioned
 * and then distributed to the child places. So the original SPMD region (as from program) is the top-level region
 * (whose origin==NULL), and the subregion that are distributed to the child places all point to the original region
 * through origin filed. The same for start_id, which is 0 for the origin SPMD region. The number of partitions to the
 * child places are all dependes on pl->num_w_threads.
 *
 */
struct rex_wunit_info {
	rex_wunit_type_t type; /* what to offload: data, code, or both*/
	/************** per-offloading var, shared by all target devices ******/
	rex_grid_topology_t * top; /* target devices */
	const char * name; /* kernel name */

	int recurring;
	volatile int count; /* if an offload is within a loop (while/for, etc and with goto) of a function, it is a recurring */
	double start_time;
	double compl_time;
	double elapsed;

	int nums_run; /* for experimental purpose so we can run the same offloading mulitple time for performance collection */

	int num_mapped_vars;
	rex_datamap_info_t * datamap_info; /* an entry for each mapped variable */
	rex_wunit_t * wunits; /* a list of dev-specific offloading objects, num of this is num of targets from top */
	rex_kernel_profile_info_t full_kernel_profile;
	rex_kernel_profile_info_t per_iteration_profile;

	/* max three level of loop nest */
	rex_dist_info_t loop_dist_info[REX_MAX_NUM_DIMENSIONS];
	int loop_depth; /* max 3 so far */
	int loop_redist_needed; /* a flag for telling whether redistribution is needed after the initial one */

	/* the universal kernel launcher and args, the helper thread will use this one if no dev-specific one is provided */
	/* the helper thread will first check these two field, if they are not null, it will use them, otherwise, it will use the dev-specific one */
	void * args;
	void (*kernel_launcher)(rex_wunit_t *, void *); /* the same kernel to be called by each of the target device, if kernel == NULL, we are just offloading data */

	/* there are two approaches we handle halo exchange, 1) halo exchange as part of a previous regular kernel offloading and halo exchange
	 * will be executed right after finishing the kernel. 2) halo exchange as a standalone offloading operations.
	 *
	 * For standalone data exchange offloading, the type is REX_OFFLOADING_STANDALONE_DATA_EXCHANGE, for the first option, runtime will
	 * check whether halo_x_info pointer is NULL or not to see whether there is appended data exchange or not after a regular kernel
	 */
	rex_datamap_halo_exchange_info_t * halo_x_info;
	int num_maps_halo_x;
};

#define OFF_MAP_CACHE_SIZE 64
/**
 * info for per device
 *
 * For each offloading, we have this object to keep track per-device info, e.g. the stream used, mapped region of
 * a variable (array or scalar) and kernel info.
 *
 * The next pointer is used to form the offloading queue (see rex_device struct)
 */
struct rex_wunit {
	/* per-offloading info */
	rex_wunit_info_t * wunit_info;

	/************** per device var ***************/
	rex_dev_stream_t mystream;
	rex_dev_stream_t stream;
	int devseqid; /* device seqid in the top */
	rex_device_t * dev; /* the dev object, as cached info */
	rex_offloading_stage_t stage;

	/* we will use a simple fix-sized array for simplicity and performance (than a linked list) */
	/* an offload can has as many as OFFLOADING_MAP_CACHE_SIZE mapped variable */
	struct {
		rex_datamap_t * map;
		int inherited; /* flag to mark whether this is an inherited map or not */
	} map_cache[OFF_MAP_CACHE_SIZE];
	int num_maps;

	rex_dist_t loop_dist[3];
	rex_kernel_profile_info_t kernel_profile;
	rex_kernel_profile_info_t per_iteration_profile;
	int loop_dist_done; /* a flag */
	int dist_counter; /* this counter should be the same as the counter of the map of this off so we keep track the data and computation */
	long last_total; /* the total amount of iteration assigned to this device last time.
			  * This is a flag used to check whether we need to do dist or not.
			  * If last_total is 0, we do not need to dist to this dev anymore */

	/* for profiling purpose */
	rex_event_t *events;
	int num_events;

	/* the auto model purpose */
	double Ar;
	double Br;

	/* kernel info */
	long X1, Y1, Z1; /* the first level kernel thread configuration, e.g. CUDA blockDim */
	long X2, Y2, Z2; /* the second level kernel thread config, e.g. CUDA gridDim */
	void *args;
	void (*kernel_launcher)(rex_wunit_t *, void *); /* device specific kernel, if any */
	double runtime_profile_elapsed;
	char padding[CACHE_LINE_SIZE];
};


/** =================================================================================================================== *
 * APIs to the OpenMP runtime system (most are simply wrappers of the corresponding __kmpc_/__kmp_ calls
 */
extern int rex_init(int argc, char * argv[]);
extern int rex_fini();
extern int rex_get_global_thread_num();
extern int rex_get_total_num_threads();
extern int rex_get_thread_num_in_team();
extern int rex_get_num_threads_in_team();
extern int rex_in_parallel( );
extern void rex_set_num_threads(int num_threads );

typedef void (*rex_pfunc_t)    (int * global_tid, int * team_tid, ... );
extern void rex_parallel(int num_threads, rex_pfunc_t func, int num_shared, ...);
extern void rex_parallel_1(rex_pfunc_t func, int num_shared, ...);

extern void rex_barrier(int gtid);
extern void rex_barrier_1();
extern int rex_master(int gtid);
extern int rex_master_1();
extern void rex_end_master(int gtid);
extern void rex_end_master_1();
extern int rex_barrier_master(int gtid);
extern int rex_barrier_master_1();
extern void rex_end_barrier_master(int gtid);
extern void rex_end_barrier_master_1();
extern int rex_single(int gtid);
extern int rex_single_1();
extern void rex_end_single(int gtid);
extern void rex_end_single_1();

/** =================================================================================================================== *
 * APIs for grid topology, implementation in rex_grid_topology.cpp
 */
extern rex_grid_topology_t * rex_grid_topology_init_simple(int ndevs, int ndims);
extern rex_grid_topology_t * rex_grid_topology_init(int ndevs, int *devs, int ndims);
/*  factor input n into dims number of numbers (store into factor[]) whose multiplication equals to n */
extern void rex_grid_topology_fini(rex_grid_topology_t * top);
extern void rex_factor(int n, int factor[], int dims);
extern void rex_topology_print(rex_grid_topology_t * top);
extern int rex_grid_topology_get_seqid(rex_grid_topology_t * top, int devid);

/** =================================================================================================================== *
 * APIs for data maps
 */
extern void rex_datamap_free(rex_datamap_t *map, rex_wunit_t *off);



extern void rex_datamap_init_info(const char *symbol, rex_datamap_info_t *info, rex_wunit_info_t *off_info,
								   void *source_ptr, int num_dims, int sizeof_element, rex_datamap_direction_t map_direction,
								   rex_datamap_type_t map_type);
extern void rex_datamap_info_set_dims_1d(rex_datamap_info_t * info, long dim0);
extern void rex_datamap_info_set_dims_2d(rex_datamap_info_t * info, long dim0, long dim1);
extern void rex_datamap_info_set_dims_3d(rex_datamap_info_t * info, long dim0, long dim1, long dim2);

extern void rex_print_map_info(rex_datamap_info_t * info);

extern void rex_datamap_dist_init_info(rex_datamap_info_t *map_info, int dim, rex_dist_policy_t dist_policy,
										long offset, long length, float chunk_size, int topdim);
extern void rex_loop_dist_init_info(rex_wunit_info_t *off_info, int level, rex_dist_policy_t dist_policy,
									long offset,
									long length, float chunk_size, int topdim);
extern void rex_datamap_dist_align_with_datamap_with_halo(rex_datamap_info_t *map_info, int dim, long offset,
															rex_datamap_info_t *alignee, int alignee_dim);
extern void rex_datamap_dist_align_with_datamap(rex_datamap_info_t *map_info, int dim, long offset,
												  rex_datamap_info_t *alignee, int alignee_dim);
extern void rex_datamap_dist_align_with_loop(rex_datamap_info_t *map_info, int dim, long offset,
											  rex_wunit_info_t *alignee, int alignee_level);
extern void rex_loop_dist_align_with_datamap(rex_wunit_info_t *loop_off_info, int level, long offset,
											  rex_datamap_info_t *alignee, int alignee_dim);
extern void rex_loop_dist_align_with_loop(rex_wunit_info_t *loop_off_info, int level, long offset,
										  rex_wunit_info_t *alignee, int alignee_level);
extern void rex_datamap_init_map(rex_datamap_t *map, rex_datamap_info_t *info, rex_device_t *dev);
extern long rex_datamap_dist(rex_datamap_t *map, int seqid);
extern long rex_loop_iteration_dist(rex_wunit_t *off);
extern void rex_datamap_add_halo_region(rex_datamap_info_t *info, int dim, int left, int right,
									rex_dist_halo_edging_type_t edging);
//extern int rex_datamap_has_halo(rex_datamap_info_t * info, int dim);
//extern int rex_datamap_get_halo_left_devseqid(rex_datamap_t * map, int dim);
//extern int rex_datamap_get_halo_right_devseqid(rex_datamap_t * map, int dim);

extern rex_datamap_t *rex_datamap_offcache_iterator(rex_wunit_t *off, int index, int * inherited);
extern void rex_datamap_append_map_to_offcache(rex_wunit_t *off, rex_datamap_t *map, int inherited);
extern int rex_datamap_is_map_inherited(rex_wunit_t *off, rex_datamap_t *map);
extern rex_datamap_t * rex_datamap_get_map_inheritance (rex_device_t * dev, void * host_ptr);
extern rex_datamap_t * rex_datamap_get_map(rex_wunit_t *off, void * host_ptr, int map_index);
extern void rex_print_datamap(rex_datamap_t * map);
extern void rex_datamap_malloc(rex_datamap_t *map, rex_wunit_t *off);
extern void * rex_datamap_marshal(rex_datamap_t *map);

extern void rex_datamap_unmarshal(rex_datamap_t * map);
extern void rex_datamap_free_dev(rex_device_t *dev, void *ptr, int size);
extern void *rex_datamap_malloc_dev(rex_device_t *dev, void *src, long size);
extern void * rex_unified_malloc(long size);
extern void rex_unified_free(void *ptr);
extern void rex_datamap_mapto(rex_datamap_t * map);
extern void rex_datamap_mapto_async(rex_datamap_t * map, rex_dev_stream_t stream);
extern void rex_datamap_mapfrom(rex_datamap_t * map);
extern void rex_datamap_mapfrom_async(rex_datamap_t * map, rex_dev_stream_t stream);
extern void rex_datamap_memcpy_to(void * dst, rex_device_t * dstdev, const void * src, long size);
extern void rex_datamap_memcpy_to_async(void * dst, rex_device_t * dstdev, const void * src, long size, rex_dev_stream_t stream);
extern void rex_datamap_memcpy_from(void * dst, const void * src, rex_device_t * srcdev, long size);
extern void rex_datamap_memcpy_from_async(void * dst, const void * src, rex_device_t * srcdev, long size, rex_dev_stream_t stream);
extern int rex_datamap_enable_memcpy_DeviceToDevice(rex_device_t * dstdev, rex_device_t * srcdev);
extern void rex_datamap_memcpy_DeviceToDevice(void * dst, rex_device_t * dstdev, void * src, rex_device_t * srcdev, int size) ;
extern void rex_datamap_memcpy_DeviceToDeviceAsync(void * dst, rex_device_t * dstdev, void * src, rex_device_t * srcdev, int size, rex_dev_stream_t srcstream);

extern void rex_halo_region_pull(rex_datamap_t * map, int dim, rex_datamap_exchange_direction_t from_left_right);
extern void rex_halo_region_pull_async(rex_datamap_t * map, int dim, int from_left_right);


#if 0
extern void rex_print_homp_usage();
extern void rex_print_dist_policy_options();
extern rex_dist_policy_t rex_read_dist_policy_options(float *chunk_size, float *cutoff_ratio);
/* The use of the two global variables is an easy way to pass env setting to user program */
extern rex_dist_policy_t LOOP_DIST_POLICY;
extern float LOOP_DIST_CHUNK_SIZE;
extern float LOOP_DIST_CUTOFF_RATIO;

extern rex_wunit_info_t * rex_offloading_init_info(const char *name, rex_grid_topology_t *top, int recurring,
														rex_offloading_type_t off_type, int num_maps,
														void (*kernel_launcher)(rex_offloading_t *, void *), void *args,
														int loop_nest_depth);
extern void rex_offloading_append_profile_per_iteration(rex_wunit_info_t *info, long num_fp_operations,
														long num_loads, long num_stores);
extern void rex_offloading_fini_info(rex_wunit_info_t * info);
extern void rex_wunit_info_report_profile(rex_wunit_info_t *info, int count);

extern void rex_offloading_append_data_exchange_info (rex_wunit_info_t * info, rex_datamap_halo_exchange_info_t * halo_x_info, int num_maps_halo_x);
extern rex_wunit_info_t * rex_offloading_standalone_data_exchange_init_info(const char *name, rex_grid_topology_t *top, int recurring,
																				 rex_datamap_halo_exchange_info_t *halo_x_info, int num_maps_halo_x);
extern void rex_offloading_start(rex_wunit_info_t *off_info);



extern int rex_get_max_threads_per_team(rex_device_t * dev);
extern int rex_get_optimal_threads_per_team(rex_device_t * dev);
extern int rex_get_max_teams_per_league(rex_device_t * dev);
extern int rex_get_optimal_teams_per_league(rex_device_t * dev, int threads_per_team, int total);

#if defined (DEVICE_NVGPU_CUDA_SUPPORT)
extern void xrex_beyond_block_reduction_float_stream_callback(cudaStream_t stream,  cudaError_t status, void* userData );
#endif

/**
 * return the mapped range index from the iteration range of the original array
 * e.g. A[128], when being mapped to a device for A[64:64] (from 64 to 128), then, the range 100 to 128 in the original A will be
 * mapped to 36 to 64 in the mapped region of A
 *
 * @param: rex_datamap_t * map: the mapped variable, we should use the original pointer and let the runtime retrieve the map
 * @param: int dim: which dimension to retrieve the range
 * @param: int offset: the offset index from the original array, if offset is -1, use the map_offset_<dim>, which will simply cause
 * 					the function return 0 for obvious reasons
 * @param: int length: the length of the range, if -1, use the mapped dim from the offset
 * @param: int * map_offset: the mapped offset index in the mapped range, if return <0 value, wrong input
 * @param: int * map_length: normally just the length, if lenght == -1, use the map_dim[dim]
 *
 * NOTE: the mapped range must be a subset of the range of the specified map in the specified dim
 *
 */
extern long rex_loop_get_range(rex_offloading_t *off, int loop_level, long *offset, long *length);

/* util */
extern double read_timer_ms();
extern double read_timer();
#endif

#ifdef __cplusplus
 }
#endif

#endif /* __REX_H__ */
