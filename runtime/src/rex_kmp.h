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

typedef void (*rex_function_t)    (void * arg1, ...);
extern void rex_parallel(int num_threads, rex_function_t func, int argc, ...);

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

/* Worksharing API */
/**
 * For STATIC schedule, please check kmp_sched.cpp file to see how __kmpc_for_static_init_4 is called and should be used
 *
 * For DYNAMIC and GUIDED schedule, please check kmp_dispatch.cpp file to see how __kmpc_dist_dispatch_init_4, __kmpc_dispatch_next_4, and
 * __kmpc_dispatch_fini_4 should be used to make a correct call here.
 * Check the https://www.openmprtl.org/sites/default/files/resources/libomp_20160808_manual.pdf page 12 to see how loop is transformed. And you may
 * also need to see the compiler IR dump to see which function is called.
 *
 * Rewrite the runtime/test/rex_kmpapi/rex_for.c example to use our interface rex_parallel and rex_for
 */
typedef enum rex_sched_type {
    REX_SCHED_STATIC,
    REX_SCHED_DYNAMIC,
    REX_SCHED_GUIDED,
} rex_sched_type_t;
#define REX_DEFAULT_CHUNK_SIZE -1
typedef void (*rex_pfunc_3args_t)    (int *global_tid, int * tid, void * arg1, void * arg2, void * arg3);
extern void rex_parallel_3args(int num_threads, rex_pfunc_3args_t func, void * arg1, void * arg2, void * arg3);
typedef void (*rex_for_body_t) (int i, void * arg1, void * arg2, void * arg3);
extern void rex_for_sched(int low, int up, int stride, rex_sched_type_t sched_type, int chunk,
                          rex_for_body_t for_body, void *arg1, void * arg2, void *arg3);
extern void rex_for(int low, int up, int stride, int chunk, rex_for_body_t for_body, void *arg1, void * arg2, void *arg3);
extern void rex_parallel_for_sched(int num_threads, int low, int up, int stride, rex_sched_type_t sched_type, int chunk,
                                   rex_for_body_t for_body, void *arg1, void *arg2, void * arg3);
extern void rex_parallel_for(int num_threads, int low, int up, int stride, int chunk,
                             rex_for_body_t for_body, void *arg1, void *arg2, void * arg3);

/**
 * tasking interface will need some reverse-engineering and studying the runtime/test/rex_kmpapi/kmp_taskloop.c
 * and runtime/src/kmp_tasking.cpp files to figure out how the three __kmpc_
 * functions are used for tasking: __kmp_task_alloc, __kmpc_omp_task and __kmpc_omp_taskwait. We will use the three
 * functions to implement our rex_ related tasking interface.
 *
 * Rewrite the rex_fib.c example using our interface for testing our interface, you need to use rex_parallel and rex_single as well.
 */
typedef struct rex_task rex_task_t;

typedef enum rex_task_deptype {
    REX_TASK_DEPTYPE_IN,
    REX_TASK_DEPTYPE_OUT,
    REX_TASK_DEPTYPE_INOUT,
} rex_task_deptype_t;

#define REX_DEFAULT_NUM_TASK_DEPS 16

/* just create a task, not schedule it */
extern rex_task_t * rex_create_task(int num_deps, rex_function_t task_func, int argc, ...);
extern rex_task_t * rex_create_task_data(int num_deps, rex_function_t task_func, int argc, void * data, int datasize, ...);

extern void rex_task_add_dependency(rex_task_t * t, void * base, int length, rex_task_deptype_t deptype);
extern void rex_sched_task(rex_task_t * t);
extern void rex_taskwait();

/* create and schedule a task */
#define REX_TASK(num_deps, task_func, argc, ...)                                        \
    do {                                                                                \
        rex_task_t * t = rex_create_task(num_deps, task_func, argc, __VA_ARGS__);       \
        rex_sched_task(t);                                                              \
    } while(0)

#define REX_TASK_DATA(num_deps, task_func, argc, data, datasize, ...)                                           \
    do {                                                                                                        \
        rex_task_t * t = rex_create_task_data(num_deps, task_func, argc, data, datasize, __VA_ARGS__);          \
        rex_sched_task(t);                                                                                      \
    } while(0)

/* taskgroup_begin and taskgroup_end has to be used in a pair, within one function (?) */
extern void rex_taskgroup_begin();
extern void rex_taskgroup_end();

extern void rex_taskyield();

/**
 * TBD
 */
extern void rex_taskloop();

/* util */
extern double read_timer_ms();
extern double read_timer();

#ifdef __cplusplus
 }
#endif

#endif /* __REX_KMP_H__ */
