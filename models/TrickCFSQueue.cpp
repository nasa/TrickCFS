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
 * TrickMQueue.cpp
 *
 *  Created on: May 23, 2018
 *      Author: tbrain
 */

#include "TrickCFSQueue.hh"

#include "osapi-error.h"
#include "sim_services/Message/include/message_proto.h"
#include <sstream>
#include <stdlib.h>
#include <string.h>

namespace
{
class MutexLock
{
public:
    MutexLock(pthread_mutex_t & mutexIn)
        : myMutex(mutexIn)
    {
        pthread_mutex_lock(&myMutex);
    }

    MutexLock(const pthread_mutex_t & mutexIn)
        : myMutex(const_cast<pthread_mutex_t &>(mutexIn))
    {
        pthread_mutex_lock(&myMutex);
    }

    virtual ~MutexLock()
    {
        pthread_mutex_unlock(&myMutex);
    }

    pthread_mutex_t & myMutex;
};
} // namespace

TrickCFSQueue::TrickCFSQueue()
    : maxMsgs(0),
      maxSize(0),
      myId(-1),
      isEnabled(false),
      messages(0x0),
      ringBufferIdx(0),
      numMessages(0)
{
    pthread_mutex_init(&cvMutex, 0x0);
    pthread_cond_init(&cv, 0x0);
}

TrickCFSQueue::~TrickCFSQueue()
{
    // Intentionally blank
}

int TrickCFSQueue::open(const std::string & nameIn, size_t depthIn, size_t maxSizeIn)
{
    MutexLock lock(cvMutex);
    if(!isEnabled)
    {
        isEnabled = true;

        messages = (Message *)calloc(depthIn, sizeof(Message));
        for(size_t ii = 0; ii < depthIn; ++ii)
        {
            messages[ii].data = (char *)calloc(1, maxSizeIn);
        }

        name = nameIn;
        maxMsgs = depthIn;
        maxSize = maxSizeIn;
        ringBufferIdx = 0;
        numMessages = 0;
    }
    return myId;
}

int TrickCFSQueue::close()
{
    int ret = 0;
    MutexLock lock(cvMutex);
    if(!isEnabled)
    {
        ret = -1;
    }
    else
    {
        if(messages != 0x0)
        {
            for(size_t ii = 0; ii < maxMsgs; ++ii)
            {
                free(messages[ii].data);
            }
            messages = 0x0;
        }
        isEnabled = false;
        ret = 0;
    }
    return ret;
}

ssize_t TrickCFSQueue::receive(char * msgPtr, size_t buffSize, struct timespec * ts)
{
    if(!isEnabled)
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__;
        ss << ", Attempted to receive message on an uninitialized pipe. Pipe ID " << myId << "\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return OS_QUEUE_ID_ERROR;
    }
    else
    {
        if(buffSize >= maxSize)
        {
            ssize_t retVal;
            MutexLock lock(cvMutex);
            while(numMessages == 0)
            {
                if(ts == 0x0)
                {
                    pthread_cond_wait(&cv, &cvMutex);
                }
                else if(pthread_cond_timedwait(&cv, &cvMutex, ts) == ETIMEDOUT)
                {
                    return OS_QUEUE_TIMEOUT;
                }
            }
            Message & nextMsg = messages[ringBufferIdx];
            if(nextMsg.hasData)
            {
                memcpy(msgPtr, nextMsg.data, nextMsg.size);
                retVal = nextMsg.size;
                ringBufferIdx++;
                nextMsg.hasData = false;
                --numMessages;
            }
            else
            {
                message_publish(MSG_ERROR,
                                "Error in %s:%d, Improper queue state for pipe \"%s\"\n",
                                __FILE__,
                                __LINE__,
                                name.c_str());
                retVal = OS_ERROR;
            }
            if(ringBufferIdx >= maxMsgs)
            {
                ringBufferIdx = 0;
            }
            return retVal;
        }
        else
        {
            message_publish(MSG_ERROR,
                            "Error in %s:%d, Buffer size is too small for pipe \"%s\"\n",
                            __FILE__,
                            __LINE__,
                            name.c_str());
            return OS_QUEUE_INVALID_SIZE;
        }
    }
}

int TrickCFSQueue::send(const char * msgPtr, size_t msgLen)
{
    if(!isEnabled)
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__;
        ss << ", Attempted to send message on an uninitialized pipe. Pipe ID " << myId << "\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return OS_ERROR;
    }
    else
    {
        if(msgLen <= maxSize)
        {
            int retVal;
            MutexLock lock(cvMutex);
            if((numMessages + 1) <= maxMsgs)
            {
                size_t testIdx = ringBufferIdx + numMessages;
                ++numMessages;
                if(testIdx >= maxMsgs)
                {
                    testIdx -= maxMsgs;
                }
                Message & nextMsg = messages[testIdx];
                memcpy(nextMsg.data, msgPtr, msgLen);
                nextMsg.size = msgLen;
                nextMsg.hasData = true;
                retVal = 0;
                pthread_cond_signal(&cv);
            }
            else
            {
                // Message Queue is FULL. Drop this message.
                retVal = OS_QUEUE_FULL;
            }
            return retVal;
        }
        else
        {
            return OS_ERROR;
        }
    }
}

size_t TrickCFSQueue::getNumMessages()
{
    if(!isEnabled)
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__;
        ss << ", Attempted to query the number of messages on an uninitialized pipe. Pipe ID " << myId << "\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return 0;
    }
    else
    {
        size_t retVal;
        MutexLock lock(cvMutex);
        retVal = numMessages;
        return retVal;
    }
}

size_t TrickCFSQueue::getMaxMessages()
{
    if(!isEnabled)
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__;
        ss << ", Attempted to query the number of messages on an uninitialized pipe. Pipe ID " << myId << "\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return 0;
    }
    else
    {
        size_t retVal;
        MutexLock lock(cvMutex);
        retVal = maxMsgs;
        return retVal;
    }
}
