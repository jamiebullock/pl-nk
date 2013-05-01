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

#ifndef PLANK_IFFFILECOMMON_H
#define PLANK_IFFFILECOMMON_H

#include "plank_File.h"

#define PLANKIFFFILE_CURRENTCHUNKPOSITION   -1
#define PLANKIFFFILE_ANYCHUNKID             -1
#define PLANKIFFFILE_ID_FCC                  1
#define PLANKIFFFILE_ID_GUID                 2

#define PLANKIFFFILE_W64_RIFF_ID          "riff-912E-11CF-A5D6-28DB04C10000"
#define PLANKIFFFILE_W64_LIST_ID          "list-912F-11CF-A5D6-28DB04C10000"
#define PLANKIFFFILE_W64_JUNK_ID          "junk-ACF3-11D3-8CD1-00C04f8EDB8A"


typedef union PlankIffID
{
    PlankFourCharCode fcc;
    PlankGUID guid;
} PlankIffID;

typedef struct PlankIffFileHeaderInfo
{
    PlankIffID mainID;
    PlankIffID formatID;
    PlankIffID junkID;
    
    PlankLL mainLength;

    PlankUC lengthSize;
    PlankUC idType;
    PlankUC headerLength;
    PlankUC reserved3;
    PlankUC mainHeaderEnd;
    PlankUC initMainLength;
    PlankUC alignment;
    PlankUC reserved7;
    
} PlankIffFileHeaderInfo;

typedef struct PlankIffFile* PlankIffFileRef;
typedef struct PlankIffFile
{
    PlankFile file;
    PlankIffFileHeaderInfo headerInfo;
} PlankIffFile;

static inline PlankResult pl_IffFile_WriteChunkLength (PlankIffFileRef p, const PlankLL length)
{
    if (p->headerInfo.lengthSize == 4)
    {
        return pl_File_WriteUI (&p->file, length < 0xffffffff ? (PlankUI)length : 0xffffffff);
    }
    else if (p->headerInfo.lengthSize == 8)
    {
        switch (p->headerInfo.idType)
        {
            case PLANKIFFFILE_ID_FCC:   return pl_File_WriteLL (&p->file, length);
            case PLANKIFFFILE_ID_GUID:  return pl_File_WriteLL (&p->file, length + p->headerInfo.lengthSize + sizeof (PlankGUID)); // includes the chunk header
            default: return PlankResult_UnknownError;
        }
    }
    else
    {
        return PlankResult_UnknownError;
    }
}

static inline PlankResult pl_IffFile_ReadChunkLength (PlankIffFileRef p, PlankLL *length)
{
    PlankResult result;
    PlankUI length32;
    
    result = PlankResult_OK;
    
    if (p->headerInfo.lengthSize == 4)
    {
        if ((result = pl_File_ReadUI (&p->file, &length32)) != PlankResult_OK) goto exit;
        *length = length32;
    }
    else if (p->headerInfo.lengthSize == 8)
    {
        switch (p->headerInfo.idType)
        {
            case PLANKIFFFILE_ID_FCC:
                result = pl_File_ReadLL (&p->file, length);
                break;
            case PLANKIFFFILE_ID_GUID:
                if ((result = pl_File_ReadLL (&p->file, length)) != PlankResult_OK) goto exit;
                *length -= p->headerInfo.lengthSize + sizeof (PlankGUID); // includes the chunk header
                break;
            default:
                result = PlankResult_UnknownError;
        }
    }
    else
    {
        result = PlankResult_UnknownError;
    }
    
exit:
    return result;
}

static inline PlankResult pl_IffFile_InitID (PlankIffFileRef p, const char* string, PlankIffID* chunkID)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:  chunkID->fcc = pl_FourCharCode (string); return PlankResult_OK;
        case PLANKIFFFILE_ID_GUID: pl_GUID_InitString (&chunkID->guid, string); return PlankResult_OK;
        default: return PlankResult_UnknownError;
    }
}

static inline PlankResult pl_IffFile_WriteChunkID (PlankIffFileRef p, const PlankIffID* chunkID)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   return pl_File_WriteFourCharCode (&p->file, chunkID->fcc);
        case PLANKIFFFILE_ID_GUID:  return pl_File_WriteGUID (&p->file, &chunkID->guid);
        default: return PLANK_FALSE;
    }
}

static inline PlankResult pl_IffFile_ReadChunkID (PlankIffFileRef p, PlankIffID* chunkID)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   return pl_File_ReadFourCharCode (&p->file, &chunkID->fcc);
        case PLANKIFFFILE_ID_GUID:  return pl_File_ReadGUID (&p->file, &chunkID->guid);
        default: return PLANK_FALSE;
    }
}

static inline PlankB pl_IffFile_EqualIDs (PlankIffFileRef p, const PlankIffID* id1,  const PlankIffID* id2)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   return id1->fcc == id2->fcc;
        case PLANKIFFFILE_ID_GUID:  return pl_GUID_Equal (&id1->guid, &id2->guid);
        default: return PLANK_FALSE;
    }
}

static inline PlankB pl_IffFile_IsNullID (PlankIffFileRef p, const PlankIffID* chunkID)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   return chunkID->fcc == 0;
        case PLANKIFFFILE_ID_GUID:  return pl_GUID_IsNull (&chunkID->guid);
        default: return PLANK_FALSE;
    }
}

static inline void pl_IffFile_ChunkIDString (PlankIffFileRef p, const PlankIffID* chunkID, char* string)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   strcpy (string, pl_FourCharCode2String (chunkID->fcc).string); return;
        case PLANKIFFFILE_ID_GUID:  pl_GUID_HexString (&chunkID->guid, PLANK_FALSE, string); return;
        default: string[0] = '\0'; return;
    }
}

static inline int pl_IffFile_ChunkIDLength (PlankIffFileRef p)
{
    switch (p->headerInfo.idType)
    {
        case PLANKIFFFILE_ID_FCC:   return 4;
        case PLANKIFFFILE_ID_GUID:  return 16;
        default: return 0;
    }
}

static inline const PlankIffID* pl_IffFileAnyID()
{
    static PlankIffID any;
    static PlankB firstTime = PLANK_TRUE;
    
    if (firstTime)
    {
        any.guid.data1 = 0xffffffff;
        any.guid.data2 = 0xffff;
        any.guid.data3 = 0xffff;
        any.guid.data4[0] = 0xff;
        any.guid.data4[1] = 0xff;
        any.guid.data4[2] = 0xff;
        any.guid.data4[3] = 0xff;
        any.guid.data4[4] = 0xff;
        any.guid.data4[5] = 0xff;
        any.guid.data4[6] = 0xff;
        any.guid.data4[7] = 0xff;
    }
    
    return &any;
}



#endif // PLANK_IFFFILECOMMON_H
