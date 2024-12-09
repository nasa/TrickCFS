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
 (Interface class between CFS and Trick. See Doxygen comments below.)

 LIBRARY DEPENDENCIES:
 (
 (TrickCFSInterface.o)
 (TrickCFS_C_Interface.cpp)
 (Trick-posix/src/bsp_start.o)
 (Trick-pc-linux/src/cfe_psp_start.o)
 (Trick-pc-linux/src/cfe_psp_exception.o)
 (Trick-posix/src/os-impl-tasks.o)
 (Trick-posix/src/bsp_console.o)
 (Trick-posix/src/os-impl-posix-gettime.o)
 )
 *******************************************************************************/

#ifndef APPS_SCH_TRICK_FSW_SRC_TRICKCFSINTERFACE_HH_
#define APPS_SCH_TRICK_FSW_SRC_TRICKCFSINTERFACE_HH_

#include "osconfig.h"
#include "pthread.h"

#include <list>
#include <map>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This class assumes the role of the CFS SCH app and uses the same interface to signal CFS apps in the schedule table.
/// The accompanying code in this directory is a copy of the SCH app. It has been modified to rely on simulation time
/// rather than wall clock time. Additionally, when signaling a CFS APP to wake up, it keeps track of the number of
/// apps that have been triggered that have at least one subscribed app to the wake up message. This class has methods
/// to be called from the Trick S_define that will wait for the same number of apps to complete to guarantee
/// synchronization. This class also provides an interface for the Trick OSAL functions to call for OS_TaskDelay.
/// It handles tasks that utilize sleep instead of a wake up call for synchronization.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrickCFSInterface
{
public:
    TrickCFSInterface();
    virtual ~TrickCFSInterface();

    std::string cfsOutputFile; /* -- File where output from CFS and apps is captured. Default is
                                  [RUN_directory]/cfs_out.txt */

    void addCFSCPUAffinity(int cpuIdxIn);

    void addInstance(const std::string & symbolNameIn, const std::string & varDeclIn);

    void initialize(const std::string & jobName);

    void updateCFS();

    bool perfectScheduleMode;
    int msWaitTimeForCFSApps;

    void signalMainAppComplete();
    void CFSWaitForTrigger();
    void processExternalDeclarations();

    void setPipeOwner(size_t pipeId, size_t taskId);
    void markPipeAsTriggered(size_t appId, size_t mid);

    void acknowledgePipeTrigger(size_t taskId, size_t mid);

    void markPipeAsComplete(size_t pipeId, size_t taskId);

    void addTaskDelay(size_t taskId, size_t msIn);

    void setAffinityAttribute(pthread_attr_t & attrIn);

    int getOutputFd();

protected:
    void updateTaskTimes();
    void triggerMainApp();
    void TrickWaitForCFSComplete();
    void calculateAbsTimeFromDelta(int msecs, struct timespec * tm);

#if !defined(SWIG) && !defined(TRICK_ICG)
    class Instance
    {
    public:
        Instance(const std::string & symbolNameIn, const std::string & varDeclIn);
        std::string symbolName;
        std::string varDecl;
    };

    class TaskIPC
    {
    public:
        enum TaskState
        {
            NONE = 0,
            TASK_DELAY,
            MSG_RCV,
            TASK_RUNNING,
            TASK_COMPLETE,
            TASK_CONTINUE,
        };

        typedef struct StackInfo
        {
            static const int SIZE_TRACE = 10;
            void * cacheBackTrace[SIZE_TRACE];
            int cacheBackTraceSize;
        } StackInfo;

        TaskIPC();
        virtual ~TaskIPC();

        std::list<size_t> triggeredMIDs;
        std::list<StackInfo *> stacks;
        std::list<size_t> acknowledgedMIDs;
        long long usRemaining;
        size_t taskId;
        TaskState state;

        pthread_mutex_t cvMutex;
        pthread_cond_t cv;
    };

    std::vector<Instance> declarations;
    std::vector<int> cpuAffinityIdxs;

    TaskIPC taskStates[OS_MAX_TASKS];
    size_t prevPipeTaskOwner[OS_MAX_QUEUES];

    std::multimap<size_t, TaskIPC *> perfectFDMap;

    FILE * outputFilePtr;

#if __linux
    int isMainCompleteTrigger;
    int triggerMain;
#else
    pthread_cond_t isMainCompleteTrigger;
    pthread_cond_t triggerMain;
    pthread_mutex_t triggerMutex;
#endif

#endif
};

extern "C"
{
    int psp_main_wrapper(void);
    int OS_shutdown(void);
    int OS_Application_Cycle(void);
}

#endif /* APPS_SCH_TRICK_FSW_SRC_TRICKCFSINTERFACE_HH_ */
