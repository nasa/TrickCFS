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
 * TrickCFSScheduler.cpp
 *
 *  Created on: Mar 8, 2021
 *      Author: tbrain
 */

#include "TrickCFSScheduler.hh"

#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Message/include/message_proto.h"
extern "C"
{
#include "GenericSchTable.h"
#include "cfe.h"
#include "cfe_sb_priv.h"
#include "cfe_sbr.h"
#ifdef sch_tt_EXPORTS
#include "sch_tt_compat.h"
#include "sch_tt_tbldefs.h"
#else
#include "sch_tbldefs.h"
#endif
}

#include "TrickCFS_C_proto.h"

CompileTimeAssert(sizeof(Generic_ScheduleEntry_t) == sizeof(SCH_ScheduleEntry_t), TrickCFSSchTblWrongSize);

CompileTimeAssert(sizeof(Generic_MessageEntry_t) == sizeof(SCH_MessageEntry_t), TrickCFSMsgTblWrongSize);

/*
 ** SDT Table Validation Error Codes
 */
#define SCH_SDT_GARBAGE_ENTRY (-1)
#define SCH_SDT_NO_FREQUENCY (-2)
#define SCH_SDT_BAD_REMAINDER (-3)
#define SCH_SDT_BAD_ACTIVITY (-4)
#define SCH_SDT_BAD_MSG_INDEX (-5)
#define SCH_SDT_BAD_ENABLE_STATE (-6)

/*
 ** MDT Table Validation Error Codes
 */
#define SCH_MDT_GARBAGE_ENTRY (-1)
#define SCH_MDT_INVALID_LENGTH (-2)
#define SCH_MDT_BAD_MSG_ID (-3)

extern "C"
{
    void SCH_TRICK_AppMain(void)
    {
        the_trickCfsScheduler->mainLoop();
    }
}

#define CHECK_STATUS(status, severity)                                                                                 \
    if(status != 0)                                                                                                    \
    {                                                                                                                  \
        message_publish(severity, "Error code %d detected in %s:%d\n", status, __PRETTY_FUNCTION__, __LINE__ - 1);     \
        return status;                                                                                                 \
    }

TrickCFSScheduler::TrickCFSScheduler()
    :
#ifdef SCH_SCH_TABLE_NAME
      schTableName(SCH_SCH_TABLE_NAME),
#elif defined SCH_SCHEDULE_TABLE_NAME
      schTableName(SCH_SCHEDULE_TABLE_NAME),
#elif defined SCH_TT_SCH_TABLE_NAME
      schTableName(SCH_TT_SCH_TABLE_NAME),
#endif
#ifdef SCH_MSG_TABLE_NAME
      msgTableName(SCH_MSG_TABLE_NAME),
#elif defined SCH_MESSAGE_TABLE_NAME
      msgTableName(SCH_MESSAGE_TABLE_NAME),
#elif defined SCH_TT_MSG_TABLE_NAME
      msgTableName(SCH_TT_MSG_TABLE_NAME),
#endif
#ifdef SCH_SCHEDULE_FILENAME
      schTablePath(SCH_SCHEDULE_FILENAME),
#elif defined SCH_TT_SCH_TABLE_FILENAME
      schTablePath(SCH_TT_SCH_TABLE_FILENAME),
#endif
#ifdef SCH_MESSAGE_FILENAME
      msgTablePath(SCH_MESSAGE_FILENAME),
#elif defined SCH_TT_MSG_TABLE_FILENAME
      msgTablePath(SCH_TT_MSG_TABLE_FILENAME),
#endif
      scheduleTableHandle(CFE_TBL_BAD_TABLE_HANDLE),
      messageTableHandle(CFE_TBL_BAD_TABLE_HANDLE),
      nextSlotNumber(0),
      scheduleTable(0x0),
      messageTable(0x0),
      simTimeSecs(-1),
      simTimeMicros(-1)
{
    appId = CFE_ES_APPID_UNDEFINED;
    if(the_trickCfsScheduler == 0x0)
    {
        the_trickCfsScheduler = this;
    }
    else
    {
        message_publish(MSG_ERROR, "Instance of TrickCFSScheduler already exists.");
    }
}

TrickCFSScheduler::~TrickCFSScheduler()
{
    // TODO Auto-generated destructor stub
}

void TrickCFSScheduler::mainLoop()
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

bool TrickCFSScheduler::init()
{
    CFE_Status_t Status;
    Status = CFE_ES_GetAppID(&appId);
    CHECK_STATUS(Status, MSG_ERROR);

    Status = initPipes();
    CHECK_STATUS(Status, MSG_ERROR);
    Status = initTables();
    CHECK_STATUS(Status, MSG_ERROR);
    return Status;
}

bool TrickCFSScheduler::initPipes()
{
    return false;
}

bool TrickCFSScheduler::initTables()
{
    size_t TableSize = SCH_TOTAL_SLOTS * SCH_ENTRIES_PER_SLOT * sizeof(Generic_ScheduleEntry_t);
    int32 Status = CFE_TBL_Register(&scheduleTableHandle,
                                    schTableName.c_str(),
                                    TableSize,
                                    CFE_TBL_OPT_DEFAULT,
                                    validateScheduleData);
    CHECK_STATUS(Status, MSG_ERROR);

    TableSize = SCH_MAX_MESSAGES * sizeof(Generic_MessageEntry_t);
    Status = CFE_TBL_Register(&messageTableHandle,
                              msgTableName.c_str(),
                              TableSize,
                              CFE_TBL_OPT_DEFAULT,
                              validateMessageData);
    CHECK_STATUS(Status, MSG_ERROR);

    Status = CFE_TBL_Load(scheduleTableHandle, CFE_TBL_SRC_FILE, (const void *)schTablePath.c_str());
    CHECK_STATUS(Status, MSG_ERROR);

    Status = CFE_TBL_Load(messageTableHandle, CFE_TBL_SRC_FILE, (const void *)msgTablePath.c_str());
    CHECK_STATUS(Status, MSG_ERROR);

    CFE_TBL_Manage(scheduleTableHandle);
    CFE_TBL_Manage(messageTableHandle);

    Status = CFE_TBL_GetAddress((void **)&scheduleTable, scheduleTableHandle);
    if(Status == CFE_SUCCESS || Status == CFE_TBL_INFO_UPDATED)
    {
        Status = 0;
    }
    CHECK_STATUS(Status, MSG_ERROR);

    Status = CFE_TBL_GetAddress((void **)&messageTable, messageTableHandle);
    if(Status == CFE_SUCCESS || Status == CFE_TBL_INFO_UPDATED)
    {
        Status = 0;
    }
    CHECK_STATUS(Status, MSG_ERROR);

    return false;
}

size_t TrickCFSScheduler::loop()
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

bool TrickCFSScheduler::cleanup()
{
    return false;
}

bool TrickCFSScheduler::processScheduleTable()
{
    int32 status = CFE_SUCCESS;

    nextSlotNumber = updateTimeTics();

    status = processNextSlot();
    CHECK_STATUS(status, MSG_ERROR);

    return (status);
}

uint32 TrickCFSScheduler::updateTimeTics()
{
    const long long SCH_MICROS_PER_MAJOR_FRAME = 1000000;
    const long long SCH_NORMAL_SLOT_PERIOD = (SCH_MICROS_PER_MAJOR_FRAME / SCH_TOTAL_SLOTS);

    /* Determine which time slot we are in from Trick executive. */
    int tics_per_sec = exec_get_time_tic_value();

    simTimeSecs = exec_get_time_tics() / tics_per_sec;
    simTimeMicros = exec_get_time_tics() % tics_per_sec;

    /*
     ** Calculate schedule table slot number
     */
    uint32 schSlot = simTimeMicros / SCH_NORMAL_SLOT_PERIOD;

    /*
     ** Check to see if close enough to round up to next slot
     */
    long long Remainder = simTimeMicros - (schSlot * SCH_NORMAL_SLOT_PERIOD);

    /*
     ** Add one more microsecond and see if it is sufficient to add another slot
     */
    Remainder += 1;
    schSlot += (Remainder / SCH_NORMAL_SLOT_PERIOD);

    /*
     ** Check to see if the Current Slot number needs to roll over
     */
    if(schSlot == SCH_TOTAL_SLOTS)
    {
        schSlot = 0;
    }

    return schSlot;
}

bool TrickCFSScheduler::processNextSlot()
{
    int32 SlotIndex = nextSlotNumber * SCH_ENTRIES_PER_SLOT;
    Generic_ScheduleEntry_t * NextEntry = &scheduleTable[SlotIndex];

    /*
     ** Process each (enabled) entry in the schedule table slot
     */
    for(int32 EntryNumber = 0; EntryNumber < SCH_ENTRIES_PER_SLOT; ++EntryNumber)
    {
        if(NextEntry->EnableState == SCH_ENABLED)
        {
            if(processNextEntry(NextEntry, EntryNumber))
            {
                return true;
            }
        }
        NextEntry++;
    }
    nextSlotNumber++;
    return false;
}

bool TrickCFSScheduler::processNextEntry(Generic_ScheduleEntry_t * NextEntry, int32 EntryNumber)
{
    /*
     ** Check for invalid table entry
     **
     ** (run time corruption -- data was verified at table load)
     */
    if((NextEntry->MessageIndex >= SCH_MAX_MESSAGES) || (NextEntry->Frequency == SCH_UNUSED) ||
       (NextEntry->Type != SCH_ACTIVITY_SEND_MSG) || (NextEntry->Remainder >= NextEntry->Frequency))
    {
        message_publish(MSG_ERROR,
                        "TrickCFSScheduler %s:%d error: Corrupt data error (1): slot = %d, entry = %d\n",
                        __PRETTY_FUNCTION__,
                        __LINE__,
                        nextSlotNumber,
                        EntryNumber);
        message_publish(
            MSG_ERROR,
            "TrickCFSScheduler %s:%d error: Corrupt data error (2): msg = %d, freq = %d, type = %d, rem = %d\n",
            __PRETTY_FUNCTION__,
            __LINE__,
            NextEntry->MessageIndex,
            NextEntry->Frequency,
            NextEntry->Type,
            NextEntry->Remainder);

        /*
         ** Disable entry to avoid repeating this error
         */
        NextEntry->EnableState = SCH_DISABLED;
        CFE_TBL_Modified(scheduleTableHandle);
        return true;
    }
    else
    {
        int32 Status;
        Generic_MessageEntry_t * messageEntryPtr = &messageTable[NextEntry->MessageIndex];
        CFE_SB_MsgId_t MsgId = messageEntryPtr->mid;
        CFE_MSG_Message_t * message = (CFE_MSG_Message_t *)messageEntryPtr->MessageBuffer;
        CFE_ES_AppId_t AppId;
        CFE_ES_GetAppID(&AppId);

        /*
         ** Look for entry active on this particular pass through table
         */
        uint32 Remainder = simTimeSecs % NextEntry->Frequency;

        if(Remainder == NextEntry->Remainder)
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

                if((PipeDscPtr->Opts & CFE_SB_PIPEOPTS_IGNOREMINE) != 0 &&
                   CFE_RESOURCEID_TEST_EQUAL(PipeDscPtr->AppId, appId))
                {
                    continue;
                } /* end if */

                osal_index_t pipeId;
                OS_ConvertToArrayIndex(PipeDscPtr->SysQueueId, &pipeId);

                SCH_TRICK_mark_pipe_as_tiggered(pipeId, CFE_SB_MsgIdToValue(MsgId));

            } /* end loop over destinations */

            CFE_SB_UnlockSharedData(__func__, __LINE__);

            Status = CFE_SB_TransmitMsg(message, false);

            if(Status != CFE_SUCCESS)
            {
                message_publish(MSG_ERROR,
                                "TrickCFSScheduler %s:%d error: Activity error: slot = %d, entry = %d, err = 0x%08X\n",
                                __PRETTY_FUNCTION__,
                                __LINE__,
                                nextSlotNumber,
                                (int)EntryNumber,
                                (unsigned int)Status);
                return true;
            }
        }
    }

    return false;
}

int32 TrickCFSScheduler::validateScheduleData(void * TableData)
{
    Generic_ScheduleEntry_t * TableArray = (Generic_ScheduleEntry_t *)TableData;
    int32 EntryResult = CFE_SUCCESS;
    int32 TableResult = CFE_SUCCESS;
    int32 TableIndex;

    uint8 EnableState;
    uint8 Type;
    uint16 Frequency;
    uint16 Remainder;
    uint16 MessageIndex;
    uint32 GroupData;

    int32 GoodCount = 0;
    int32 BadCount = 0;
    int32 UnusedCount = 0;

    /*
     ** Verify each entry in pending SCH schedule table
     */
    for(TableIndex = 0; TableIndex < SCH_TABLE_ENTRIES; TableIndex++)
    {
        EnableState = TableArray[TableIndex].EnableState;
        Type = TableArray[TableIndex].Type;
        Frequency = TableArray[TableIndex].Frequency;
        Remainder = TableArray[TableIndex].Remainder;
        MessageIndex = TableArray[TableIndex].MessageIndex;
        GroupData = TableArray[TableIndex].GroupData;

        EntryResult = CFE_SUCCESS;

        if(EnableState == SCH_UNUSED)
        {
            /*
             ** If enable state is unused, then all fields must be unused
             */
            if((Frequency != SCH_UNUSED) || (Remainder != SCH_UNUSED) || (GroupData != SCH_UNUSED) ||
               (Type != SCH_UNUSED) || (MessageIndex != SCH_UNUSED))
            {
                EntryResult = SCH_SDT_GARBAGE_ENTRY;
                BadCount++;
            }
            else
            {
                UnusedCount++;
            }
        }
        else if((EnableState == SCH_ENABLED) || (EnableState == SCH_DISABLED))
        {
            /*
             ** If enable state is used, then verify all fields
             **
             **  - Frequency must be non-zero
             **  - Remainder must be < Frequency
             **  - Type must be SCH_ACTIVITY_SEND_MSG
             **  - MessageIndex must be non-zero (reserved value = "unused")
             **  - MessageIndex must be < SCH_MAX_MESSAGES
             */
            if(Frequency == SCH_UNUSED)
            {
                EntryResult = SCH_SDT_NO_FREQUENCY;
            }
            else if(Remainder >= Frequency)
            {
                EntryResult = SCH_SDT_BAD_REMAINDER;
            }
            else if(Type != SCH_ACTIVITY_SEND_MSG)
            {
                EntryResult = SCH_SDT_BAD_ACTIVITY;
            }
            else if(MessageIndex == 0)
            {
                EntryResult = SCH_SDT_BAD_MSG_INDEX;
            }
            else if(MessageIndex >= SCH_MAX_MESSAGES)
            {
                EntryResult = SCH_SDT_BAD_MSG_INDEX;
            }

            if(EntryResult != CFE_SUCCESS)
            {
                BadCount++;
            }
            else
            {
                GoodCount++;
            }
        }
        else
        {
            EntryResult = SCH_SDT_BAD_ENABLE_STATE;
            BadCount++;
        }

        /*
         ** Send event for "first" error found
         */
        if((EntryResult != CFE_SUCCESS) && (TableResult == CFE_SUCCESS))
        {
            TableResult = EntryResult;

            message_publish(MSG_ERROR,
                            "TrickCFSScheduler %s:%d error: Schedule tbl verify error - idx[%d] ena[%d] typ[%d] "
                            "fre[%d] rem[%d] msg[%d] grp[0x%08X]\n",
                            __PRETTY_FUNCTION__,
                            __LINE__,
                            (int)TableIndex,
                            EnableState,
                            Type,
                            Frequency,
                            Remainder,
                            MessageIndex,
                            (unsigned int)GroupData);
        }
    }

    /*
     ** Send event describing results
     */
    message_publish(MSG_INFO,
                    "TrickCFSScheduler %s:%d reports: Schedule table verify results -- good[%d] bad[%d] unused[%d]\n",
                    __PRETTY_FUNCTION__,
                    __LINE__,
                    (int)GoodCount,
                    (int)BadCount,
                    (int)UnusedCount);

    return (TableResult);
}

double TrickCFSScheduler::getMinorFrameRate()
{
    return (1.0) / SCH_TOTAL_SLOTS;
}

uint32 TrickCFSScheduler::getMinorFrameTics()
{
    double schRateDbl = (1.0) / SCH_TOTAL_SLOTS;
    return (uint32)(schRateDbl * 1000);
}

int32 TrickCFSScheduler::validateMessageData(void * TableData)
{
    Generic_MessageEntry_t * TableArray = (Generic_MessageEntry_t *)TableData;
    int32 EntryResult = CFE_SUCCESS;
    int32 TableResult = CFE_SUCCESS;
    int32 TableIndex;
    int32 BufferIndex;

    uint8 * MessageBuffer = NULL;
    uint8 * UserDataPtr = NULL;
    uint8 * EndDataPtr = NULL;

    uint16 PayloadLength;
    uint16 CmdCode;
    CFE_SB_MsgId_Atom_t MessageID;
    CFE_SB_MsgId_Atom_t MaxValue = CFE_PLATFORM_SB_HIGHEST_VALID_MSGID;
    CFE_SB_MsgId_Atom_t MinValue = CFE_SB_MsgIdToValue(CFE_SB_INVALID_MSG_ID);

    int32 GoodCount = 0;
    int32 BadCount = 0;
    int32 UnusedCount = 0;

    /*
     ** Verify each entry in pending SCH Message table
     */
    for(TableIndex = 0; TableIndex < SCH_MAX_MESSAGES; TableIndex++)
    {
        EntryResult = CFE_SUCCESS;
        BufferIndex = 0;

        MessageBuffer = (uint8 *)&TableArray[TableIndex].MessageBuffer[0];
        MessageID = CFE_SB_MsgIdToValue(TableArray[TableIndex].mid);
        CmdCode = TableArray[TableIndex].cmdCode;
        PayloadLength = TableArray[TableIndex].payloadLength;

        if(MessageID == SCH_UNUSED_MID)
        {
            /*
             ** If message ID is unused, then look for junk in user data portion
             */
            UnusedCount++;
            UserDataPtr = MessageBuffer;
            /* Get first address beyond messageBuffer array */
            EndDataPtr = ((uint8 *)(&TableArray[TableIndex].MessageBuffer[SCH_MAX_MSG_WORDS]));
            while(UserDataPtr < EndDataPtr)
            {
                if(*UserDataPtr != SCH_UNUSED)
                {
                    EntryResult = SCH_MDT_GARBAGE_ENTRY;
                    BadCount++;
                    UnusedCount--;
                    break;
                }
                UserDataPtr++;
            }
        }
        else if((MessageID <= MaxValue) && (MessageID >= MinValue))
        {
            /*
             ** If message ID is valid, then check message length
             */
            if(PayloadLength > SCH_MAX_MSG_WORDS * 2)
            {
                EntryResult = SCH_MDT_INVALID_LENGTH;
                BadCount++;
            }
            else
            {
                GoodCount++;
                // Populate the CCSDS header and move the message content into the proper user data space.
                uint16 TempBuffer[SCH_MAX_MSG_WORDS];
                if(PayloadLength > 0)
                {
                    CFE_PSP_MemCpy(TempBuffer, MessageBuffer, PayloadLength);
                }

                CFE_MSG_Type_t Type = CFE_MSG_Type_Invalid;
                CFE_MSG_GetTypeFromMsgId(CFE_SB_ValueToMsgId(MessageID), &Type);
                if(Type == CFE_MSG_Type_Cmd)
                {
                    CFE_MSG_Init((CFE_MSG_Message_t *)MessageBuffer,
                                 CFE_SB_ValueToMsgId(MessageID),
                                 sizeof(CFE_MSG_CommandHeader_t));
                    CFE_MSG_SetSize((CFE_MSG_Message_t *)MessageBuffer,
                                    sizeof(CFE_MSG_CommandHeader_t) + PayloadLength);
                    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)MessageBuffer, CmdCode);
                    CFE_PSP_MemCpy(MessageBuffer + sizeof(CFE_MSG_CommandHeader_t), TempBuffer, PayloadLength);
                }
                else
                {
                    CFE_MSG_Init((CFE_MSG_Message_t *)MessageBuffer,
                                 CFE_SB_ValueToMsgId(MessageID),
                                 sizeof(CFE_MSG_TelemetryHeader_t));
                    CFE_MSG_SetSize((CFE_MSG_Message_t *)MessageBuffer,
                                    sizeof(CFE_MSG_TelemetryHeader_t) + PayloadLength);
                    CFE_PSP_MemCpy(MessageBuffer + sizeof(CFE_MSG_TelemetryHeader_t), TempBuffer, PayloadLength);
                }
            }
        }
        else
        {
            EntryResult = SCH_MDT_BAD_MSG_ID;
            BadCount++;
        }

        /*
         ** Save index of "first" error found
         */
        if((EntryResult != CFE_SUCCESS) && (TableResult == CFE_SUCCESS))
        {
            TableResult = EntryResult;

            message_publish(
                MSG_ERROR,
                "TrickCFSScheduler %s:%d error: Message tbl verify err - idx[%d] mid[0x%X] len[%d] buf[%d]\n",
                __PRETTY_FUNCTION__,
                __LINE__,
                (int)TableIndex,
                MessageID,
                PayloadLength,
                (int)BufferIndex);
        }
    }

    /*
     ** Send event describing results
     */
    message_publish(MSG_INFO,
                    "TrickCFSScheduler %s:%d reports: Schedule table verify results -- good[%d] bad[%d] unused[%d]\n",
                    __PRETTY_FUNCTION__,
                    __LINE__,
                    (int)GoodCount,
                    (int)BadCount,
                    (int)UnusedCount);

    return (TableResult);
}
