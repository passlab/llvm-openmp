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

#include "rex.h"


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

// end of file //

