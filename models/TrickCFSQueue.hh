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
 (Instance of a TrickCFS Message Queue.)

 LIBRARY DEPENDENCIES:
 (
 (TrickCFSQueue.o)
 )

 *******************************************************************************/

#ifndef TRICKCFSQUEUE_HH_
#define TRICKCFSQUEUE_HH_

#include <pthread.h>
#include <queue>
#include <stdio.h>
#include <string>

class TrickCFSQueue
{
public:
    TrickCFSQueue();
    virtual ~TrickCFSQueue();

    void setId(int idx)
    {
        myId = idx;
    }

    size_t getNumMessages();

    size_t getMaxMessages();

    int open(const std::string & nameIn, size_t depthIn, size_t maxSizeIn);

    int close();

    ssize_t receive(char * msgPtr, size_t buffSize, struct timespec * ts);

    int send(const char * msgPtr, size_t msgLen);

protected:
    // std::queue<char *> data;
    std::string name;
    size_t maxMsgs;
    size_t maxSize;
    int myId;
    bool isEnabled;

    pthread_cond_t cv;       /* ** Do not process */
    pthread_mutex_t cvMutex; /* ** Do not process */

    struct Message
    {
        char * data;
        size_t size;
        bool hasData;
    };

    Message * messages;
    size_t ringBufferIdx;
    size_t numMessages;
};

#endif /* TRICKCFSQUEUE_HH_ */
