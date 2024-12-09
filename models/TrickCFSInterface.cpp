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
 * ExternalVariableDeclarator.cpp
 *
 *  Created on: May 15, 2017
 *      Author: tbrain
 */

#include "TrickCFSInterface.hh"
#include "TrickCFS_C_proto.h"

#ifdef TRICKCFS_DEBUG
#ifdef TRICKCFS_DEBUG_CONDITON
#define TRICKCFS_DEBUG_OUT(out)                                                                                        \
    if(TRICKCFS_DEBUG_CONDITON)                                                                                        \
    {                                                                                                                  \
        out;                                                                                                           \
    }
#else
#define TRICKCFS_DEBUG_OUT(out) out;
#endif
#else
#define TRICKCFS_DEBUG_OUT(out)
#endif

extern "C"
{
#include "cfe.h"
#include "cfe_es_apps.h"
// clang-format off
#include "cfe_es_mempool.h"
#include "cfe_es_perf.h"
#include "cfe_es_global.h"
// clang-format on
#include "cfe_es_resource.h"
    extern CFE_ES_Global_t CFE_ES_Global;
    extern void OS_CompAbsDelayTime(uint32 milli_second, struct timespec * tm);
}

#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/exec_proto.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#if __linux

#include <execinfo.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <execinfo.h>
#endif

TrickCFSInterface * the_trickToCFSInterface = 0x0;

class MutexLock
{
public:
    MutexLock(pthread_mutex_t & mutexIn);
    MutexLock(const pthread_mutex_t & mutexIn);
    virtual ~MutexLock();

    void lock();
    void unlock();

    pthread_mutex_t & myMutex;
    bool isLocked;
};

MutexLock::MutexLock(pthread_mutex_t & mutexIn)
    : myMutex(mutexIn),
      isLocked(false)
{
    lock();
}

MutexLock::~MutexLock()
{
    if(isLocked)
    {
        unlock();
    }
}

void MutexLock::lock()
{
    pthread_mutex_lock(&myMutex);
    isLocked = true;
}

MutexLock::MutexLock(const pthread_mutex_t & mutexIn)
    : myMutex(const_cast<pthread_mutex_t &>(mutexIn))
{
    lock();
}

void MutexLock::unlock()
{
    isLocked = false;
    pthread_mutex_unlock(&myMutex);
}

TrickCFSInterface::TrickCFSInterface()
{
    perfectScheduleMode = false;
    msWaitTimeForCFSApps = -1;
    outputFilePtr = 0x0;
#if __linux
    isMainCompleteTrigger = eventfd(0, 0);
    triggerMain = eventfd(0, 0);
#else
    pthread_cond_init(&isMainCompleteTrigger, NULL);
    pthread_cond_init(&triggerMain, NULL);
    pthread_mutex_init(&triggerMutex, NULL);
#endif

    for(size_t ii = 0; ii < OS_MAX_TASKS; ++ii)
    {
        taskStates[ii].taskId = ii;
    }

    memset(prevPipeTaskOwner, 0, sizeof(prevPipeTaskOwner));

    if(the_trickToCFSInterface == 0x0)
    {
        the_trickToCFSInterface = this;
    }
    else
    {
        std::string errStr("Multiple instances of ");
        errStr += __PRETTY_FUNCTION__;
        exec_terminate_with_return(-1, __FILE__, __LINE__, errStr.c_str());
    }
}

TrickCFSInterface::~TrickCFSInterface()
{
    // TODO Auto-generated destructor stub
}

void TrickCFSInterface::initialize(const std::string & jobName)
{
    if(cfsOutputFile.empty())
    {
        cfsOutputFile = command_line_args_get_output_dir();
        cfsOutputFile += "/cfs_out.txt";
    }

    outputFilePtr = fopen(cfsOutputFile.c_str(), "w");
    if(outputFilePtr == 0x0)
    {
        std::string errStr("Unable to open file ");
        errStr += cfsOutputFile;
        exec_terminate_with_return(-1, __FILE__, __LINE__, errStr.c_str());
    }

    psp_main_wrapper();
    TrickWaitForCFSComplete();
    size_t periodPos = jobName.rfind(".");
    if(periodPos != std::string::npos)
    {
        std::string jobPrefix = jobName.substr(0, periodPos);
        std::string updateJobName = jobPrefix + ".updateCFS";
        double cfsRate = SCH_TRICK_minor_frame_rate();
        exec_set_job_cycle(updateJobName.c_str(), 0, cfsRate);
        double rtFrameRate = exec_get_software_frame();
        if((std::fabs(rtFrameRate) - 1.0) < 1.0e-9)
        {
            if(cfsRate < 0.001)
            {
                message_publish(MSG_WARNING,
                                "TRICKCFS: Detected unset software frame and SCH rate is small. Defaulting to "
                                "scheduler rate of 0.001");
                exec_set_software_frame(0.001);
            }
            else
            {
                message_publish(MSG_INFO,
                                "TRICKCFS: Detected unset software frame. Defaulting to scheduler rate of %f",
                                cfsRate);
                exec_set_software_frame(cfsRate);
            }
        }
    }
}

void TrickCFSInterface::addInstance(const std::string & symbolNameIn, const std::string & varDeclIn)
{
    declarations.push_back(Instance(symbolNameIn, varDeclIn));
}

void TrickCFSInterface::addTaskDelay(size_t taskId, size_t msIn)
{
    if(exec_get_mode() != Initialization && exec_get_mode() != ExitMode)
    {
        CFE_ES_LockSharedData(__func__, __LINE__);
        TaskIPC & currTask = taskStates[taskId];
        CFE_ES_AppRecord_t & appRecord = *CFE_ES_LocateAppRecordByID(CFE_ES_Global.TaskTable[currTask.taskId].AppId);
        CFE_ES_UnlockSharedData(__func__, __LINE__);
        MutexLock locked(currTask.cvMutex);
        if(perfectScheduleMode && (currTask.triggeredMIDs.size() > 0))
        {
            // Do not enter task delay if we have a triggered MID.
            currTask.usRemaining = -1;
        }
        else
        {
            TRICKCFS_DEBUG_OUT(printf("TaskDelay : App %s entering TASK_DELAY for %d msecs.\n",
                                      appRecord.TaskInfo.MainTaskName,
                                      msIn);)
            currTask.state = TaskIPC::TASK_DELAY;
            currTask.usRemaining = msIn;
            pthread_cond_signal(&currTask.cv);
            locked.unlock();
            locked.lock();
            while(currTask.state != TaskIPC::TASK_CONTINUE && appRecord.AppState == CFE_ES_AppState_RUNNING)
            {
                pthread_cond_wait(&currTask.cv, &currTask.cvMutex);
            }
            currTask.state = TaskIPC::TASK_RUNNING;
            TRICKCFS_DEBUG_OUT(
                printf("TaskDelay : App %s exiting TASK_DELAY of %d msecs.\n", appRecord.TaskInfo.MainTaskName, msIn);)
        }
        locked.unlock();
    }
    else
    {
        struct timespec waittime;
        uint32 ms = msIn;
        int sleepstat;

        waittime.tv_sec = ms / 1000;
        waittime.tv_nsec = (ms % 1000) * 1000000;

        /*
         ** Do not allow signals to interrupt nanosleep until the requested time
         */
        do
        {
            sleepstat = nanosleep(&waittime, &waittime);
        } while(sleepstat == -1 && errno == EINTR);
    }
}

void TrickCFSInterface::updateTaskTimes()
{
    long long cycleTics = exec_get_job_cycle(0x0) * (long long)exec_get_time_tic_value();
    for(size_t ii = 0; ii < OS_MAX_TASKS; ++ii)
    {
        TaskIPC & delay = taskStates[ii];
        MutexLock locked(delay.cvMutex);
        if(delay.state == TaskIPC::TASK_DELAY)
        {
            delay.usRemaining -= cycleTics;
            if(delay.usRemaining <= 0)
            {
                delay.usRemaining = -1;
                delay.state = TaskIPC::TASK_CONTINUE;
                pthread_cond_signal(&delay.cv);
            }
        }
        locked.unlock();
    }
}

void TrickCFSInterface::processExternalDeclarations()
{
    for(size_t ii = 0; ii < declarations.size(); ++ii)
    {
        Instance & inst = declarations[ii];
        cpuaddr symPtr;
        int32_t ret = OS_SymbolLookup(&symPtr, inst.symbolName.c_str());
        if(ret != OS_SUCCESS)
        {
            std::stringstream ss;
            ss << "Unable to add declaration \"" << inst.varDecl
               << "\" to Trick MemoryManager. Unable to resolve symbol name " << inst.symbolName;
            message_publish(MSG_ERROR, ss.str().c_str());
        }
        else
        {
            TMM_declare_ext_var_s(reinterpret_cast<void *>(symPtr), inst.varDecl.c_str());
        }
    }

    declarations.clear();
}

TrickCFSInterface::Instance::Instance(const std::string & symbolNameIn, const std::string & varDeclIn)
    : symbolName(symbolNameIn),
      varDecl(varDeclIn)
{
}

TrickCFSInterface::TaskIPC::TaskIPC()
    : usRemaining(-1),
      taskId(-1),
      state(NONE)
{
    pthread_mutex_init(&cvMutex, 0x0);
    pthread_cond_init(&cv, 0x0);
}

TrickCFSInterface::TaskIPC::~TaskIPC() {}

void TrickCFSInterface::signalMainAppComplete()
{
    if(perfectScheduleMode)
    {
        for(std::multimap<size_t, TaskIPC *>::iterator it = perfectFDMap.begin(), end = perfectFDMap.end(); it != end;
            ++it)
        {
            TaskIPC & currTask = *it->second;
            CFE_ES_AppRecord_t & appRecord = *CFE_ES_LocateAppRecordByID(
                CFE_ES_Global.TaskTable[currTask.taskId].AppId);

            MutexLock locked(currTask.cvMutex);
            int reqTriggers = currTask.triggeredMIDs.size();
            locked.unlock();
            int numTriggers = 0;
            int numCompletes = 0;
            while(numCompletes != reqTriggers)
            {
                locked.lock();
                switch(currTask.state)
                {
                    case TaskIPC::TASK_DELAY:
                        currTask.state = TaskIPC::TASK_CONTINUE;
                        pthread_cond_signal(&currTask.cv);
                        break;
                    case TaskIPC::MSG_RCV:
                        if(numTriggers != reqTriggers)
                        {
                            ++numTriggers;
                            TRICKCFS_DEBUG_OUT(printf("2 : SCH triggered App %s.\n", appRecord.TaskInfo.MainTaskName);)
                            currTask.state = TaskIPC::TASK_CONTINUE;
                            pthread_cond_signal(&currTask.cv);
                        }
                        break;
                    case TaskIPC::TASK_COMPLETE:
                        ++numCompletes;
                        TRICKCFS_DEBUG_OUT(printf("5 : SCH received completion notice from App %s.\n",
                                                  appRecord.TaskInfo.MainTaskName);)
                        currTask.state = TaskIPC::TASK_CONTINUE;
                        pthread_cond_signal(&currTask.cv);
                        break;
                    default:
                        break;
                }
                locked.unlock();

                CFE_ES_LockSharedData(__func__, __LINE__);
                if(appRecord.AppState != CFE_ES_AppState_RUNNING)
                {
                    numCompletes = reqTriggers;
                    currTask.state = TaskIPC::TASK_COMPLETE;
                    currTask.triggeredMIDs.clear();
                }
                CFE_ES_UnlockSharedData(__func__, __LINE__);
            }
        }
        perfectFDMap.clear();
    }

#if __linux
    uint64_t value = 1;
    write(isMainCompleteTrigger, &value, sizeof(uint64_t));
#else
    MutexLock triggerLocked(triggerMutex);
    pthread_cond_signal(&isMainCompleteTrigger);
    triggerLocked.unlock();
#endif
}

void TrickCFSInterface::CFSWaitForTrigger()
{
#if __linux
    uint64_t value;
    read(triggerMain, &value, sizeof(uint64_t));
#else
    pthread_cond_wait(&triggerMain, &triggerMutex);
#endif
}

void TrickCFSInterface::triggerMainApp()
{
#if __linux
    uint64_t value = 1;
    write(triggerMain, &value, sizeof(uint64_t));
#else
    MutexLock triggerLocked(triggerMutex);
    pthread_cond_signal(&triggerMain);
    triggerLocked.unlock();
#endif
}

void TrickCFSInterface::markPipeAsTriggered(size_t pipeId, size_t mid)
{
    CFE_ES_LockSharedData(__func__, __LINE__);
    uint32 taskId = prevPipeTaskOwner[pipeId];
    TaskIPC & currTask = taskStates[taskId];
    CFE_ES_AppRecord_t & appRecord = *CFE_ES_LocateAppRecordByID(CFE_ES_Global.TaskTable[currTask.taskId].AppId);
    CFE_ES_UnlockSharedData(__func__, __LINE__);

    MutexLock locked(currTask.cvMutex);
    if(appRecord.AppState == CFE_ES_AppState_RUNNING)
    {
        currTask.triggeredMIDs.push_back(mid);
        TRICKCFS_DEBUG_OUT(printf("SCH sent MID 0x%04X to App %s at time = %f\n",
                                  mid,
                                  appRecord.TaskInfo.MainTaskName,
                                  exec_get_sim_time());)
        if(perfectScheduleMode)
        {
            perfectFDMap.insert(std::make_pair(appRecord.StartParams.MainTaskInfo.Priority, &currTask));
        }
    }
    locked.unlock();
}

void TrickCFSInterface::acknowledgePipeTrigger(size_t taskId, size_t mid)
{
    CFE_ES_LockSharedData(__func__, __LINE__);
    TaskIPC & currTask = taskStates[taskId];
    CFE_ES_AppRecord_t & appRecord = *CFE_ES_LocateAppRecordByID(CFE_ES_Global.TaskTable[currTask.taskId].AppId);
    CFE_ES_UnlockSharedData(__func__, __LINE__);

    MutexLock locked(currTask.cvMutex);
    std::list<size_t>::iterator it = std::find(currTask.triggeredMIDs.begin(), currTask.triggeredMIDs.end(), mid);
    std::list<size_t>::iterator end = currTask.triggeredMIDs.end();
    if(it != end)
    {
        TaskIPC::StackInfo * newStack = new TaskIPC::StackInfo;
        newStack->cacheBackTraceSize = backtrace(newStack->cacheBackTrace, TaskIPC::StackInfo::SIZE_TRACE);
        currTask.stacks.push_back(newStack);
        currTask.acknowledgedMIDs.push_back(mid);

        if(perfectScheduleMode)
        {
            TRICKCFS_DEBUG_OUT(printf("1 : App %s waiting for permission to proceed on MID 0x%04X.\n",
                                      appRecord.TaskInfo.MainTaskName,
                                      mid);)
            currTask.state = TaskIPC::MSG_RCV;
            pthread_cond_signal(&currTask.cv);
            locked.unlock();
            locked.lock();
            while(currTask.state != TaskIPC::TASK_CONTINUE && appRecord.AppState == CFE_ES_AppState_RUNNING)
            {
                pthread_cond_wait(&currTask.cv, &currTask.cvMutex);
            }
            currTask.state = TaskIPC::TASK_RUNNING;
            pthread_cond_signal(&currTask.cv);
            TRICKCFS_DEBUG_OUT(
                printf("3 : App %s started processing MID 0x%04X.\n", appRecord.TaskInfo.MainTaskName, mid);)
        }
        else
        {
            TRICKCFS_DEBUG_OUT(
                printf("3-nonperfect : App %s started processing MID 0x%04X.\n", appRecord.TaskInfo.MainTaskName, mid);)
            currTask.state = TaskIPC::TASK_RUNNING;
            pthread_cond_signal(&currTask.cv);
        }
    }
    else
    {
        TRICKCFS_DEBUG_OUT(
            printf("3-NoMIDs : App %s started processing MID 0x%04X.\n", appRecord.TaskInfo.MainTaskName, mid);)
        currTask.state = TaskIPC::TASK_RUNNING;
        pthread_cond_signal(&currTask.cv);
    }
    locked.unlock();
}

void TrickCFSInterface::markPipeAsComplete(size_t pipeId, size_t taskId)
{
    CFE_ES_LockSharedData(__func__, __LINE__);
    TaskIPC & currTask = taskStates[taskId];
    CFE_ES_AppRecord_t & appRecord = *CFE_ES_LocateAppRecordByID(CFE_ES_Global.TaskTable[currTask.taskId].AppId);
    setPipeOwner(pipeId, taskId);
    CFE_ES_UnlockSharedData(__func__, __LINE__);

    MutexLock locked(currTask.cvMutex);
    if(currTask.stacks.size() > 0)
    {
        std::list<size_t>::iterator it = std::find(currTask.triggeredMIDs.begin(),
                                                   currTask.triggeredMIDs.end(),
                                                   currTask.acknowledgedMIDs.back());
        if(it != currTask.triggeredMIDs.end())
        {
            currTask.triggeredMIDs.erase(it);
            currTask.acknowledgedMIDs.pop_back();
        }
        TaskIPC::StackInfo currStack;
        size_t prevSize = currTask.stacks.size();
        currStack.cacheBackTraceSize = backtrace(currStack.cacheBackTrace, TaskIPC::StackInfo::SIZE_TRACE);
        for(std::list<TaskIPC::StackInfo *>::iterator it = currTask.stacks.begin(), end = currTask.stacks.end();
            it != end;
            ++it)
        {
            TaskIPC::StackInfo * stack = *it;
            bool foundEntry = true;
            if(currStack.cacheBackTraceSize == stack->cacheBackTraceSize)
            {
                for(int ii = 3; ii < currStack.cacheBackTraceSize; ++ii)
                {
                    if(currStack.cacheBackTrace[ii] != stack->cacheBackTrace[ii])
                    {
                        foundEntry = false;
                        break;
                    }
                }
            }
            else
            {
                foundEntry = false;
            }
            if(foundEntry)
            {
                delete stack;
                currTask.stacks.erase(it);
                break;
            }
        }

        if(currTask.stacks.size() != prevSize)
        {
            if(perfectScheduleMode)
            {
                TRICKCFS_DEBUG_OUT(
                    printf("4 : App %s waiting for ack of completion.\n", appRecord.TaskInfo.MainTaskName);)

                currTask.state = TaskIPC::TASK_COMPLETE;
                pthread_cond_signal(&currTask.cv);
                locked.unlock();
                locked.lock();
                while(currTask.state != TaskIPC::TASK_CONTINUE && appRecord.AppState == CFE_ES_AppState_RUNNING)
                {
                    pthread_cond_wait(&currTask.cv, &currTask.cvMutex);
                }
                TRICKCFS_DEBUG_OUT(printf("6 : App %s ack completed.\n", appRecord.TaskInfo.MainTaskName);)
                currTask.state = TaskIPC::TASK_RUNNING;
                pthread_cond_signal(&currTask.cv);
            }
            else
            {
                TRICKCFS_DEBUG_OUT(printf("6-nonperfect : App %s ack completed.\n", appRecord.TaskInfo.MainTaskName);)
                currTask.state = TaskIPC::TASK_COMPLETE;
                pthread_cond_signal(&currTask.cv);
            }
        }
        else
        {
            TRICKCFS_DEBUG_OUT(printf("6-NoStackChange : App %s ack completed.\n", appRecord.TaskInfo.MainTaskName);)
            currTask.state = TaskIPC::TASK_RUNNING;
            pthread_cond_signal(&currTask.cv);
        }
    }
    else
    {
        TRICKCFS_DEBUG_OUT(printf("6-NoStacks : App %s ack completed.\n", appRecord.TaskInfo.MainTaskName);)
        currTask.state = TaskIPC::TASK_RUNNING;
        pthread_cond_signal(&currTask.cv);
    }
    locked.unlock();
}

void TrickCFSInterface::addCFSCPUAffinity(int cpuIdxIn)
{
    cpuAffinityIdxs.push_back(cpuIdxIn);
}

void TrickCFSInterface::setAffinityAttribute(pthread_attr_t & attrIn)
{
    if(!cpuAffinityIdxs.empty() && !perfectScheduleMode)
    {
#if __linux
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        for(size_t ii = 0; ii < cpuAffinityIdxs.size(); ++ii)
        {
            CPU_SET(cpuAffinityIdxs[ii], &cpus);
        }
        int ret = pthread_attr_setaffinity_np(&attrIn, sizeof(cpu_set_t), &cpus);
#else
        int ret = ENOSYS;
#endif
        if(ret != 0)
        {
            std::stringstream ss;
            ss << "Error in " << __FILE__ << ":" << __LINE__;
            ss << ", Error calling pthread_attr_setaffinity_np, reported \"";
            ss << strerror(ret) << "\"\n";
            message_publish(MSG_ERROR, ss.str().c_str());
        }
    }
}

int TrickCFSInterface::getOutputFd()
{
    if(outputFilePtr == 0x0)
    {
        return -1;
    }
    return fileno(outputFilePtr);
}

void TrickCFSInterface::updateCFS()
{
    updateTaskTimes();
    triggerMainApp();
    TrickWaitForCFSComplete();
}

void TrickCFSInterface::setPipeOwner(size_t pipeId, size_t taskId)
{
    prevPipeTaskOwner[pipeId] = taskId;
}

void TrickCFSInterface::calculateAbsTimeFromDelta(int msecs, struct timespec * tm)
{
    clock_gettime(CLOCK_REALTIME, tm);

    /* add the delay to the current time */
    tm->tv_sec += (time_t)(msecs / 1000);
    /* convert residue ( msecs )  to nanoseconds */
    tm->tv_nsec += (msecs % 1000) * 1000000L;

    if(tm->tv_nsec >= 1000000000L)
    {
        tm->tv_nsec -= 1000000000L;
        tm->tv_sec++;
    }
}

void TrickCFSInterface::TrickWaitForCFSComplete()
{
#if __linux
    uint64_t value = 1;
    read(isMainCompleteTrigger, &value, sizeof(uint64_t));
#else
    pthread_cond_wait(&isMainCompleteTrigger, &triggerMutex);
#endif

    if(!perfectScheduleMode)
    {
        for(size_t ii = 0; ii < CFE_ES_Global.RegisteredTasks; ++ii)
        {
            CFE_ES_LockSharedData(__func__, __LINE__);
            TaskIPC * currTask = &taskStates[ii];
            CFE_ES_UnlockSharedData(__func__, __LINE__);

            if(exec_get_exec_command() == ExitCmd)
            {
                break;
            }

            MutexLock locked(currTask->cvMutex);
            if(currTask->state == TaskIPC::TASK_DELAY)
            {
                locked.unlock();
            }
            else
            {
                if((currTask->triggeredMIDs.size() == 0) && (currTask->stacks.size() == 0))
                {
                    locked.unlock();
                }
                else
                {
                    int remainingTime = msWaitTimeForCFSApps;
                    if(msWaitTimeForCFSApps == -1)
                    {
                        remainingTime = 5000;
                    }
                    // Handle the wait forever case
                    while(remainingTime > 0)
                    {
                        int pollTime = 5000;
                        if(remainingTime < pollTime)
                        {
                            pollTime = remainingTime;
                        }
                        struct timespec ts;
                        calculateAbsTimeFromDelta(pollTime, &ts);
                        // Condition mutex is already locked
                        int ret = pthread_cond_timedwait(&currTask->cv, &currTask->cvMutex, &ts);
                        if(ret == ETIMEDOUT)
                        {
                            if(exec_get_exec_command() == ExitCmd)
                            {
                                break;
                            }
                            if(msWaitTimeForCFSApps == -1)
                            {
                                remainingTime += pollTime;
                            }
                            remainingTime -= pollTime;
                            if(remainingTime <= 0)
                            {
                                std::stringstream ss;
                                if(currTask->triggeredMIDs.empty())
                                {
                                    if(currTask->stacks.empty())
                                    {
                                        ss << "Timed out waiting for Task \"" << CFE_ES_Global.TaskTable[ii].TaskName;
                                        ss << "\" to complete for unknown reason.\n";
                                    }
                                    else
                                    {
                                        ss << "Timed out waiting for Task \"" << CFE_ES_Global.TaskTable[ii].TaskName;
                                        ss << "\" to return to Schedule pipe Recv function call.\n";
                                    }
                                }
                                else
                                {
                                    ss << "Timed out waiting for Task \"" << CFE_ES_Global.TaskTable[ii].TaskName
                                       << "\" for MID(s) = 0x" << std::uppercase << std::setfill('0') << std::setw(8)
                                       << std::hex;
                                    ss << currTask->triggeredMIDs.front()
                                       << " to complete. This can be an indicator of a poor schedule table.\n";
                                }
                                message_publish(MSG_ERROR, ss.str().c_str());
                                currTask->triggeredMIDs.clear();
                            }
                            else
                            {
                                std::stringstream ss;

                                if(currTask->triggeredMIDs.empty())
                                {
                                    ss << "Waiting for Task \"" << CFE_ES_Global.TaskTable[ii].TaskName;
                                    ss << "\" to complete for unknown reason.\n";
                                }
                                else
                                {
                                    ss << "Waiting for Task \"" << CFE_ES_Global.TaskTable[ii].TaskName
                                       << "\" for MID(s) = 0x" << std::uppercase << std::setfill('0') << std::setw(8)
                                       << std::hex << currTask->triggeredMIDs.front() << " to complete....\n";
                                }
                                message_publish(MSG_WARNING, ss.str().c_str());
                            }
                        }
                        else
                        {
                            // Decrement index in order to retest this app.
                            --ii;
                            locked.unlock();
                            break;
                        }
                    }
                }
            }
        }
    }
}
