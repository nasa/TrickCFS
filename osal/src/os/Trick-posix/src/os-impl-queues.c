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
 * \file     os-impl-queues.c
 * \ingroup  posix
 * \author   joseph.p.hickey@nasa.gov
 *
 */

/****************************************************************************************
                                    INCLUDE FILES
 ***************************************************************************************/

#include "bsp-impl.h"
#include "cfe_sb_module_all.h"
#include "os-posix.h"

#include "os-impl-queues.h"
#include "os-shared-idmap.h"
#include "os-shared-queue.h"

#include "sim_services/Executive/include/exec_proto.h"

#include "TrickCFSQueue_C_proto.h"
#include "TrickCFS_C_proto.h"

void OS_Posix_CompAbsDelayTime(uint32 msecs, struct timespec * tm);

/* Tables where the OS object information is stored */
// OS_impl_queue_internal_record_t     OS_impl_queue_table         [OS_MAX_QUEUES];

/****************************************************************************************
                                MESSAGE QUEUE API
 ***************************************************************************************/

/*----------------------------------------------------------------
 *
 * Function: OS_QueueCreate_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_QueueCreate_Impl(const OS_object_token_t * token, uint32 flags)
{
    int return_code;
    size_t queue_id;
    mqd_t queueDesc;
    struct mq_attr queueAttr;
    char name[OS_MAX_API_NAME * 2];
    OS_impl_queue_internal_record_t * impl;
    OS_queue_internal_record_t * queue;

    impl = OS_OBJECT_TABLE_GET(OS_impl_queue_table, *token);
    queue = OS_OBJECT_TABLE_GET(OS_queue_table, *token);
    queue_id = OS_ObjectIndexFromToken(token);

    /* set queue attributes */
    memset(&queueAttr, 0, sizeof(queueAttr));
    queueAttr.mq_maxmsg = queue->max_depth;
    queueAttr.mq_msgsize = queue->max_size;

    /*
     * The "TruncateQueueDepth" indicates a soft limit to the size of a queue.
     * If nonzero, anything larger than this will be silently truncated
     * (Supports running applications as non-root)
     */
    if(POSIX_GlobalVars.TruncateQueueDepth > 0 && POSIX_GlobalVars.TruncateQueueDepth < queueAttr.mq_maxmsg)
    {
        queueAttr.mq_maxmsg = POSIX_GlobalVars.TruncateQueueDepth;
    }

    /*
    ** Construct the queue name:
    ** The name will consist of "/<process_id>.queue_name"
    */
    snprintf(name, sizeof(name), "/%d.%s", (int)getpid(), queue->queue_name);

    /*
     ** create message queue
     */
    queueDesc = TrickCFSQueue_open(name, queue_id, queueAttr.mq_maxmsg, queueAttr.mq_msgsize);
    if(queueDesc == (mqd_t)(-1))
    {
        OS_DEBUG("UNEXPECTED OS_QueueCreate Error from TRICKCFS.");
        return_code = OS_ERROR;
    }
    else
    {
        impl->id = queueDesc;
        return_code = OS_SUCCESS;
    }
    uint32 taskId;
    OS_ConvertToArrayIndex(OS_TaskGetId(), &taskId);
    SCH_TRICK_initialize_pipe(queue_id, taskId);

    return return_code;
} /* end OS_QueueCreate_Impl */

/*----------------------------------------------------------------
 *
 * Function: OS_QueueDelete_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_QueueDelete_Impl(const OS_object_token_t * token)
{
    int32 return_code;
    size_t queue_id = OS_ObjectIndexFromToken(token);

    /* Try to delete and unlink the queue */
    if(TrickCFSQueue_close(OS_impl_queue_table[queue_id].id) != 0)
    {
        OS_DEBUG("UNEXPECTED OS_QueueDelete Error from TRICKCFS.");
        return_code = OS_ERROR;
    }
    else
    {
        return_code = OS_SUCCESS;
    }

    return return_code;
} /* end OS_QueueDelete_Impl */

/*----------------------------------------------------------------
 *
 * Function: OS_QueueGet_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_QueueGet_Impl(const OS_object_token_t * token, void * data, size_t size, size_t * size_copied, int32 timeout)
{
    int32 return_code;
    ssize_t sizeCopied;
    struct timespec ts;
    uint32 taskId;

    OS_ConvertToArrayIndex(OS_TaskGetId(), &taskId);

    size_t queue_id = OS_ObjectIndexFromToken(token);

    /*
     ** Read the message queue for data
     */
    sizeCopied = -1;
    if(timeout == OS_PEND)
    {
        SCH_TRICK_mark_pipe_as_complete(queue_id, taskId);

        sizeCopied = TrickCFSQueue_receive(queue_id, data, size);

        // if (sizeCopied == -1)
        //{
        //     *size_copied = 0;
        //     return(OS_ERROR);
        // }
        // else
        //{
        //    *size_copied = sizeCopied;
        //    CFE_SB_MsgId_t mid = CFE_SB_GetMsgId((CFE_SB_MsgPtr_t)(((CFE_SB_BufferD_t **)data)[0]->Buffer));
        //    SCH_TRICK_acknowledge_pipe_trigger(taskId, mid);
        // }
    }
    else if(timeout == OS_CHECK)
    {
        SCH_TRICK_mark_pipe_as_complete(queue_id, taskId);

        memset(&ts, 0, sizeof(ts));

        sizeCopied = TrickCFSQueue_timedreceive(queue_id, data, size, &ts);
        if(sizeCopied < 0)
        {
            if(sizeCopied == OS_QUEUE_TIMEOUT)
            {
                sizeCopied = OS_QUEUE_EMPTY;
            }
        }
    }
    else
    {
        if(exec_get_mode() != Initialization)
        {
            uint32 schRate = SCH_TRICK_minor_frame_rate_ms_tics();
            do
            {
                SCH_TRICK_mark_pipe_as_complete(queue_id, taskId);

                OS_Posix_CompAbsDelayTime(schRate, &ts);

                sizeCopied = TrickCFSQueue_timedreceive(queue_id, data, size, &ts);

                if(sizeCopied < 0)
                {
                    if(sizeCopied == OS_QUEUE_TIMEOUT)
                    {
                        timeout -= schRate;
                        if(timeout > 0)
                        {
                            OS_TaskDelay(0);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }

            } while(timeout > 0);
        }
        else
        {
            SCH_TRICK_mark_pipe_as_complete(queue_id, taskId);

            OS_Posix_CompAbsDelayTime(timeout, &ts);

            sizeCopied = TrickCFSQueue_timedreceive(queue_id, data, size, &ts);
        }
    } /* END timeout */

    /* Figure out the return code */
    if(sizeCopied < 0)
    {
        *size_copied = 0;
        return_code = sizeCopied;
    }
    else
    {
        *size_copied = sizeCopied;
        size_t beginAddr = (size_t)&CFE_SB_Global.Mem;
        size_t endAddr = beginAddr + sizeof(CFE_SB_Global.Mem);
        size_t dataAddr = (size_t)(*(void **)data);
        if(dataAddr >= beginAddr && dataAddr < endAddr)
        {
            CFE_SB_BufferD_t ** BufDscPtrPtr = (CFE_SB_BufferD_t **)data;
            CFE_SB_BufferD_t * BufDscPtr = *BufDscPtrPtr;
            SCH_TRICK_acknowledge_pipe_trigger(taskId, CFE_SB_MsgIdToValue(BufDscPtr[0].MsgId));
        }
        return_code = OS_SUCCESS;
    }

    return return_code;
} /* end OS_QueueGet_Impl */

/*----------------------------------------------------------------
 *
 * Function: OS_QueuePut_Impl
 *
 *  Purpose: Implemented per internal OSAL API
 *           See prototype for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 OS_QueuePut_Impl(const OS_object_token_t * token, const void * data, size_t size, uint32 flags)
{
    struct timespec ts;
    size_t queue_id = OS_ObjectIndexFromToken(token);

    /*
     * NOTE - using a zero timeout here for the same reason that QueueGet does ---
     * checking the attributes and doing the actual send is non-atomic, and if
     * two threads call QueuePut() at the same time on a nearly-full queue,
     * one could block.
     */
    memset(&ts, 0, sizeof(ts));

    /* send message */
    return TrickCFSQueue_send(queue_id, data, size);
} /* end OS_QueuePut_Impl */
