/*
 -------------------------------------------------------------------------------
 This file is part of the Plink, Plonk, Plank libraries
  by Martin Robinson
 
 http://code.google.com/p/pl-nk/
 
 Copyright University of the West of England, Bristol 2011-13
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

#include "../core/plank_StandardHeader.h"
#include "plank_MultiFileReader.h"
#include "../maths/plank_Maths.h"
#include "../random/plank_RNG.h"


PlankResult pl_MultiFileReaderArraySequenceOnceNextFunction (PlankMulitFileReaderRef p);
PlankResult pl_MultiFileReaderArraySequenceLoopNextFunction (PlankMulitFileReaderRef p);
PlankResult pl_MultiFileReaderArrayRandomNextFunction (PlankMulitFileReaderRef p);
PlankResult pl_MultiFileReaderArrayRandomNoRepeatNextFunction (PlankMulitFileReaderRef p);
PlankResult pl_MultiFileReaderQueueNextFunction (PlankMulitFileReaderRef p);


//..

PlankResult pl_MultiFileReaderArraySequenceOnceNextFunction (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankDynamicArrayRef array;
    PlankFile* fileArray;
    PlankFileRef file;
    int arraySize;
    
    file = (PlankFileRef)PLANK_NULL;
    array = (PlankDynamicArrayRef)p->source;
    arraySize = pl_DynamicArray_GetSize (array);
    
    p->index = p->index + 1;
    
    if ((p->index >= 0) && (p->index < arraySize))
    {
        fileArray = (PlankFile*)pl_DynamicArray_GetArray (array);
        file = fileArray + p->index;
    }
    
    p->currentFile = file;
    
exit:
    return result;
}

PlankResult pl_MultiFileReaderArraySequenceLoopNextFunction (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankDynamicArrayRef array;
    PlankFile* fileArray;
    PlankFileRef file;
    int arraySize;
    
    file = (PlankFileRef)PLANK_NULL;
    array = (PlankDynamicArrayRef)p->source;
    arraySize = pl_DynamicArray_GetSize (array);
    
    p->index = p->index + 1;
    
    if ((p->index < 0) || (p->index > arraySize))
        p->index = 0;

    fileArray = (PlankFile*)pl_DynamicArray_GetArray (array);
    file = fileArray + p->index;
    p->currentFile = file;
    
exit:
    return result;
}

PlankResult pl_MultiFileReaderArrayRandomNextFunction (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankDynamicArrayRef array;
    PlankFile* fileArray;
    PlankFileRef file;
    int arraySize;
    
    file = (PlankFileRef)PLANK_NULL;
    array = (PlankDynamicArrayRef)p->source;
    arraySize = pl_DynamicArray_GetSize (array);
    
    p->index = pl_RNG_NextInt (pl_RNGGlobal(), arraySize);
    fileArray = (PlankFile*)pl_DynamicArray_GetArray (array);
    file = fileArray + p->index;
    p->currentFile = file;

exit:
    return result;
}

PlankResult pl_MultiFileReaderArrayRandomNoRepeatNextFunction (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankDynamicArrayRef array;
    PlankFile* fileArray;
    PlankFileRef file;
    int arraySize, index;
    
    file = (PlankFileRef)PLANK_NULL;
    array = (PlankDynamicArrayRef)p->source;
    arraySize = pl_DynamicArray_GetSize (array);
    
    if (arraySize < 2)
        return pl_MultiFileReaderArrayRandomNextFunction (p);
    
    index = p->index;
    
    while (index == p->index)
        index = pl_RNG_NextInt (pl_RNGGlobal(), arraySize);
    
    p->index = index;
    fileArray = (PlankFile*)pl_DynamicArray_GetArray (array);
    file = fileArray + p->index;
    p->currentFile = file;
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_Queue_FreeCurrentFile (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankLockFreeQueueRef q;
    PlankLockFreeQueueFreeElementDataFunction f;

    if (p->currentFile)
    {
        q = (PlankLockFreeQueueRef)p->source;
        f = pl_LockFreeQueue_GetFreeElementDataFunction (q);
        
        if (f)
            result = (f) (p->currentFile);
        
        p->currentFile = (PlankFileRef)PLANK_NULL;
    }
    
    return result;
}

PlankResult pl_MultiFileReaderQueueNextFunction (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    PlankFileRef file;
    PlankLockFreeQueueRef q;
    PlankLockFreeQueueElementRef e;
    
    file = (PlankFileRef)PLANK_NULL;
    q = (PlankLockFreeQueueRef)p->source;
    e = (PlankLockFreeQueueElementRef)PLANK_NULL;
    
    if ((result = pl_LockFreeQueue_Pop (q, &e)) != PlankResult_OK)
        goto exit;
    
    if (e)
    {
        file = (PlankFileRef)pl_LockFreeQueueElement_GetData (e);
        pl_LockFreeQueueElement_Destroy (e);
    }
    
exit:
    result = pl_MultiFileReader_Queue_FreeCurrentFile (p);
    
    p->currentFile = file;
    
    return result;
}

///----

//PlankMulitFileReaderRef pl_MultiFileReader_CreateAndInit()
//{
//    PlankMulitFileReaderRef p;
//    p = pl_MultiFileReader_Create();
//    
//    if (p != PLANK_NULL)
//    {
//        if (pl_MultiFileReader_Init (p) != PlankResult_OK)
//            pl_MultiFileReader_Destroy (p);
//        else
//            return p;
//    }
//    
//    return PLANK_NULL;
//}

PlankMulitFileReaderRef pl_MultiFileReader_Create()
{
    PlankMemoryRef m;
    PlankMulitFileReaderRef p;
    
    m = pl_MemoryGlobal();
    p = (PlankMulitFileReaderRef)pl_Memory_AllocateBytes (m, sizeof (PlankMulitFileReader));
    
    if (p != PLANK_NULL)
        pl_MemoryZero (p, sizeof (PlankMulitFileReader));
    
    return p;
}

//PlankResult pl_MultiFileReader_Init (PlankMulitFileReaderRef p)
//{
//    pl_MemoryZero (p, sizeof (PlankMulitFileReader));
//    return PlankResult_OK;
//}

PlankResult pl_MultiFileReader_InitArraySequence (PlankMulitFileReaderRef p, PlankDynamicArrayRef array, const PlankB loop)
{
    PlankResult result = PlankResult_OK;
    pl_MemoryZero (p, sizeof (PlankMulitFileReader));

    p->index = -1;
    p->source = array;
    
    if (loop)
    {
        p->mode = PLANKMULITFILE_MODE_ARRAYSEQUENCELOOP;
        p->nextFuntion = pl_MultiFileReaderArraySequenceLoopNextFunction;
    }
    else
    {
        p->mode = PLANKMULITFILE_MODE_ARRAYSEQUENCEONCE;
        p->nextFuntion = pl_MultiFileReaderArraySequenceOnceNextFunction;
    }
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_InitArrayRandom (PlankMulitFileReaderRef p, PlankDynamicArrayRef array, const PlankB noRepeat)
{
    PlankResult result = PlankResult_OK;
    pl_MemoryZero (p, sizeof (PlankMulitFileReader));
    
    p->index = -1;
    p->source = array;
    
    if (noRepeat)
    {
        p->mode = PLANKMULITFILE_MODE_ARRAYRANDOMNOREPEAT;
        p->nextFuntion = pl_MultiFileReaderArrayRandomNoRepeatNextFunction;
    }
    else
    {
        p->mode = PLANKMULITFILE_MODE_ARRAYRANDOM;
        p->nextFuntion = pl_MultiFileReaderArrayRandomNextFunction;
    }

exit:
    return result;
}

PlankResult pl_MultiFileReader_InitArrayCallback (PlankMulitFileReaderRef p, PlankDynamicArrayRef array, PlankMultiFileNextFunction callback)
{
    PlankResult result = PlankResult_OK;
    pl_MemoryZero (p, sizeof (PlankMulitFileReader));
    
    p->index = -1;
    p->source = array;
    p->mode = PLANKMULITFILE_MODE_ARRAYCALLBACK;
    p->nextFuntion = callback;
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_InitQueue (PlankMulitFileReaderRef p, PlankLockFreeQueueRef queue)
{
    PlankResult result = PlankResult_OK;
    pl_MemoryZero (p, sizeof (PlankMulitFileReader));
    
    p->index = -1;
    p->source = queue;
    p->mode = PLANKMULITFILE_MODE_QUEUE;
    p->nextFuntion = pl_MultiFileReaderQueueNextFunction;
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_DeInit (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;
    
    if (p == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    if (p->mode == PLANKMULITFILE_MODE_QUEUE)
    {
        result = pl_MultiFileReader_Queue_FreeCurrentFile (p);
    }
    
    pl_MemoryZero (p, sizeof (PlankMulitFileReader));
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_Destroy (PlankMulitFileReaderRef p)
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
    
    if ((result = pl_MultiFileReader_DeInit (p)) != PlankResult_OK)
        goto exit;
    
    result = pl_Memory_Free (m, p);
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_GetMode (PlankMulitFileReaderRef p, int* mode)
{
    *mode = p->mode;
    return PlankResult_OK;
}

PlankResult pl_MultiFileReader_GetArray (PlankMulitFileReaderRef p, PlankDynamicArrayRef* array)
{
    switch (p->mode)
    {
        case PLANKMULITFILE_MODE_ARRAYSEQUENCEONCE:
        case PLANKMULITFILE_MODE_ARRAYSEQUENCELOOP:
        case PLANKMULITFILE_MODE_ARRAYRANDOM:
        case PLANKMULITFILE_MODE_ARRAYRANDOMNOREPEAT:
        case PLANKMULITFILE_MODE_ARRAYCALLBACK:
            *array = (PlankDynamicArrayRef)p->source;
            return PlankResult_OK;
        default:
            *array = (PlankDynamicArrayRef)PLANK_NULL;
            break;
    }
    
    return PlankResult_UnknownError;
}

PlankResult pl_MultiFileReader_GetQueue (PlankMulitFileReaderRef p, PlankLockFreeQueueRef* queue)
{
    switch (p->mode)
    {
        case PLANKMULITFILE_MODE_QUEUE:
            *queue = (PlankLockFreeQueueRef)p->source;
            return PlankResult_OK;
        default:
            *queue = (PlankLockFreeQueueRef)PLANK_NULL;
            break;
    }
    
    return PlankResult_UnknownError;
}

PlankResult pl_MultiFileReader_GetCurrentFile (PlankMulitFileReaderRef p, PlankFileRef* currentFile)
{
    *currentFile = p->currentFile;
    return PlankResult_OK;
}

PlankResult pl_MultiFileReader_GetIndex (PlankMulitFileReaderRef p, int* index)
{
    *index = p->index;
    return PlankResult_OK;
}

PlankResult pl_MultiFileReader_Open (PlankMulitFileReaderRef p)
{
    PlankResult result = PlankResult_OK;

    if (!p->currentFile)
    {
        if ((result = (p->nextFuntion) (p)) != PlankResult_OK)
            goto exit;
        
        if (p->currentFile)
        {
            if (pl_File_IsPositionable (p->currentFile))
                result = pl_File_ResetPosition (p->currentFile);
        }
    }
    
exit:
    return result;
}

PlankResult pl_MultiFileReader_Read (PlankMulitFileReaderRef p, PlankP ptr, int maximumBytes, int* bytesRead)
{
    PlankResult result = PlankResult_OK;
    int bytesThisTime, bytesReadLocal;
    
    bytesReadLocal = 0;
    
    if ((result = pl_MultiFileReader_Open (p)) != PlankResult_OK)
        goto exit;    
    
    while (maximumBytes > 0)
    {
        if (!p->currentFile)
        {
            
//            bytesThisTime = maximumBytes;
//            pl_MemoryZero (ptr, bytesThisTime);
            
            result = PlankResult_FileEOF;
            goto exit;
        }
        else
        {
            result = pl_File_Read (p->currentFile, ptr, maximumBytes, &bytesThisTime);
            
            if ((result != PlankResult_OK) && (result != PlankResult_FileEOF))
                goto exit;
            
            if (pl_File_IsEOF (p->currentFile))
            {
                p->currentFile = (PlankFileRef)PLANK_NULL;

                if ((result = pl_MultiFileReader_Open (p)) != PlankResult_OK)
                    goto exit;
            }
            
            result = PlankResult_OK;
        }
        
        bytesReadLocal += bytesThisTime;
        maximumBytes -= bytesThisTime;
        ptr += bytesThisTime;
    }
    
exit:
    if (bytesRead != PLANK_NULL)
        *bytesRead = bytesReadLocal;
    
    return result;
}


