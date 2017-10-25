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

/** similar to __kmpc_fork_call
 * TODO: if __kmpc_fork_call is changed, we may need to change this accordingly
 * @param loc
 * @param gtid
 * @param argc
 * @param microtask
 * @param ap
 */
static void __rex_to_kmp_fork_call(ident_t *loc, int gtid, kmp_int32 argc, kmpc_micro microtask,
#if (KMP_ARCH_X86_64 || KMP_ARCH_ARM || KMP_ARCH_AARCH64) && KMP_OS_LINUX
        va_list   * ap
#else
                                 va_list     ap
#endif
) {
#if (KMP_STATS_ENABLED)
    int inParallel = __kmpc_in_parallel(loc);
  if (inParallel)
  {
      KMP_COUNT_BLOCK(OMP_NESTED_PARALLEL);
  }
  else
  {
      KMP_COUNT_BLOCK(OMP_PARALLEL);
  }
#endif

    // maybe to save thr_state is enough here
    {
#if OMPT_SUPPORT
        ompt_frame_t* ompt_frame;
    if (ompt_enabled) {
       kmp_info_t *master_th = __kmp_threads[ gtid ];
       kmp_team_t *parent_team = master_th->th.th_team;
       ompt_lw_taskteam_t *lwt = parent_team->t.ompt_serialized_team_info;
       if (lwt)
         ompt_frame = &(lwt->ompt_task_info.frame);
       else
       {
         int tid = __kmp_tid_from_gtid( gtid );
         ompt_frame = &(parent_team->t.t_implicit_task_taskdata[tid].
         ompt_task_info.frame);
       }
       ompt_frame->reenter_runtime_frame = __builtin_frame_address(1);
    }
#endif

#if INCLUDE_SSC_MARKS
        SSC_MARK_FORKING();
#endif
        __kmp_fork_call( loc, gtid, fork_context_intel,
                         argc,
#if OMPT_SUPPORT
                VOLATILE_CAST(void *) microtask,      // "unwrapped" task
#endif
                         VOLATILE_CAST(microtask_t) microtask, // "wrapped" task
                         VOLATILE_CAST(launch_t)    __kmp_invoke_task_func,
                         ap
        );
#if INCLUDE_SSC_MARKS
        SSC_MARK_JOINING();
#endif
        __kmp_join_call( loc, gtid
#if OMPT_SUPPORT
                , fork_context_intel
#endif
        );
    }
}

void rex_parallel(int num_threads, rex_pfunc_t func, int num_shared, ...)
{
    int current_thread = __kmpc_global_thread_num(NULL);
    __kmpc_push_num_threads(NULL, current_thread, num_threads );
    va_list ap;
    va_start(ap, num_shared);
    __rex_to_kmp_fork_call(NULL, current_thread, num_shared, func,
#if (KMP_ARCH_X86_64 || KMP_ARCH_ARM || KMP_ARCH_AARCH64) && KMP_OS_LINUX
            &ap
#else
            ap
#endif
    );
    va_end(ap);
}

void rex_parallel_1(rex_pfunc_t func, int num_shared, ...)
{
    int current_thread = __kmpc_global_thread_num(NULL);
    va_list ap;
    va_start(ap, num_shared);
    __rex_to_kmp_fork_call(NULL, current_thread, num_shared, func,
#if (KMP_ARCH_X86_64 || KMP_ARCH_ARM || KMP_ARCH_AARCH64) && KMP_OS_LINUX
            &ap
#else
                           ap
#endif
    );
    va_end(ap);
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


// end of file //

