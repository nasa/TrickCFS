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
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/******************************************************************************
S
** File:  cfe_psp_exception.c
**
**      POSIX ( Mac OS X, Linux, Cygwin ) version
**
** Purpose:
**   cFE PSP Exception handling functions
**
** History:
**   2007/05/29  A. Cudmore      | POSIX Version
**
******************************************************************************/

/*
**  Include Files
*/
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/*
** cFE includes
*/
#include "cfe_psp.h"
#include "cfe_psp_config.h"
#include "cfe_psp_exceptionstorage_api.h"
#include "cfe_psp_exceptionstorage_types.h"
#include "common_types.h"
#include "osapi.h"

#include <execinfo.h>
#include <signal.h>

/*
 * A set of asynchronous signals which will be masked during other signal processing
 */
sigset_t CFE_PSP_AsyncMask;

///***************************************************************************
// **                        FUNCTIONS DEFINITIONS
// ***************************************************************************/

/*
** Name: CFE_PSP_ExceptionSigHandler
**
** Installed as a signal handler to log exception events.
**
*/
void CFE_PSP_ExceptionSigHandler(int signo, siginfo_t * si, void * ctxt)
{
    CFE_PSP_Exception_LogData_t * Buffer;
    int NumAddrs;

    /*
     * Note that the time between CFE_PSP_Exception_GetNextContextBuffer()
     * and CFE_PSP_Exception_WriteComplete() is sensitive in that it is
     * accessing a global.
     *
     * Cannot use a conventional lock because this is a signal handler, the
     * solution would need to involve a signal-safe spinlock and/or C11
     * atomic ops.
     *
     * This means if another exception occurs on another task during this
     * time window, it may use the same buffer.
     *
     * However, exceptions should be rare enough events that this is highly
     * unlikely to occur, so leaving this unhandled for now.
     */
    Buffer = CFE_PSP_Exception_GetNextContextBuffer();
    if(Buffer != NULL)
    {
        /*
         * read the clock as a timestamp - note "clock_gettime" is signal safe per POSIX,
         *
         * _not_ going through OSAL to read this as it may do something signal-unsafe...
         * (current implementation would be safe, but it is not guaranteed to always be).
         */
        clock_gettime(CLOCK_MONOTONIC, &Buffer->context_info.event_time);
        /*   Start TrickCFS edit
        memcpy(&Buffer->context_info.si, si, sizeof(Buffer->context_info.si));
          -- end TrickCFS edit*/
        NumAddrs = backtrace(Buffer->context_info.bt_addrs, CFE_PSP_MAX_EXCEPTION_BACKTRACE_SIZE);
        Buffer->context_size = offsetof(CFE_PSP_Exception_ContextDataEntry_t, bt_addrs[NumAddrs]);
        /* pthread_self() is signal-safe per POSIX.1-2013 */
        Buffer->sys_task_id = pthread_self();
        CFE_PSP_Exception_WriteComplete();
    }

    /*
     * notify the main (idle) thread that an interesting event occurred.
     * Note on this platform this cannot _directly_ invoke CFE from a signal handler.
     */
    /*   Start TrickCFS edit
    pthread_kill(CFE_PSP_IdleTaskState.ThreadID, CFE_PSP_EXCEPTION_EVENT_SIGNAL);
      -- end TrickCFS edit*/
}

/*
**   Name: CFE_PSP_AttachExceptions
**
**   This is called from the CFE Main task, before any other threads
**   are started.  Use this opportunity to install the handler for
**   CTRL+C events, which will now be treated as an exception.
**
**   Not only does this clean up the code by NOT requiring a specific
**   handler for CTRL+C, it also provides a way to exercise and test
**   the exception handling in general, which tends to be infrequently
**   invoked because otherwise it only happens with off nominal behavior.
**
**   This has yet another benefit that SIGINT events will make their
**   way into the exception and reset log, so it is visible why the
**   CFE shut down.
**
**   In TrickCFS, we are okay noting the async signals and populating those but
**   we do not want to attach any handlers to signals. Trick will already take
**   care of those.
*/

void CFE_PSP_AttachExceptions(void)
{
    /*   Start TrickCFS edit
    void *Addr[1];
      -- end TrickCFS edit*/

    /*
     * preemptively call "backtrace" -
     * The manpage notes that backtrace is implemented in libgcc
     * which may be dynamically linked with lazy binding. So
     * by calling it once we ensure that it is loaded and therefore
     * it is safe to use in a signal handler.
     */
    /*   Start TrickCFS edit
    backtrace(Addr, 1);
      -- end TrickCFS edit*/

    OS_printf("CFE_PSP: CFE_PSP_AttachExceptions Called\n");

    /*
     * Block most other signals during handler execution.
     * Exceptions are for synchronous errors SIGFPE/SIGSEGV/SIGILL/SIGBUS
     */
    sigfillset(&CFE_PSP_AsyncMask);
    sigdelset(&CFE_PSP_AsyncMask, SIGILL);
    sigdelset(&CFE_PSP_AsyncMask, SIGFPE);
    sigdelset(&CFE_PSP_AsyncMask, SIGBUS);
    sigdelset(&CFE_PSP_AsyncMask, SIGSEGV);

    /*
     * Install sigint_handler as the signal handler for SIGINT.
     *
     * In the event that the user presses CTRL+C at the console
     * this will be recorded as an exception and use the general
     * purpose exception processing logic to shut down CFE.
     *
     * Also include SIGTERM so it will invoke a graceful shutdown
     */
    /*   Start TrickCFS edit
    CFE_PSP_AttachSigHandler(SIGINT);
    CFE_PSP_AttachSigHandler(SIGTERM);
      -- end TrickCFS edit*/

    /*
     * Clear any pending exceptions.
     *
     * This is just in case this is a PROCESSOR reset and there
     * was something still in the queue from the last lifetime.
     *
     * It should have been logged already, but if not, then
     * don't action on it now.
     */
    CFE_PSP_Exception_Reset();
}
