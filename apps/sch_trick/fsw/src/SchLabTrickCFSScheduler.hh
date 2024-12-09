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

/*******************************************************************************
 PURPOSE:
 (Base class for TrickCFSScheduler app. Derived classes should implement enough
 functionality to replace the schedule app from the project)

 LIBRARY DEPENDENCIES:
 (
 )
 *******************************************************************************/

#ifndef TRICKCFS_APPS_SCH_TRICK_FSW_SRC_SCHLABTRICKCFSSCHEDULER_H_
#define TRICKCFS_APPS_SCH_TRICK_FSW_SRC_SCHLABTRICKCFSSCHEDULER_H_

#include "BaseTrickCFSScheduler.hh"
#include <vector>

extern "C"
{
#include "sch_lab_interface_cfg.h"
#include "sch_lab_tbldefs.h"
}

// This is the estimated structure of each entry.
// Cannot include the header for it because it was defined in a source file.
typedef struct SchLabStateEntry_t
{
    CFE_MSG_CommandHeader_t CommandHeader;
    uint16 MessageBuffer[SCH_LAB_MAX_ARGS_PER_ENTRY];
    uint16 PayloadLength;
    CFE_SB_MsgId_t MsgId;
    uint32 PacketRate;
    uint32 Counter;
} SchLabStateEntry_t;

class SchLabTrickCFSScheduler : public BaseTrickCFSScheduler
{
public:
    SchLabTrickCFSScheduler();
    virtual ~SchLabTrickCFSScheduler();

    virtual double getMinorFrameRate();
    virtual size_t getMinorFrameMsTics();

    virtual size_t initTables();
    virtual size_t processScheduleTable();

    void addScheduledCmd(size_t mid, double cmdRate, size_t cmdCode, size_t payloadLength);

    SchLabStateEntry_t State[SCH_LAB_MAX_SCHEDULE_ENTRIES];
    CFE_TBL_Handle_t TblHandle;

    size_t schedRateInMicros; /* (microseconds) Microsends per scheduler frame */
    size_t schedFreq;         /* (Hz) Frequency of scheduler frame */

protected:
    std::vector<SchLabStateEntry_t> userAddedCmds;

private:
    SchLabTrickCFSScheduler(const SchLabTrickCFSScheduler & other);
    SchLabTrickCFSScheduler & operator=(const SchLabTrickCFSScheduler & other);
};

#endif /* TRICKCFS_APPS_SCH_TRICK_FSW_SRC_SCHLABTRICKCFSSCHEDULER_H_ */
