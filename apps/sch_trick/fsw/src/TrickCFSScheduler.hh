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
 (TrickCFS Scheduler app. )

 LIBRARY DEPENDENCIES:
 (
 )
 *******************************************************************************/

#ifndef APPS_SCH_TRICK_FSW_TRICKCFSSCHEDULER_HH_
#define APPS_SCH_TRICK_FSW_TRICKCFSSCHEDULER_HH_

#include <string>

extern "C"
{
#include "GenericSchTable.h"
#include "cfe_es_extern_typedefs.h"
#include "cfe_tbl.h"
}

class TrickCFSScheduler
{
public:
    friend class InputProcessor;
    friend void init_attrTrickCFSScheduler();
    TrickCFSScheduler();
    virtual ~TrickCFSScheduler();
    virtual void mainLoop();

    std::string schTableName;
    std::string msgTableName;
    std::string schTablePath;
    std::string msgTablePath;

    double getMinorFrameRate();
    uint32 getMinorFrameTics();

protected:
    virtual bool init();

    virtual size_t loop();

    virtual bool cleanup();

    virtual bool initPipes();
    virtual bool initTables();
    virtual bool processScheduleTable();
    virtual bool processNextSlot();
    virtual bool processNextEntry(Generic_ScheduleEntry_t * NextEntry, int32 EntryNumber);

    virtual uint32 updateTimeTics();

    static int32 validateScheduleData(void * TableData);
    static int32 validateMessageData(void * TableData);

    CFE_TBL_Handle_t scheduleTableHandle;
    CFE_TBL_Handle_t messageTableHandle;
    uint32 nextSlotNumber;
    Generic_ScheduleEntry_t * scheduleTable;
    Generic_MessageEntry_t * messageTable;
    long long simTimeSecs;
    long long simTimeMicros;
    CFE_ES_AppId_t appId;

    // private:
    //     // Unimplemented copy constructor and assignment operator
    //     TrickCFSScheduler(const TrickCFSScheduler&);
    //     TrickCFSScheduler& operator =(const TrickCFSScheduler&);
};

#endif /* APPS_SCH_TRICK_FSW_TRICKCFSSCHEDULER_HH_ */
