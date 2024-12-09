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
 * SchLabTrickCFSScheduler.cpp
 *
 *  Created on: May 22, 2024
 *      Author: tbrain
 */

#include "SchLabTrickCFSScheduler.hh"

extern "C"
{
#include "cfe_config.h"
#include "sch_lab_tbl.h"
#include "sch_lab_tblstruct.h"
#include "sch_lab_version.h"
}

SchLabTrickCFSScheduler::SchLabTrickCFSScheduler()
    : BaseTrickCFSScheduler("SchLabTrickCFSScheduler"),
      TblHandle(0),
      schedRateInMicros(0),
      schedFreq(0)
{
    memset(State, 0, sizeof(State));
}

SchLabTrickCFSScheduler::~SchLabTrickCFSScheduler()
{
    // TODO Auto-generated destructor stub
}

size_t SchLabTrickCFSScheduler::initTables()
{
    int i, x;
    CFE_Status_t Status;
    uint32 TimerPeriod;
    SCH_LAB_ScheduleTable_t * ConfigTable;
    SCH_LAB_ScheduleTableEntry_t * ConfigEntry;
    SchLabStateEntry_t * LocalStateEntry;
    void * TableAddr;
    char VersionString[SCH_LAB_CFG_MAX_VERSION_STR_LEN];

    /*
     ** Register tables with cFE and load default data
     */
    Status = CFE_TBL_Register(&TblHandle, "ScheduleTable", sizeof(SCH_LAB_ScheduleTable_t), CFE_TBL_OPT_DEFAULT, NULL);

    TRICKCFS_CHECK_STATUS(Status, MSG_ERROR);

    if(Status != CFE_SUCCESS)
    {
        message_publish(MSG_ERROR,
                        "SCH_LAB_TRICKCFS: Error Registering ScheduleTable, RC = 0x%08lX\n",
                        (unsigned long)Status);

        return Status;
    }
    else
    {
        /*
         ** Loading Table
         */
        Status = CFE_TBL_Load(TblHandle, CFE_TBL_SRC_FILE, SCH_LAB_TBL_DEFAULT_FILE);
        if(Status != CFE_SUCCESS)
        {
            message_publish(MSG_ERROR,
                            "SCH_LAB_TRICKCFS: Error Loading Table ScheduleTable, RC = 0x%08lX\n",
                            (unsigned long)Status);
            CFE_TBL_ReleaseAddress(TblHandle);

            return Status;
        }
    }

    /*
     ** Get Table Address
     */
    Status = CFE_TBL_GetAddress(&TableAddr, TblHandle);
    if(Status != CFE_SUCCESS && Status != CFE_TBL_INFO_UPDATED)
    {
        message_publish(MSG_ERROR,
                        "SCH_LAB_TRICKCFS: Error Getting Table's Address ScheduleTable, RC = 0x%08lX\n",
                        (unsigned long)Status);

        return Status;
    }

    /*
     ** Initialize the command headers
     */
    ConfigTable = static_cast<SCH_LAB_ScheduleTable_t *>(TableAddr);
    ConfigEntry = ConfigTable->Config;
    LocalStateEntry = State;

    /* Populate the CCSDS message and move the message content into the proper user data space. */
    for(i = 0; i < SCH_LAB_MAX_SCHEDULE_ENTRIES; i++)
    {
        if(ConfigEntry->PacketRate != 0)
        {
            LocalStateEntry->MsgId = ConfigEntry->MessageID;

            /* Initialize the message with the length of the header + payload */
            CFE_MSG_Init(CFE_MSG_PTR(LocalStateEntry->CommandHeader),
                         ConfigEntry->MessageID,
                         sizeof(LocalStateEntry->CommandHeader) + ConfigEntry->PayloadLength);
            CFE_MSG_SetFcnCode(CFE_MSG_PTR(LocalStateEntry->CommandHeader), ConfigEntry->FcnCode);

            LocalStateEntry->PacketRate = ConfigEntry->PacketRate;
            LocalStateEntry->PayloadLength = ConfigEntry->PayloadLength;

            for(x = 0; x < SCH_LAB_MAX_ARGS_PER_ENTRY; x++)
            {
                LocalStateEntry->MessageBuffer[x] = ConfigEntry->MessageBuffer[x];
            }
        }
        ++ConfigEntry;
        ++LocalStateEntry;
    }

    if(ConfigTable->TickRate == 0)
    {
        /* use default of 1 second */
        message_publish(MSG_INFO, "%s: Using default tick rate of 1 second\n", __func__);
        TimerPeriod = 1000000;
    }
    else
    {
        TimerPeriod = 1000000 / ConfigTable->TickRate;
        if((TimerPeriod * ConfigTable->TickRate) != 1000000)
        {
            message_publish(MSG_WARNING,
                            "%s: WARNING: tick rate of %lu is not an integer number of microseconds\n",
                            __func__,
                            (unsigned long)ConfigTable->TickRate);
        }
    }
    schedRateInMicros = TimerPeriod;
    schedFreq = ConfigTable->TickRate;

    /*
     ** Release the table
     */
    Status = CFE_TBL_ReleaseAddress(TblHandle);
    if(Status != CFE_SUCCESS)
    {
        message_publish(MSG_ERROR,
                        "SCH_LAB_TRICKCFS: Error Releasing Table ScheduleTable, RC = 0x%08lX\n",
                        (unsigned long)Status);
    }

    for(size_t userEntryIdx = 0; userEntryIdx < userAddedCmds.size(); ++userEntryIdx)
    {
        SchLabStateEntry_t & newEntry = userAddedCmds[userEntryIdx];
        LocalStateEntry = State;
        for(i = 0; i < SCH_LAB_MAX_SCHEDULE_ENTRIES; i++)
        {
            if(LocalStateEntry->PacketRate == 0 && CFE_SB_MsgId_Equal(LocalStateEntry->MsgId, CFE_SB_INVALID_MSG_ID))
            {
                memcpy(LocalStateEntry, &newEntry, sizeof(*LocalStateEntry));
                LocalStateEntry->PacketRate = newEntry.PacketRate / schedRateInMicros;
                break;
            }
            ++LocalStateEntry;
        }
    }

    CFE_Config_GetVersionString(VersionString,
                                SCH_LAB_CFG_MAX_VERSION_STR_LEN,
                                "SCH Lab TrickCFS",
                                SCH_LAB_VERSION,
                                SCH_LAB_BUILD_CODENAME,
                                SCH_LAB_LAST_OFFICIAL);

    OS_printf("SCH Lab TrickCFS Initialized.%s\n", VersionString);

    return CFE_SUCCESS;
}

double SchLabTrickCFSScheduler::getMinorFrameRate()
{
    return static_cast<double>(schedRateInMicros) / 1.0E6;
}

size_t SchLabTrickCFSScheduler::getMinorFrameMsTics()
{
    return schedRateInMicros / 1E3;
}

size_t SchLabTrickCFSScheduler::processScheduleTable()
{
    /*
     ** Process table every tick, sending packets that are ready
     */
    SchLabStateEntry_t * LocalStateEntry = State;

    for(size_t i = 0; i < SCH_LAB_MAX_SCHEDULE_ENTRIES; i++)
    {
        if(LocalStateEntry->PacketRate != 0)
        {
            ++LocalStateEntry->Counter;
            if(LocalStateEntry->Counter >= LocalStateEntry->PacketRate)
            {
                LocalStateEntry->Counter = 0;
                sendScheduledMessage(LocalStateEntry->MsgId, CFE_MSG_PTR(LocalStateEntry->CommandHeader), true);
            }
        }
        ++LocalStateEntry;
    }

    return 0;
}

void SchLabTrickCFSScheduler::addScheduledCmd(size_t mid, double cmdRate, size_t cmdCode, size_t payloadLength)
{
    SchLabStateEntry_t newEntry = {};
    CFE_SB_MsgId_t cfsMid = CFE_SB_ValueToMsgId(mid);

    CFE_MSG_Init(CFE_MSG_PTR(newEntry.CommandHeader), cfsMid, sizeof(newEntry.CommandHeader) + payloadLength);
    CFE_MSG_SetFcnCode(CFE_MSG_PTR(newEntry.CommandHeader), cmdCode);

    newEntry.PayloadLength = payloadLength;
    newEntry.MsgId = cfsMid;
    newEntry.PacketRate = static_cast<uint32_t>(
        cmdRate * 1000000); // Absolute rate here in microseconds. Convert to tickrate later

    userAddedCmds.push_back(newEntry);
}
