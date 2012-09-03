/*
 -------------------------------------------------------------------------------
 This file is part of the Plink, Plonk, Plank libraries
  by Martin Robinson
 
 http://code.google.com/p/pl-nk/
 
 Copyright University of the West of England, Bristol 2011-12
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of University of the West of England, Bristol nor 
   the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF THE WEST OF ENGLAND, BRISTOL BE 
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 
 This software makes use of third party libraries. For more information see:
 doc/license.txt included in the distribution.
 -------------------------------------------------------------------------------
 */

#include "plank_StandardHeader.h"
#include "plank_Thread.h"
#include "../maths/plank_Maths.h"

#define PLANK_THREAD_PAUSEQUANTA 0.00001

typedef PlankThreadNativeReturn (PLANK_THREADCALL *PlankThreadNativeFunction)(PlankP);
PlankThreadNativeReturn PLANK_THREADCALL pl_ThreadNativeFunction (PlankP argument);

void pl_Thread_Reset (PlankThreadRef p)
{
    pl_AtomicI_Set (&p->shouldExitAtom, PLANK_FALSE);
    p->thread = (PlankThreadNativeHandle)0;
    p->threadID = (PlankThreadID)0;
    pl_AtomicI_Set (&p->isRunningAtom, PLANK_FALSE);    
}

PlankResult pl_ThreadSleep (PlankD seconds)
{
#if PLANK_APPLE
    useconds_t useconds;
    pl_AtomicMemoryBarrier();
    useconds = (useconds_t)pl_MaxD (seconds * 1000000.0, 0.0);
    
    if (useconds > 0)
        usleep (useconds);
    else
        sched_yield();

#elif PLANK_WIN    
    Sleep ((int)pl_MaxD (seconds * 1000.0, 0.0));
#endif
    
    return PlankResult_OK;
}

PlankResult pl_ThreadYield()
{
    pl_ThreadSleep (0.0);
    return PlankResult_OK;
}

PlankThreadID pl_ThreadCurrentID()
{
#if PLANK_APPLE
    pthread_t native = pthread_self();
    return (PlankThreadID)native;
#elif PLANK_WIN
    return (PlankThreadID)GetCurrentThreadId();
#endif
}

#if PLANK_WIN
struct THREADNAME_INFO
{
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#endif

static PlankResult pl_ThreadSetNameInternal (const char* name)
{
#if PLANK_APPLE
    pthread_setname_np (name);
    return PlankResult_OK;
#elif PLANK_WIN
    
    (void)name;
//    not tested
//    THREADNAME_INFO info;
//    info.dwType = 0x1000;
//    info.szName = name;
//    info.dwThreadID = pl_Thread_GetID (p);
//    info.dwFlags = 0;
//    
//    __try
//    {
//        RaiseException (0x406D1388, 0, sizeof (info) / sizeof (DWORD), (DWORD*)&info);
//    }
//    except(EXCEPTION_CONTINUE_EXECUTION)
//    {
//    }    
    
    return PlankResult_OK;
    
#else
    (void)name;
    return PlankResult_OK;
#endif
}

PlankThreadNativeReturn PLANK_THREADCALL pl_ThreadNativeFunction (PlankP argument)
{
    PlankResult result;
    PlankThreadRef p;
    
    p = (PlankThreadRef)argument;
    
    if (p->name[0] != '\0')
    {
        result = pl_ThreadSetNameInternal (p->name);
        
        if (result != PlankResult_OK)
            goto exit;
    }
    
    if (p->function == 0)
        result = PlankResult_ThreadFunctionInvalid;
    else
        result = (*p->function) (p);
  
exit:
    pl_Thread_Reset (p);
    return (result == PlankResult_OK) ? 0 : (PlankThreadNativeReturn)(-1);
}

PlankThreadRef pl_Thread_CreateAndInit()
{
    PlankThreadRef p;
    p = pl_Thread_Create();
    
    if (p != PLANK_NULL)
    {
        if (pl_Thread_Init (p) != PlankResult_OK)
            pl_Thread_Destroy (p);
        else
            return p;
    }
    
    return PLANK_NULL;
}

PlankThreadRef pl_Thread_Create()
{
    PlankMemoryRef m;
    PlankThreadRef p;
    
    m = pl_MemoryGlobal();
    p = (PlankThreadRef)pl_Memory_AllocateBytes (m, sizeof (PlankThread));
    
    if (p != PLANK_NULL)
        pl_MemoryZero (p, sizeof (PlankThread));
    
    return p;
}

PlankResult pl_Thread_Init (PlankThreadRef p)
{
    PlankResult result = PlankResult_OK;
    
    p->function = (PlankThreadFunction)0;
    p->name[0] = '\0';
    
    pl_AtomicI_Init (&p->shouldExitAtom);
    pl_AtomicI_Init (&p->isRunningAtom);
    pl_AtomicI_Init (&p->paused);
    pl_AtomicPX_Init (&p->userDataAtom);
        
    pl_Thread_Reset (p);
    
    return result;
}

PlankResult pl_Thread_DeInit (PlankThreadRef p)
{
    PlankResult result;        
    result = PlankResult_OK;    

    pl_AtomicI_DeInit (&p->shouldExitAtom);
    pl_AtomicI_DeInit (&p->isRunningAtom);
    pl_AtomicI_DeInit (&p->paused);
    pl_AtomicPX_DeInit (&p->userDataAtom);

    return result;    
}

PlankResult pl_Thread_Destroy (PlankThreadRef p)
{
    PlankResult result;    
    PlankMemoryRef m;
    
    result = PlankResult_OK;    
    m = pl_MemoryGlobal();

    if (p == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    if ((result = pl_Thread_DeInit (p)) != PlankResult_OK)
        goto exit;
    
    result = pl_Memory_Free (m, p);
    
exit:
    return result;    
}

PlankThreadID pl_Thread_GetID (PlankThreadRef p)
{
    return p->threadID;
}

PlankResult pl_Thread_SetName (PlankThreadRef p, const char* name)
{    
    if (pl_AtomicI_Get (&p->isRunningAtom))
        return PlankResult_ThreadSetFunctionFailed;

    strncpy (p->name, name, PLANK_THREAD_MAXNAMELENGTH);
    
    return PlankResult_OK;
}

PlankResult pl_Thread_SetFunction (PlankThreadRef p, PlankThreadFunction function)
{
    if (pl_AtomicI_Get (&p->isRunningAtom))
        return PlankResult_ThreadSetFunctionFailed;
    
    p->function = function;
    return PlankResult_OK;
}

PlankResult pl_Thread_SetUserData (PlankThreadRef p, PlankP userData)
{
    if (pl_AtomicI_Get (&p->isRunningAtom))
        return PlankResult_ThreadSetUserDataFailed;

    pl_AtomicPX_Set (&p->userDataAtom, userData);
    return PlankResult_OK;
}

PlankP pl_Thread_GetUserData (PlankThreadRef p)
{
    return pl_AtomicPX_Get (&p->userDataAtom);
}

PlankResult pl_Thread_Start (PlankThreadRef p)
{
    if (pl_AtomicI_Get (&p->isRunningAtom))
        return PlankResult_ThreadAlreadyRunning;
    
    if (p->function == PLANK_NULL)
        return PlankResult_ThreadFunctionInvalid;
    
#if PLANK_APPLE
    if (pthread_create (&p->thread, NULL, pl_ThreadNativeFunction, p) != 0)
        return PlankResult_ThreadStartFailed;
    
    p->threadID = (PlankThreadID)p->thread;
    
#elif PLANK_WIN    
    if ((p->thread = _beginthreadex (NULL, 0, pl_ThreadNativeFunction, p, 0, (unsigned int*)&p->threadID)) == 0) 
        return PlankResult_ThreadStartFailed;

#endif    
    pl_AtomicI_Set (&p->isRunningAtom, PLANK_TRUE);
    
    return PlankResult_OK;
}

PlankResult pl_Thread_Cancel (PlankThreadRef p)
{
    if (! p->thread)
        return PlankResult_ThreadInvalid;

#if PLANK_APPLE
    if (pthread_cancel (p->thread) == 0)
    {
        pl_Thread_Reset (p);
        return PlankResult_OK;
    }
    
	return PlankResult_ThreadCancelFailed;

#elif PLANK_WIN   
    TerminateThread ((HANDLE)p->thread, 0);
    return PlankResult_OK;
#endif
}

PlankResult pl_Thread_Wait (PlankThreadRef p)
{
    if (! p->thread)
        return PlankResult_ThreadInvalid;
    
#if PLANK_APPLE
    if (pthread_join (p->thread, NULL) == 0) 
    {
        pl_Thread_Reset (p);
        return PlankResult_OK;
    }
    
#elif PLANK_WIN    
    if (WaitForSingleObject ((HANDLE)p->thread, INFINITE) == WAIT_OBJECT_0) 
    {
        CloseHandle ((HANDLE)p->thread);
        pl_Thread_Reset (p);
        return PlankResult_OK;
    }
    
#endif
    return PlankResult_ThreadWaitFailed;    
}

PlankResult pl_Thread_Pause (PlankThreadRef p)
{    
    return pl_Thread_PauseWithTimeout (p, PLANK_INFINITY);
}

PlankResult pl_Thread_PauseWithTimeout (PlankThreadRef p, double duration)
{
    if (pl_ThreadCurrentID() != pl_Thread_GetID (p)) // a thread can only pause itself
        return PlankResult_ThreadInvalid;
    
    pl_AtomicI_Set (&p->paused, PLANK_TRUE);
    
    double time = 0.0;
    
    while (pl_AtomicI_GetUnchecked (&p->paused) && (time < duration))
    {
        pl_ThreadSleep (PLANK_THREAD_PAUSEQUANTA);
        time = time + PLANK_THREAD_PAUSEQUANTA;
    }
        
    pl_AtomicI_Set (&p->paused, PLANK_FALSE);

    return PlankResult_OK;
}

PlankResult pl_Thread_Resume (PlankThreadRef p)
{
    if (pl_ThreadCurrentID() == pl_Thread_GetID (p)) // a thread can't resume itself
        return PlankResult_ThreadInvalid;
    
    pl_AtomicI_Set (&p->paused, PLANK_FALSE);
    return PlankResult_OK;
}

PlankB pl_Thread_IsRunning (PlankThreadRef p)
{
    return pl_AtomicI_Get (&p->isRunningAtom) != 0;
}

PlankResult pl_Thread_SetShouldExit (PlankThreadRef p)
{
    if (pl_AtomicI_Get (&p->shouldExitAtom) != 0)
        return PlankResult_ThreadShouldExitAlreadySet;
        
    pl_AtomicI_Set (&p->shouldExitAtom, PLANK_TRUE);
    return PlankResult_OK;
}

PlankB pl_Thread_GetShouldExit (PlankThreadRef p)
{
    return pl_AtomicI_Get (&p->shouldExitAtom) != 0;
}

PlankResult pl_Thread_SetPriority (PlankThreadRef p, int priority)
{
#if PLANK_APPLE
    struct sched_param param;
    int policy, minPriority, maxPriority;
    priority = pl_ClipI (priority, 0, 10);
                
    if (pthread_getschedparam (p->thread, &policy, &param) != 0)
        return PlankResult_ThreadSetPriorityFailed;
    
    policy = (priority == 0) ? SCHED_OTHER : SCHED_RR;
    minPriority = sched_get_priority_min (policy);
    maxPriority = sched_get_priority_max (policy);
    
    param.sched_priority = ((maxPriority - minPriority) * priority) / 10 + minPriority;
    return pthread_setschedparam (p->thread, policy, &param) == 0 
           ? PlankResult_OK 
           : PlankResult_ThreadSetPriorityFailed;
#else
    (void)p;
    (void)priority;
    return PlankResult_ThreadSetPriorityFailed;
#endif
}

