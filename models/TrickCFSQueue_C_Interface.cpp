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
 * TrickCFSQueue_C_Interface.cpp
 *
 *  Created on: May 24, 2018
 *      Author: tbrain
 */

#include "TrickCFSQueueManager.hh"
#include "TrickCFSQueue_C_proto.h"

extern TrickCFSQueueManager * the_trickCFSQueueManager;

extern "C"
{
    mqd_t TrickCFSQueue_open(const char * nameIn, size_t idx, size_t depthIn, size_t maxSizeIn)
    {
        return the_trickCFSQueueManager->open(nameIn, idx, depthIn, maxSizeIn);
    }

    int TrickCFSQueue_close(size_t idx)
    {
        return the_trickCFSQueueManager->close(idx);
    }

    size_t TrickCFSQueue_getNumMessages(size_t idx)
    {
        return the_trickCFSQueueManager->getNumMessages(idx);
    }

    size_t TrickCFSQueue_getMaxMessages(size_t idx)
    {
        return the_trickCFSQueueManager->getMaxMessages(idx);
    }

    ssize_t TrickCFSQueue_receive(size_t idx, char * msgPtr, size_t buffSize)
    {
        return the_trickCFSQueueManager->receive(idx, msgPtr, buffSize, 0x0);
    }

    ssize_t TrickCFSQueue_timedreceive(size_t idx, char * msgPtr, size_t buffSize, struct timespec * ts)
    {
        return the_trickCFSQueueManager->receive(idx, msgPtr, buffSize, ts);
    }

    int TrickCFSQueue_send(int idx, const char * msgPtr, size_t msgLen)
    {
        return the_trickCFSQueueManager->send(idx, msgPtr, msgLen);
    }
}
