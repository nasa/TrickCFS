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
 * TrickMQueueManager.cpp
 *
 *  Created on: May 23, 2018
 *      Author: tbrain
 */

#include "TrickCFSQueueManager.hh"
#include <sstream>

#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Message/include/message_proto.h"

TrickCFSQueueManager * the_trickCFSQueueManager = 0x0;

TrickCFSQueueManager::TrickCFSQueueManager()
{
    for(size_t ii = 0; ii < OS_MAX_QUEUES; ++ii)
    {
        queues[ii].setId(ii);
    }

    if(the_trickCFSQueueManager == 0x0)
    {
        the_trickCFSQueueManager = this;
    }
    else
    {
        std::string errStr("Multiple instances of ");
        errStr += __PRETTY_FUNCTION__;
        exec_terminate_with_return(-1, __FILE__, __LINE__, errStr.c_str());
    }
}

TrickCFSQueueManager::~TrickCFSQueueManager()
{
    // Intentionally blank
}

int TrickCFSQueueManager::open(const std::string & nameIn, size_t idx, size_t depthIn, size_t maxSizeIn)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].open(nameIn, depthIn, maxSizeIn);
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}

int TrickCFSQueueManager::close(size_t idx)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].close();
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}

ssize_t TrickCFSQueueManager::receive(size_t idx, char * msgPtr, size_t buffSize, struct timespec * ts)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].receive(msgPtr, buffSize, ts);
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}

size_t TrickCFSQueueManager::getNumMessages(size_t idx)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].getNumMessages();
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}

size_t TrickCFSQueueManager::getMaxMessages(size_t idx)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].getMaxMessages();
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}

int TrickCFSQueueManager::send(int idx, const char * msgPtr, size_t msgLen)
{
    if(idx < OS_MAX_QUEUES)
    {
        return queues[idx].send(msgPtr, msgLen);
    }
    else
    {
        std::stringstream ss;
        ss << "Error in " << __FILE__ << ":" << __LINE__ << ", Requested index is larger than OS_MAX_QUEUES\n";
        message_publish(MSG_ERROR, ss.str().c_str());
        return -1;
    }
}
