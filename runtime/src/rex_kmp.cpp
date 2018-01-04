//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//

#include "omp.h"        /* extern "C" declarations of user-visible routines */
#include "kmp.h"
#include "kmp_i18n.h"
#include "kmp_itt.h"
#include "kmp_lock.h"
#include "kmp_error.h"
#include "kmp_stats.h"

#if OMPT_SUPPORT
#include "ompt-internal.h"
#include "ompt-specific.h"
#endif

#include "rex_kmp.h"

/**
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * A temp solution to use the OpenMP exported interface. We should put this in a file named, e.g.
 * rex_kmp_internal.h and include. We will do this later.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern void __kmpc_for_static_init_4(ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
                              kmp_int32 *plastiter, kmp_int32 *plower,
                              kmp_int32 *pupper, kmp_int32 *pstride,
                              kmp_int32 incr, kmp_int32 chunk);

#ifdef __cplusplus
}
#endif
/**
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

/*!
 * @ingroup REX_RUNTIME
 *
 * @param argc   in   number of argv (currently not used)
 * @param argv   in   a list of argv (currently not used)
 * @return The global thread id of the calling thread in the runtime
 *
 * Initialize the runtime library. This call is optional; if it is not made then
 * it will be implicitly called by attempts to use other library functions.
 *
 */
int rex_init(int argc, char * argv[])
{
    KC_TRACE( 10, ("rex_init: called\n" ) );
    // By default __kmp_ignore_mppbeg() returns TRUE.
    if (__kmp_ignore_mppbeg() == FALSE) {
        __kmp_internal_begin();
        KC_TRACE( 10, ("rex_init: end\n" ) );
    }
    return __kmp_get_gtid();
}

/*!
 * @ingroup REX_RUNTIME
 * @return The global thread id of the calling thread in the runtime
 *
 * Shutdown the runtime library. This is also optional, and even if called will not
 * do anything unless the `KMP_IGNORE_MPPEND` environment variable is set to zero.
  */
int rex_fini()
{
    // By default, __kmp_ignore_mppend() returns TRUE which makes __kmpc_end() call no-op.
    // However, this can be overridden with KMP_IGNORE_MPPEND environment variable.
    // If KMP_IGNORE_MPPEND is 0, __kmp_ignore_mppend() returns FALSE and __kmpc_end()
    // will unregister this root (it can cause library shut down).
    if (__kmp_ignore_mppend() == FALSE) {
        int gtid = __kmp_get_gtid();
        KC_TRACE( 10, ("__kmpc_end: called\n" ) );
        KA_TRACE( 30, ("__kmpc_end\n" ));

        __kmp_internal_end_thread( -1 );
        return gtid;
    } return -1;
}

int rex_get_global_thread_num()
{
    kmp_int32 gtid = __kmp_entry_gtid();
    KC_TRACE( 10, ("rex_get_global_thread_num: T#%d\n", gtid ) );
    return gtid;
}

int rex_get_total_num_threads()
{
    KC_TRACE(10,("rex_get_total_num_threads: num_threads = %d\n", __kmp_all_nth));
    return TCR_4(__kmp_all_nth);
}

int rex_get_thread_num_in_team()
{
    KC_TRACE( 10, ("rex_get_thread_num_in_team: called\n" ) );
    return __kmp_tid_from_gtid( __kmp_entry_gtid() );
}

int rex_get_num_threads_in_team()
{
    KC_TRACE( 10, ("rex_get_num_threads_in_team: called\n" ) );

    return __kmp_entry_thread() -> th.th_team -> t.t_nproc;
}

int rex_in_parallel( )
{
    return __kmp_entry_thread() -> th.th_root -> r.r_active;
}

void rex_set_num_threads(int num_threads )
{
    int global_tid = __kmp_get_gtid();
    KA_TRACE( 20, ("rex_set_num_threads: enter T#%d num_threads=%d\n",
      global_tid, num_threads ) );

    __kmp_push_num_threads( NULL, global_tid, num_threads );
}

/**
 * rex_parallel is implemented as a wrapper of __kmpc_fork_call, but we restricted that the
 * microtask (parallel_func) must be defined with 5 parameters as follows:

 void parallel_func(int *global_tid, int* tid, void *arg1, void *arg2, void * arg3)
 *
 * This restriction for # and type of the parameters comes from the kmpc_micro definition
 * typedef void (*kmpc_micro)(kmp_int32 *global_tid, kmp_int32 *bound_tid, ...);
 *
 * So other than the first two parameters of the kmpc_micro, we further restricted the varadic (...) to be 3 in
 * order to simplify the implementation and in most cases, 3 users arguments are sufficient. If a user want to pass
 * more than 3 arguments to a function, she has to box it, as ususally done for passing multiple arguments to
 * function such as pthread_create
 *
 * @param num_threads
 * @param func: must be of void parallel_func(int *global_tid, int* tid, void *arg1, void *arg2, void * arg3)
 * @param arg1
 * @param arg2
 * @param arg3
 */
void rex_parallel(int num_threads, rex_pfunc_t func, void * arg1, void * arg2, void * arg3) {
    int current_thread = __kmpc_global_thread_num(NULL);
    __kmpc_push_num_threads(NULL, current_thread, num_threads );
    __kmpc_fork_call(NULL, 3, (kmpc_micro)func, arg1, arg2, arg3);
}

void rex_barrier(int gtid)
{
    __kmpc_barrier(NULL, gtid);
}

void rex_barrier_1()
{
    __kmpc_barrier(NULL, __kmp_get_gtid());
}

int rex_master(int gtid)
{
    return __kmpc_master(NULL, gtid);
}

int rex_master_1()
{
    return __kmpc_master(NULL, __kmp_get_gtid());
}

void rex_end_master(int gtid)
{
    __kmpc_end_master(NULL, gtid);
}

void rex_end_master_1()
{
    __kmpc_end_master(NULL, __kmp_get_gtid());
}

int rex_barrier_master(int gtid)
{
    return __kmpc_barrier_master(NULL, gtid);
}

int rex_barrier_master_1()
{
    return __kmpc_barrier_master(NULL, __kmp_get_gtid());
}

void rex_end_barrier_master(int gtid)
{
    __kmpc_end_barrier_master(NULL, gtid);
}

void rex_end_barrier_master_1()
{
    __kmpc_end_barrier_master(NULL, __kmp_get_gtid());
}

int rex_single(int gtid)
{
    return __kmpc_single(NULL, gtid);
}

int rex_single_1()
{
    return __kmpc_single(NULL, __kmp_get_gtid());
}

void rex_end_single(int gtid)
{
    __kmpc_end_single(NULL, gtid);
}

void rex_end_single_1()
{
    __kmpc_end_single(NULL, __kmp_get_gtid());
}


/**
 * This function minics "omp for" construct within a parallel region. Thus this function can only be called inside a
 * parallel_func(int *global_tid, int* tid, void *arg1, void *arg2, void * arg3) which is passed to rex_parallel
 *
 * So by using rex_parallel and rex_for_sched with the provided parallel_func and for_body function, we can create
 * OpenMP styple "omp parallel" and "omp for" nested worksharing
 *
 * typedef void (*for_body) (int i, void * arg1, void * arg2, void * arg3) is the function that enclose the body of
 * a parallel loop, and we restrict exactly three arguments for the function.
 *
 * @param low
 * @param up
 * @param stride
 * @param sched_type
 * @param chunk
 * @param for_body
 * @param arg1
 * @param arg2
 * @param arg3
 */
void rex_for_sched(int low, int up, int stride, rex_sched_type_t sched_type, int chunk,
                   rex_for_body_t for_body, void *arg1, void * arg2, void *arg3) {
    int i;
    int lower, upper, liter = 0, incr = 1;
    enum sched_type sched;
    if (chunk == REX_DEFAULT_CHUNK_SIZE) {
        sched = kmp_sch_static;
    } else {
        if (sched_type == REX_SCHED_STATIC) {
            sched = kmp_sch_static;
        } else if (sched_type == REX_SCHED_DYNAMIC) {
            sched = kmp_sch_dynamic_chunked;
        } else if (sched_type == REX_SCHED_GUIDED) {
            sched = kmp_sch_guided_chunked;
        } else {
            sched = kmp_sch_static;
        }
    }
    //fprintf(stderr, "Thread %d/%d inside rex_for, calling __kmpc_dispatch_init_4() with lastiter = %d, low = %d, up = %d, stride = %d, incr = %d, chunk = %d\n", omp_get_thread_num(), omp_get_num_threads(), liter, low, up, stride, incr, chunk);
    __kmpc_dispatch_init_4(NULL, __kmp_get_global_thread_id(), sched, low, up, stride, chunk ); /// strid issue here
    //fprintf(stderr, "Thread %d/%d inside rex_for, going inside for loop with parameters low = %d, up = %d, stride = %d, lastiter = %d\n", omp_get_thread_num(), omp_get_num_threads(), low, up, stride, liter);
    while ( __kmpc_dispatch_next_4( NULL, __kmp_get_global_thread_id(), & liter, & low, & up, &stride) ) {
        //fprintf(stderr, "--Thread %d/%d inside rex_for, going inside for loop with parameters low = %d, up = %d, stride = %d, lastiter = %d\n", omp_get_thread_num(), omp_get_num_threads(), low, up, stride, liter);
        for( i = low; i <= up; i=i+stride )
            for_body(i, arg1, arg2, arg3);
    }
    /** kmp_sched.cpp is where the runtime code for static scheduling. However, as shown above
     * kmp_dispatch.cpp also allows for static scheduling, thus we combine them into one above
    if (sched_type == REX_SCHED_STATIC) {
        int lastiter;

        __kmpc_for_static_init_4(NULL, __kmp_get_global_thread_id(), kmp_sch_static,
        &lastiter, &low, &upper, &stride, 0, 0);

    }
    */
}

/**
 * simple rex_for that minics "omp for" with default clause and sched type
 * @param low
 * @param up
 * @param stride
 * @param chunk
 * @param for_body
 * @param arg1
 * @param arg2
 * @param arg3
 */
void rex_for(int low, int up, int stride, int chunk, rex_for_body_t for_body, void *arg1, void * arg2, void *arg3) {
    rex_for_sched(low, up, stride, REX_SCHED_DYNAMIC, chunk, for_body, arg1, arg2, arg3);
}

/**
 * For "omp parallel for", the implementation does not use rex_parallel and rex_for_sched directly. Instead, we created
 * rex_parallel_func_for_parallel_for to do that.
 *
 * @param global_id
 * @param tid
 * @param low
 * @param up
 * @param stride
 * @param sched_type
 * @param chunk
 * @param for_body
 * @param arg1
 * @param arg2
 * @param arg3
 */
static void rex_parallel_func_for_parallel_for(int *global_id, int * tid, int low, int up, int stride,
                                      rex_sched_type_t sched_type, int chunk,
                                      rex_for_body_t for_body, void *arg1, void * arg2, void * arg3) {
    rex_for_sched(low, up, stride, sched_type, chunk, for_body, arg1, arg2, arg3);
}

/**
 * This function minics "omp parallel for" with provided sched_type and chunk
 * @param num_threads
 * @param low
 * @param up
 * @param stride
 * @param sched_type
 * @param chunk
 * @param for_body
 * @param arg1
 * @param arg2
 * @param arg3
 */
void rex_parallel_for_sched(int num_threads, int low, int up, int stride, rex_sched_type_t sched_type, int chunk,
                            rex_for_body_t for_body, void *arg1, void *arg2, void * arg3) {
    int current_thread = __kmpc_global_thread_num(NULL);
    __kmpc_push_num_threads(NULL, current_thread, num_threads );
    __kmpc_fork_call(NULL, 9, (kmpc_micro)rex_parallel_func_for_parallel_for, low, up, stride,
                     sched_type, chunk, for_body, arg1, arg2, arg3);
}

/**
 * The simple form of rex_parallel_for_sched
 */
void rex_parallel_for(int num_threads, int low, int up, int stride, int chunk,
                      rex_for_body_t for_body, void *arg1, void *arg2, void * arg3) {
    rex_parallel_for_sched(num_threads, low, up, stride, REX_SCHED_DYNAMIC, chunk, for_body, arg1, arg2, arg3);
}

/**
 * The memory for a task is as follows, see the rex_create_task func and the __kmpc_omp_task_alloc function.
 * ____________________________________________________
 * |      kmp_taskdata_t                              |
 * |  |---kmp_task_t                                  |
 * |  |   task_func                                   |
 * |  |   num_deps_user_requested(int)                |
 * |  |---num_deps(int)                               |
 * |      kmp_depend_info_t[num_deps_user_requested]  |
 * |      private data area                           |
 * ----------------------------------------------------
 *
 * in rex_create_task, __kmpc_omp_task_alloc is called and returned kmp_task_t. Since we use a rex_taskinfo_t data structure
 * so the returned pointer is also the pointer to the rex_taskinfo_t object.
 *
 * So a pointer to rex_task_t, kmp_task_t and rex_taskinfo_t are the same.
 */
typedef struct rex_taskinfo {
    kmp_task_t task;
    union {
        rex_task_func_t func;
        rex_task_func_args_t func_args;
    } task_func;
    int max_num_deps; /* the number of dependencies set when calling rex_create_task, i.e. max # of deps */
    int num_deps; /* the number of dependencies later on when rex_task_add_dependency is called for the actual added */
} rex_taskinfo_t;

#define REX_GET_TASK_DEPEND_INFO_PTR(t) (kmp_depend_info_t*)((char*)t+sizeof(rex_taskinfo_t))

/**
 * a task entry function so we can wrap task func so kmp accepts it
 * @param task_fun
 * @param arg1
 * @return
 *
 * The trick of passing data pointer (void *) and function pointer is by using pointer to a pointer

typedef void (*FPtr)(void); // Hide the ugliness
FPtr f = someFunc;          // Function pointer to convert
void* ptr = *(void**)(&f);  // Data pointer
FPtr f2 = *(FPtr*)(&ptr);   // Function pointer restored
 */

static int rex_task_entry_func(int gtid, void * arg) {
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*) arg;
    kmp_depend_info_t * depinfo = REX_GET_TASK_DEPEND_INFO_PTR(arg);
    void * task_private = (void*)&depinfo[rex_tinfo->max_num_deps];

//    fprintf(stderr, "task func when executing tasks: %X\n", task_func);
    rex_tinfo->task_func.func(task_private, rex_tinfo->task.shareds);

    //((void (*)(void *))(*(task->routine)))(task->shareds);
}

static int rex_task_entry_func_args(int gtid, void * arg) {
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*) arg;
    kmp_depend_info_t * depinfo = REX_GET_TASK_DEPEND_INFO_PTR(arg);
    void ** task_private = (void**)&depinfo[rex_tinfo->max_num_deps];

 //   fprintf(stderr, "task func when executing tasks: %X, n: %d, &x: %p\n", rex_tinfo->task_func.func_args, task_private[0], task_private[1]);
    rex_tinfo->task_func.func_args(task_private[0], task_private[1], task_private[2]);
}

/**
 * create a task object, but not schedule it.
 *
 * Due to the way kmp stores a task, this function calls __kmpc_omp_task_alloc for allocating all the memory for the
 * task, see above the note about the memory for a task.
 *
 * @param task_fun: the task function that take a pointer to the private data and a pointer to the shared data
 * @param size_of_private: the size of private data
 * @param priv: the pointer of the private data. private data will be copied to the staging memory area of the task
 * @param shared: the pointer of the shared data, only the pointer
 * @return
 */
rex_task_t * rex_create_task(rex_task_func_t task_func, int size_of_private, void * priv, void * shared, int num_deps) {

    /* kmpc_omp_task_alloc allocates kmp_taskdata_t and the rest */
    kmp_task_t * task = __kmpc_omp_task_alloc(NULL,__kmp_get_global_thread_id(), 1,
                                sizeof(rex_taskinfo_t) + sizeof(kmp_depend_info_t) * num_deps + size_of_private,
                                              sizeof(void*), &rex_task_entry_func);
    task->shareds = shared;
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*)task;
    rex_tinfo->task_func.func = task_func;
    rex_tinfo->max_num_deps = num_deps;
    rex_tinfo->num_deps = 0;
    kmp_depend_info_t * depinfo = REX_GET_TASK_DEPEND_INFO_PTR(task);
    void * task_private = (void*)&depinfo[num_deps];

    /* copy the private data */
    memcpy(task_private, priv, size_of_private);
    return (rex_task_t *) task;
}

/**
 * create a task with three arguments. The three arguments are copied to the private data area of the task
 * @param task_fun
 * @param arg1
 * @param arg2
 * @param arg3
 * @param num_deps
 * @return
 */
rex_task_t * rex_create_task_args(rex_task_func_args_t task_func, void * arg1, void * arg2, void * arg3, int num_deps) {
    /* kmpc_omp_task_alloc allocates kmp_taskdata_t and the rest */
    kmp_task_t * task = __kmpc_omp_task_alloc(NULL,__kmp_get_global_thread_id(), 1,
                                              sizeof(rex_taskinfo_t) + sizeof(kmp_depend_info_t) * num_deps + sizeof(void*)*3,
                                              0, &rex_task_entry_func_args);
    task->shareds = NULL;
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*)task;
    rex_tinfo->task_func.func_args = task_func;
    rex_tinfo->max_num_deps = num_deps;
    rex_tinfo->num_deps = 0;
    kmp_depend_info_t * depinfo = REX_GET_TASK_DEPEND_INFO_PTR(task);
    void ** task_private = (void**)&depinfo[num_deps];
    /* copy the private data */
    task_private[0] = arg1;
    task_private[1] = arg2;
    task_private[2] = arg3;
//    fprintf(stderr, "Creating tasks: %X, n: %d, &x: %p\n", task_func, arg1, arg2);

    return (rex_task_t *) task;
}

/**
 * to add a dependency to a task before sched_task. This call can only be called in between rex_create_task and
 * rex_sched_task
 * @param t
 * @param base
 * @param length
 * @param deptype
 *
 * This is definitely not thread safe, no need to be
 */
void rex_task_add_dependency(rex_task_t * t, void * base, int length, rex_task_deptype_t deptype) {
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*)t;
    int num_deps = rex_tinfo->num_deps;
    if (num_deps == rex_tinfo->max_num_deps) {
        return; /* TODO, return some warning that they want to add more dependencies than they originalled asked for */
    }
    kmp_depend_info_t * depend_info = &(REX_GET_TASK_DEPEND_INFO_PTR(t))[num_deps];
    depend_info->base_addr = (kmp_intptr_t)base;
    depend_info->len = length;
    if (deptype == REX_TASK_DEPTYPE_IN) {
        depend_info->flags.in = 1;
        depend_info->flags.out = 0;
    }
    if (deptype == REX_TASK_DEPTYPE_OUT) {
        depend_info->flags.in = 1; /* out = inout as implied by the spec and expected by the runtime */
        depend_info->flags.out = 1;
    }
    if (deptype == REX_TASK_DEPTYPE_INOUT) {
        depend_info->flags.in = 1;
        depend_info->flags.out = 1;
    }
    rex_tinfo->num_deps++;
}

/**
 * schedule a task for execution
 * @param t
 * @return
 */
void rex_sched_task(rex_task_t * t) {
    rex_taskinfo_t * rex_tinfo = (rex_taskinfo_t*)t;
    kmp_depend_info_t * depinfo = REX_GET_TASK_DEPEND_INFO_PTR(t);
    int gtid = __kmp_get_global_thread_id();
    if (rex_tinfo->num_deps > 0)
        __kmpc_omp_task_with_deps(NULL, gtid, (kmp_task_t*)t, rex_tinfo->num_deps, depinfo, 0, NULL);
    else
        __kmpc_omp_task(NULL, gtid, (kmp_task_t*)t);

    /* it is ok to schedule a task with no dependency using __kmpc_omp_task_with_deps with 0 dependency
     * So the following call is actually enough for the above if-else code seg
    __kmpc_omp_task_with_deps(NULL, gtid, (kmp_task_t*)t, rex_tinfo->num_deps, depinfo, 0, NULL);
     */
}

/**
 * create a task and give it to runtime for execution.
 * @param task_fun
 * @param size_of_private
 * @param priv
 * @param shared
 */
void rex_task(rex_task_func_t task_func, int size_of_private, void * priv, void * shared) {
    rex_task_t * t = rex_create_task(task_func, size_of_private, priv, shared, 0);
    rex_sched_task(t);
}

/**
 * create a task with three arguments and give it to runtime for execution. The three arguments are copied to the
 * private data area of the task first
 * @param task_fun
 * @param arg1
 * @param arg2
 * @param arg3
 */
void rex_task_args(rex_task_func_args_t task_func, void * arg1, void * arg2, void * arg3) {
    rex_task_t * t = rex_create_task_args(task_func, arg1, arg2, arg3, 0);
    rex_sched_task(t);
}

void rex_taskwait() {
    __kmpc_omp_taskwait(NULL, __kmp_get_global_thread_id());
}

void rex_taskgroup_begin() {
    __kmpc_taskgroup(NULL, __kmp_get_global_thread_id());
}

void rex_taskgroup_end() {
    __kmpc_taskgroup(NULL, __kmp_get_global_thread_id());
}

void rex_taskyield() {
    __kmpc_omp_taskyield(NULL, __kmp_get_global_thread_id(), 0);
}

// end of file //

