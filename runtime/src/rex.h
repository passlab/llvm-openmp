#ifndef __REX_H__
#define __REX_H__

#define REX_VERSION_MAJOR    @REX_VERSION_MAJOR@
#define REX_VERSION_MINOR    @REX_VERSION_MINOR@

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
 }
#endif

#endif /* __REX_H__ */
