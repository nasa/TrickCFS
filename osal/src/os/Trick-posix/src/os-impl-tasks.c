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

/*
 *
 *    Copyright (c) 2020, United States government as represented by the
 *    administrator of the National Aeronautics Space Administration.
 *    All rights reserved. This software was created at NASA Goddard
 *    Space Flight Center pursuant to government contracts.
 *
 *    This is governed by the NASA Open Source Agreement and may be used,
 *    distributed and modified only according to the terms of that agreement.
 *
 */

/**
 * \file     os-impl-tasks.c
 * \ingroup  posix
 * \author   joseph.p.hickey@nasa.gov
 *
 */

/****************************************************************************************
                                    INCLUDE FILES
 ***************************************************************************************/

#include "bsp-impl.h"
#include "os-posix.h"
#include <sched.h>
#include <sys/resource.h>

#include "os-impl-tasks.h"

#include "os-shared-idmap.h"
#include "os-shared-task.h"

#include "TrickCFS_C_proto.h"

/*
 * Extra Stack Space for overhead -
 *
 * glibc pthreads implicitly puts its TCB/TLS structures at the base of the stack.
 * The actual size of these structures is highly system and configuration dependent.
 * Experimentation/measurement on an x64-64 system with glibc shows this to consume
 * between 9-10kB of stack space off the top.
 *
 * There does not seem to be a reliable/standardized way of determining how large this
 * is going to be, but PTHREAD_STACK_MIN (if defined) should include adequate space for it.
 * If this is not defined, then just assume 1 page.
 *
 * Importantly - when the user passes a stack size to OS_TaskCreate() - the expectation is
 * that there will be at least this amount of real _usable_ space for the task.  If 10kB
 * is used before the entry point is even called, this needs to be accounted for, or else
 * the stack might end up being too small.
 */
#ifdef PTHREAD_STACK_MIN
#define OS_IMPL_STACK_EXTRA PTHREAD_STACK_MIN
#else
#define OS_IMPL_STACK_EXTRA POSIX_GlobalVars.PageSize
#endif

/*----------------------------------------------------------------------------
 * Name: OS_PriorityRemap
 *
 * Purpose: Remaps the OSAL priority into one that is viable for this OS
 *
 * Note: This implementation assumes that InputPri has already been verified
 * to be within the range of [0,OS_MAX_TASK_PRIORITY]
 *
----------------------------------------------------------------------------*/
static int OS_PriorityRemap(uint32 InputPri)
{
    int OutputPri;

    if(InputPri == 0)
    {
        /* use the "MAX" local priority only for OSAL tasks with priority=0 */
        OutputPri = POSIX_GlobalVars.PriLimits.PriorityMax;
    }
    else if(InputPri >= OS_MAX_TASK_PRIORITY)
    {
        /* use the "MIN" local priority only for OSAL tasks with priority=255 */
        OutputPri = POSIX_GlobalVars.PriLimits.PriorityMin;
    }
    else
    {
        /*
         * Spread the remainder of OSAL priorities over the remainder of local priorities
         *
         * Note OSAL priorities use the VxWorks style with zero being the
         * highest and OS_MAX_TASK_PRIORITY being the lowest, this inverts it
         */
        OutputPri = (OS_MAX_TASK_PRIORITY - 1) - (int)InputPri;

        OutputPri *= (POSIX_GlobalVars.PriLimits.PriorityMax - POSIX_GlobalVars.PriLimits.PriorityMin) - 2;
        OutputPri += OS_MAX_TASK_PRIORITY / 2;
        OutputPri /= (OS_MAX_TASK_PRIORITY - 2);
        OutputPri += POSIX_GlobalVars.PriLimits.PriorityMin + 1;
    }

    return OutputPri;
} /* end OS_PriorityRemap */

/*---------------------------------------------------------------------------------------
   Name: OS_PthreadEntry

   Purpose: A Simple pthread-compatible entry point that calls the real task function

   returns: NULL

    NOTES: This wrapper function is only used locally by OS_TaskCreate below

---------------------------------------------------------------------------------------*/
static void * OS_PthreadTaskEntry(void * arg)
{
    OS_VoidPtrValueWrapper_t local_arg;

    local_arg.opaque_arg = arg;
    OS_TaskEntryPoint(local_arg.id); /* Never returns */

    return NULL;
}

#ifndef TRICKCFS_CFS_COMPAT
/*----------------------------------------------------------------
 *
 *  Purpose: Local helper routine, not part of OSAL API.
 *
 *-----------------------------------------------------------------*/
int32 OS_Posix_InternalTaskCreate_Impl(pthread_t * pthr,
                                       osal_priority_t priority,
                                       osal_stackptr_t stackptr,
                                       size_t stacksz,
                                       PthreadFuncPtr_t entry,
                                       void * entry_arg)
{
    int return_code = 0;
    pthread_attr_t custom_attr;
    struct sched_param priority_holder;

    /*
     ** Initialize the pthread_attr structure.
     ** The structure is used to set the stack and priority
     */
    memset(&custom_attr, 0, sizeof(custom_attr));
    return_code = pthread_attr_init(&custom_attr);
    if(return_code != 0)
    {
        OS_DEBUG("pthread_attr_init error in OS_TaskCreate: %s\n", strerror(return_code));
        return OS_ERROR;
    }

    /*
    ** Set the Stack Pointer and/or Size
    */
    if(stackptr != OSAL_TASK_STACK_ALLOCATE)
    {
        return_code = pthread_attr_setstack(&custom_attr, stackptr, stacksz);
    }
    else
    {
        /*
         * Adjust the stack size parameter, add budget for TCB/TLS overhead.
         * Note that this budget can only be added when allocating the stack here,
         * if the caller passed in a stack, they take responsibility for adding this.
         */
        stacksz += OS_IMPL_STACK_EXTRA;

        stacksz += POSIX_GlobalVars.PageSize - 1;
        stacksz -= stacksz % POSIX_GlobalVars.PageSize;

        struct rlimit rl;

        int result = getrlimit(RLIMIT_STACK, &rl);
        if(result == 0)
        {
            if(stacksz > rl.rlim_cur)
            {
                return_code = pthread_attr_setstacksize(&custom_attr, stacksz);
            }
        }
    }

    if(return_code != 0)
    {
        OS_DEBUG("Error configuring stack in OS_TaskCreate: %s\n", strerror(return_code));
        return OS_ERROR;
    }

    /*
    ** Set the thread to be joinable by default
    */
    return_code = pthread_attr_setdetachstate(&custom_attr, PTHREAD_CREATE_JOINABLE);
    if(return_code != 0)
    {
        OS_DEBUG("pthread_attr_setdetachstate error in OS_TaskCreate: %s\n", strerror(return_code));
        return OS_ERROR;
    }

    /*
    ** Test to see if the original main task scheduling priority worked.
    ** If so, then also set the attributes for this task.  Otherwise attributes
    ** are left at default.
    */
    if(POSIX_GlobalVars.EnableTaskPriorities)
    {
        /*
        ** Set the scheduling inherit attribute to EXPLICIT
        */
        return_code = pthread_attr_setinheritsched(&custom_attr, PTHREAD_EXPLICIT_SCHED);
        if(return_code != 0)
        {
            OS_DEBUG("pthread_attr_setinheritsched error in OS_TaskCreate, errno = %s\n", strerror(return_code));
            return OS_ERROR;
        }

        /*
        ** Set the scheduling policy
        ** The best policy is determined during initialization
        */
        return_code = pthread_attr_setschedpolicy(&custom_attr, POSIX_GlobalVars.SelectedRtScheduler);
        if(return_code != 0)
        {
            OS_DEBUG("pthread_attr_setschedpolity error in OS_TaskCreate: %s\n", strerror(return_code));
            return OS_ERROR;
        }

        /*
        ** Set priority
        */
        return_code = pthread_attr_getschedparam(&custom_attr, &priority_holder);
        if(return_code != 0)
        {
            OS_DEBUG("pthread_attr_getschedparam error in OS_TaskCreate: %s\n", strerror(return_code));
            return OS_ERROR;
        }

        priority_holder.sched_priority = OS_PriorityRemap(priority);
        return_code = pthread_attr_setschedparam(&custom_attr, &priority_holder);
        if(return_code != 0)
        {
            OS_DEBUG("pthread_attr_setschedparam error in OS_TaskCreate: %s\n", strerror(return_code));
            return OS_ERROR;
        }

    } /* End if user is root */

    SCH_TRICK_set_affinity_attribute(&custom_attr);

    /*
     ** Create thread
     */
    return_code = pthread_create(pthr, &custom_attr, entry, entry_arg);
    if(return_code != 0)
    {
        OS_DEBUG("pthread_create error in OS_TaskCreate: %s\n", strerror(return_code));
        return OS_ERROR;
    }

    /*
     ** Free the resources that are no longer needed
     ** Since the task is now running - pthread_create() was successful -
     ** Do not treat anything bad that happens after this point as fatal.
     ** The task is running, after all - better to leave well enough alone.
     */
    return_code = pthread_attr_destroy(&custom_attr);
    if(return_code != 0)
    {
        OS_DEBUG("pthread_attr_destroy error in OS_TaskCreate: %s\n", strerror(return_code));
    }

    return OS_SUCCESS;
}

/*----------------------------------------------------------------
 *
 * Function: OS_TaskCreate_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_TaskCreate_Impl(const OS_object_token_t * token, uint32 flags)
{
    OS_VoidPtrValueWrapper_t arg;
    int32 return_code;
    OS_impl_task_internal_record_t * impl;
    OS_task_internal_record_t * task;
    static const int MAX_THREAD_NAME_SIZE = 16;
    char linux_thread_name[MAX_THREAD_NAME_SIZE];

    memset(&arg, 0, sizeof(arg));

    /* cppcheck-suppress unreadVariable // intentional use of other union member */
    arg.id = OS_ObjectIdFromToken(token);

    task = OS_OBJECT_TABLE_GET(OS_task_table, *token);
    impl = OS_OBJECT_TABLE_GET(OS_impl_task_table, *token);

    return_code = OS_Posix_InternalTaskCreate_Impl(&impl->id,
                                                   task->priority,
                                                   task->stack_pointer,
                                                   task->stack_size,
                                                   OS_PthreadTaskEntry,
                                                   arg.opaque_arg);

    if(return_code == 0)
    {
        strncpy(linux_thread_name, task->task_name, MAX_THREAD_NAME_SIZE);
        linux_thread_name[MAX_THREAD_NAME_SIZE - 1] = '\0';
        return_code = pthread_setname_np(impl->id, linux_thread_name);
        if(return_code != 0)
        {
            OS_DEBUG("pthread_setname_np error in OS_TaskCreate: %s\n", strerror(return_code));
        }
    }

    return return_code;
} /* end OS_TaskCreate_Impl */

#else

#define COMPAT_FILE(compat, tag, ext) str_COMPAT_FILE(compat - tag.ext)
#define str_COMPAT_FILE(s) #s

#include COMPAT_FILE(os - impl - tasks, TRICKCFS_CFS_COMPAT, c)
#endif

/*----------------------------------------------------------------
 *
 * Function: OS_TaskDelay_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_TaskDelay_Impl(uint32 millisecond)
{
    uint32 taskId;
    OS_ConvertToArrayIndex(OS_TaskGetId(), &taskId);

    SCH_TRICK_schedule_delay(taskId, millisecond);

    return OS_SUCCESS;
} /* end OS_TaskDelay_Impl */
