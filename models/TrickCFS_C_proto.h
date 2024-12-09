//=============================================================================
// Notices:
//
// Copyright © 2024 United States Government as represented by the Administrator
// of the National Aeronautics and Space Administration.  All Rights Reserved.
//
//
// Disclaimers:
//
// No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
// ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
// TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
// FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE ERROR
// FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE
// SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
// ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
// RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
// RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
// DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
// IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
//
// Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST THE
// UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY
// PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS IN ANY
// LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH USE,
// INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM, RECIPIENT'S
// USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD HARMLESS THE
// UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY
// PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.  RECIPIENT'S SOLE REMEDY FOR
// ANY SUCH MATTER SHALL BE THE IMMEDIATE, UNILATERAL TERMINATION OF THIS
// AGREEMENT.
//
//=============================================================================

/************************TRICK HEADER*************************
 PURPOSE:
 (Trick CFS C-interface API for the SCH_TRICK CFS App.)
 LIBRARY DEPENDENCIES:
 (
 (TrickCFS_C_Interface.o)
 )
 *************************************************************/

#ifndef _TRICKCFS_C_PROTO_HH_
#define _TRICKCFS_C_PROTO_HH_

#include "pthread.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    double SCH_TRICK_minor_frame_rate(void);
    size_t SCH_TRICK_minor_frame_rate_ms_tics(void);
    void SCH_TRICK_schedule_delay(size_t taskId, size_t milliseconds);
    void SCH_TRICK_main_complete(void);
    void SCH_TRICK_wait_for_trigger(void);
    void SCH_TRICK_initialize_pipe(size_t pipeId, size_t taskId);
    void SCH_TRICK_mark_pipe_as_tiggered(size_t pipeId, size_t mid);
    void SCH_TRICK_acknowledge_pipe_trigger(size_t taskId, size_t mid);
    void SCH_TRICK_mark_pipe_as_complete(size_t pipeId, size_t taskId);
    void SCH_TRICK_set_affinity_attribute(pthread_attr_t * attrPtrIn);
    bool SCH_TRICK_is_perfect_enabled(void);
    int SCH_TRICK_get_output_fd(void);

    // Entry point for SCH_TRICK "app"
    void SCH_TRICK_AppMain(void);

#ifdef __cplusplus
}
#endif

#endif
