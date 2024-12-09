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
 * BaseTrickCFSScheduler.cpp
 *
 *  Created on: May 22, 2024
 *      Author: tbrain
 */

#include "BaseTrickCFSScheduler.hh"
#include "TrickCFSInterface.hh"

#include "sim_services/Executive/include/exec_proto.h"

extern "C"
{
#include "cfe.h"
#include "cfe_sb_priv.h"
#include "cfe_sbr.h"
}

extern TrickCFSInterface * the_trickToCFSInterface;
BaseTrickCFSScheduler * the_trickCfsScheduler = 0x0;

extern "C"
{
    void SCH_TRICK_AppMain(void)
    {
        the_trickCfsScheduler->mainLoop();
    }
}

BaseTrickCFSScheduler::BaseTrickCFSScheduler()
    : appId(CFE_ES_APPID_UNDEFINED),
      isInitialized(false),
      schedTypeName("")
{
    if(the_trickCfsScheduler == 0x0)
    {
        the_trickCfsScheduler = this;
    }
    else
    {
        message_publish(MSG_ERROR, "Instance of TrickCFSScheduler already exists.");
    }
}

BaseTrickCFSScheduler::BaseTrickCFSScheduler(const std::string & typeNameIn)
    : appId(CFE_ES_APPID_UNDEFINED),
      isInitialized(false),
      schedTypeName(typeNameIn)
{
    if(the_trickCfsScheduler == 0x0)
    {
        the_trickCfsScheduler = this;
    }
    else
    {
        message_publish(MSG_ERROR, "Instance of TrickCFSScheduler already exists.");
    }
}

BaseTrickCFSScheduler::~BaseTrickCFSScheduler()
{
    // TODO Auto-generated destructor stub
}

void BaseTrickCFSScheduler::mainLoop()
{
    message_publish(MSG_INFO, "Initializing TrickCFSScheduler.\n");
    if(init())
    {
        message_publish(MSG_ERROR, "Unable to initialize TrickCFSScheduler.\n");
        exec_terminate_with_return(-1, __PRETTY_FUNCTION__, __LINE__, "Unable to initialize TrickCFSScheduler.");
        CFE_ES_ExitApp(0xFF);
    }
    else
    {
        message_publish(MSG_INFO, "TrickCFSScheduler initialization complete.\n");
    }

    message_publish(MSG_INFO, "Starting TrickCFSScheduler main loop\n");
    size_t Status = loop();
    if(Status != 0)
    {
        message_publish(MSG_ERROR, "TrickCFSScheduler terminating, err = 0x%08X\n", Status);
        message_publish(MSG_ERROR, "CFE_ES_ExitApp: Application TrickCFSScheduler Had a Runtime Error.\n");
        exec_terminate_with_return(-1, __PRETTY_FUNCTION__, __LINE__, "Unable to initialize TrickCFSScheduler.");

        OS_TaskExit();
    }
    else
    {
        cleanup();
    }

    /*
     ** Let cFE kill the task (and any child tasks)
     */
    CFE_ES_ExitApp(Status);
}

size_t BaseTrickCFSScheduler::init()
{
    CFE_Status_t Status;
    Status = CFE_ES_GetAppID(&appId);
    TRICKCFS_CHECK_STATUS(Status, MSG_ERROR);
    Status = initPipes();
    TRICKCFS_CHECK_STATUS(Status, MSG_ERROR);
    Status = initTables();
    TRICKCFS_CHECK_STATUS(Status, MSG_ERROR);
    std::string declStr = schedTypeName + " * the_trickCfsScheduler";
    the_trickToCFSInterface->addInstance("the_trickCfsScheduler", declStr);
    isInitialized = true;
    SCH_TRICK_main_complete();
    return Status;
}

size_t BaseTrickCFSScheduler::loop()
{
    uint32 Status = CFE_ES_RunStatus_APP_RUN;
    while(CFE_ES_RunLoop(&Status))
    {
        SCH_TRICK_wait_for_trigger();

        Status = processScheduleTable();
        if(!Status)
        {
            Status = CFE_ES_RunStatus_APP_RUN;
        }

        SCH_TRICK_main_complete();
    }
    return Status;
}

size_t BaseTrickCFSScheduler::cleanup()
{
    return 0;
}

size_t BaseTrickCFSScheduler::initPipes()
{
    return 0;
}

void BaseTrickCFSScheduler::sendScheduledMessage(CFE_SB_MsgId_t MsgId,
                                                 CFE_MSG_Message_t * message,
                                                 bool allowSBUpdateHeader)
{
    /* take semaphore to prevent a task switch during processing */
    CFE_SB_LockSharedData(__func__, __LINE__);

    CFE_SBR_RouteId_t RouteId = CFE_SBR_GetRouteId(MsgId);
    CFE_SB_DestinationD_t * DestPtr = 0x0;

    /* Send the packet to all destinations  */
    for(DestPtr = CFE_SBR_GetDestListHeadPtr(RouteId); DestPtr != NULL;
        DestPtr = (CFE_SB_DestinationD_t *)DestPtr->Next)
    {
        if(DestPtr->Active == CFE_SB_INACTIVE) /* destination is active */
        {
            continue;
        } /*end if */

        CFE_SB_PipeD_t * PipeDscPtr = CFE_SB_LocatePipeDescByID(DestPtr->PipeId);

        if((PipeDscPtr->Opts & CFE_SB_PIPEOPTS_IGNOREMINE) != 0 && CFE_RESOURCEID_TEST_EQUAL(PipeDscPtr->AppId, appId))
        {
            continue;
        } /* end if */

        osal_index_t pipeId;
        OS_ConvertToArrayIndex(PipeDscPtr->SysQueueId, &pipeId);

        SCH_TRICK_mark_pipe_as_tiggered(pipeId, CFE_SB_MsgIdToValue(MsgId));

    } /* end loop over destinations */

    CFE_SB_UnlockSharedData(__func__, __LINE__);

    CFE_SB_TransmitMsg(message, allowSBUpdateHeader);
}
