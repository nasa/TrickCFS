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

#ifndef TRICKCFS_APPS_SCH_TRICK_FSW_SRC_BASETRICKCFSSCHEDULER_HH_
#define TRICKCFS_APPS_SCH_TRICK_FSW_SRC_BASETRICKCFSSCHEDULER_HH_

#include "TrickCFS_C_proto.h"
#include "sim_services/Message/include/message_proto.h"
#include <stddef.h>
#include <string>

extern "C"
{
#include "cfe.h"
}

#define TRICKCFS_CHECK_STATUS(status, severity)                                                                        \
    if(status != 0)                                                                                                    \
    {                                                                                                                  \
        message_publish(severity, "Error code %d detected in %s:%d\n", status, __PRETTY_FUNCTION__, __LINE__ - 1);     \
        return status;                                                                                                 \
    }

class BaseTrickCFSScheduler
{
public:
    friend class InputProcessor;
    friend void init_attrBaseTrickCFSScheduler();

    BaseTrickCFSScheduler();
    BaseTrickCFSScheduler(const std::string & typeNameIn);
    virtual ~BaseTrickCFSScheduler();

    virtual void mainLoop();

    virtual double getMinorFrameRate() = 0;
    virtual size_t getMinorFrameMsTics() = 0;

    virtual size_t init();

    virtual size_t loop();

    virtual size_t cleanup();

    virtual size_t initPipes();
    virtual size_t initTables() = 0;
    virtual size_t processScheduleTable() = 0;

    CFE_ES_AppId_t appId;
    bool isInitialized;

protected:
    const std::string schedTypeName;

    virtual void sendScheduledMessage(CFE_SB_MsgId_t MsgId, CFE_MSG_Message_t * message, bool allowSBUpdateHeader);

private:
    BaseTrickCFSScheduler & operator=(const BaseTrickCFSScheduler & other);
    BaseTrickCFSScheduler(const BaseTrickCFSScheduler & other);
};

#endif /* TRICKCFS_APPS_SCH_TRICK_FSW_SRC_BASETRICKCFSSCHEDULER_H_ */
