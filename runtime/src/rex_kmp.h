#ifndef __REX_KMP_H__
#define __REX_KMP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

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

typedef void (*rex_pfunc_t)    (int * global_tid, int * num_args, ... );
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

/* for worksharing */
typedef enum rex_sched_type {
    REX_SCHED_STATIC,
    REX_SCHED_DYNAMIC,
    REX_SCHED_GUIDED,
} rex_sched_type_t;

#define REX_DEFAULT_CHUNK_SIZE -1

typedef void* (*for_body_1) (int i, void * arg1);
typedef void* (*for_body_2) (int i, void * arg1, void * arg2);
typedef void* (*for_body_3) (int i, void * arg1, void * arg2, void * arg3);

/* Worksharing API */
/**
 * called within a parallel function
 * @param low
 * @param up
 * @param stride
 * @param sched_type: make sure you find the mapping of our definition of sched_type to kmp's enum definition of sched_type
 *                    in kmp.h:313
 * @param chunk
 * @param for_body_1
 * @param args
 *
 * For STATIC schedule, please check kmp_sched.cpp file to see how __kmpc_for_static_init_4 is called and should be used
 *
 * For DYNAMIC and GUIDED schedule, please check kmp_dispatch.cpp file to see how __kmpc_dist_dispatch_init_4, __kmpc_dispatch_next_4, and
 * __kmpc_dispatch_fini_4 should be used to make a correct call here.
 * Check the https://www.openmprtl.org/sites/default/files/resources/libomp_20160808_manual.pdf page 12 to see how loop is transformed. And you may
 * also need to see the compiler IR dump to see which function is called.
 *
 * Rewrite the runtime/test/rex_kmpapi/rex_for.c example to use our interface rex_parallel and rex_for
 */
void rex_for_sched(int low, int up, int stride, rex_sched_type_t sched_type, int chunk, void (*for_body_1) (int, void *), void *args);

/**
 * A simpler implementation that always use dynamic schedule policy
 * @param low
 * @param up
 * @param stride
 * @param sched_type
 * @param chunk
 * @param for_body_1
 * @param args
 */
void rex_for(int low, int up, int stride, int chunk, void (*for_body_1) (int, void *), void *args);


/**
 * rex_parallel_for is the combination of rex_parallel and rex_for
 */
void rex_parallel_for();

/**
 * tasking interface will need some reverse-engineering and studying the runtime/test/rex_kmpapi/kmp_taskloop.c
 * and runtime/src/kmp_tasking.cpp files to figure out how the three __kmpc_
 * functions are used for tasking: __kmp_task_alloc, __kmpc_omp_task and __kmpc_omp_taskwait. We will use the three
 * functions to implement our rex_ related tasking interface.
 *
 * Rewrite the rex_fib.c example using our interface for testing our interface, you need to use rex_parallel and rex_single as well.
 */

typedef void rex_task_t; /* even rex_task_t is exported as void type, internally it is kmp_task_t type so we can work around the inclusion of the kmp.h file */
typedef void (*rex_task_func) (void * priv, void * shared);

extern rex_task_t * rex_create_task(rex_task_func task_fun, int size_of_private, void * priv, void * shared);
void * rex_sched_task(rex_task_t * t);
void * rex_taskwait();

/* util */
extern double read_timer_ms();
extern double read_timer();

#ifdef __cplusplus
 }
#endif

#endif /* __REX_KMP_H__ */
