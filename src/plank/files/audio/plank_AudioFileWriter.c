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

#include "../../core/plank_StandardHeader.h"
#include "../plank_File.h"
#include "../plank_IffFileWriter.h"
#include "../../maths/plank_Maths.h"
#include "../../random/plank_RNG.h"
#include "plank_AudioFileWriter.h"
#include "plank_AudioFileMetaData.h"
#include "plank_AudioFileCuePoint.h"
#include "plank_AudioFileRegion.h"

#define PLANKAUDIOFILEWRITER_BUFFERLENGTH 256

// private

typedef PlankResult (*PlankAudioFileWriterWriteHeaderFunction)(PlankAudioFileWriterRef);
typedef PlankResult (*PlankAudioFileWriterWriteFramesFunction)(PlankAudioFileWriterRef, const int, const void*);
typedef PlankResult (*PlankAudioFileWriterSetFramePositionFunction)(PlankAudioFileWriterRef, const PlankLL);
typedef PlankResult (*PlankAudioFileWriterGetFramePositionFunction)(PlankAudioFileWriterRef, PlankLL *);

PlankResult pl_AudioFileWriter_WriteHeader (PlankAudioFileWriterRef p);

PlankResult pl_AudioFileWriter_WAV_Open (PlankAudioFileWriterRef p, const char* filepath);
PlankResult pl_AudioFileWriter_WAV_WriteHeader (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_WAVEXT_WriteHeader (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_WAV_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data);

PlankResult pl_AudioFileWriter_AIFF_Open (PlankAudioFileWriterRef p, const char* filepath);
PlankResult pl_AudioFileWriter_AIFF_WriteHeader (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_AIFF_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data);

PlankResult pl_AudioFileWriter_AIFC_Open (PlankAudioFileWriterRef p, const char* filepath);
PlankResult pl_AudioFileWriter_AIFC_WriteHeader (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_AIFC_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data);

PlankResult pl_AudioFileWriter_Iff_WriteFrames (PlankAudioFileWriterRef p, const PlankFourCharCode chunkID, const int numFrames, const void* data);

PlankResult pl_AudioFileWriter_OggVorbis_Open (PlankAudioFileWriterRef p, const char* filepath);
PlankResult pl_AudioFileWriter_OggVorbis_Close (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_OggVorbis_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data);

PlankResult pl_AudioFileWriter_Opus_Open (PlankAudioFileWriterRef p, const char* filepath);
PlankResult pl_AudioFileWriter_Opus_Close (PlankAudioFileWriterRef p);
PlankResult pl_AudioFileWriter_Opus_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data);


//PlankAudioFileWriterRef pl_AudioFileWriter_CreateAndInit()
//{
//    PlankAudioFileWriterRef p;
//    p = pl_AudioFileWriter_Create();
//    
//    if (p != PLANK_NULL)
//    {
//        if (pl_AudioFileWriter_Init (p) != PlankResult_OK)
//            pl_AudioFileWriter_Destroy (p);
//        else
//            return p;
//    }
//    
//    return (PlankAudioFileWriterRef)PLANK_NULL;
//}
//
//PlankAudioFileWriterRef pl_AudioFileWriter_Create()
//{
//    return (PlankAudioFileWriterRef)PLANK_NULL;
//}

PlankResult pl_AudioFileWriter_Init (PlankAudioFileWriterRef p)
{
    PlankResult result = PlankResult_OK;
    
    p->peer                        = PLANK_NULL;
    p->formatInfo.format           = PLANKAUDIOFILE_FORMAT_INVALID;
    p->formatInfo.encoding         = PLANKAUDIOFILE_ENCODING_INVALID;
    p->formatInfo.bitsPerSample    = 0;
    p->formatInfo.bytesPerFrame    = 0;
    p->formatInfo.numChannels      = 0;
    p->formatInfo.sampleRate       = 0.0;
    p->formatInfo.channelMask      = 0;
    p->metaData                    = PLANK_NULL;
    
    p->writeFramesFunction         = PLANK_NULL;
    p->setFramePositionFunction    = PLANK_NULL;
    p->getFramePositionFunction    = PLANK_NULL;
    
    return result;
}

PlankResult pl_AudioFileWriter_DeInit (PlankAudioFileWriterRef p)
{
    PlankResult result = PlankResult_OK;
    
    if (p == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    if ((result = pl_AudioFileWriter_Close (p)) != PlankResult_OK) goto exit;
    
    pl_MemoryZero (p, sizeof (PlankAudioFileWriter));
    
exit:
    return result;
}

//PlankResult pl_AudioFileWriter_Destroy (PlankAudioFileWriterRef p)
//{
//    PlankResult result = PlankResult_OK;
//    PlankMemoryRef m = pl_MemoryGlobal();
//    
//    if (p == PLANK_NULL)
//    {
//        result = PlankResult_MemoryError;
//        goto exit;
//    }
//    
//    if ((result = pl_AudioFileWriter_DeInit (p)) != PlankResult_OK)
//        goto exit;
//    
//    result = pl_Memory_Free (m, p);    
//    
//exit:
//    return result;
//}

PlankFileRef pl_AudioFileWriter_GetFile (PlankAudioFileWriterRef p)
{
    return (PlankFileRef)p->peer;
}

PlankAudioFileFormatInfo* pl_AudioFileWriter_GetFormat (PlankAudioFileWriterRef p)
{
    return p->peer ? 0 : &p->formatInfo;
}

PlankResult pl_AudioFileWriter_SetFormatWAV (PlankAudioFileWriterRef p, const int bitsPerSample, const int numChannels, const double sampleRate, const PlankB isFloat)
{
    if (p->peer)
        return PlankResult_UnknownError;
    
    if (isFloat)
    {
        if ((bitsPerSample != 32) && (bitsPerSample != 64))
            return PlankResult_AudioFileInavlidType;
    }
    
    p->formatInfo.format        = PLANKAUDIOFILE_FORMAT_WAV;
    p->formatInfo.encoding      = isFloat ? PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN : PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN;
    p->formatInfo.bitsPerSample = bitsPerSample;
    p->formatInfo.numChannels   = numChannels;
    p->formatInfo.sampleRate    = sampleRate;
    p->formatInfo.bytesPerFrame = (PlankI) (((bitsPerSample + (0x00000008 - 1)) & ~(0x00000008 - 1)) * numChannels / 8);
    
    return PlankResult_OK;
}

PlankResult pl_AudioFileWriter_SetFormatAIFF (PlankAudioFileWriterRef p, const int bitsPerSample, const int numChannels, const double sampleRate)
{
    if (p->peer)
        return PlankResult_UnknownError;
        
    p->formatInfo.format        = PLANKAUDIOFILE_FORMAT_AIFF;
    p->formatInfo.encoding      = PLANKAUDIOFILE_ENCODING_PCM_BIGENDIAN;
    p->formatInfo.bitsPerSample = bitsPerSample;
    p->formatInfo.numChannels   = numChannels;
    p->formatInfo.sampleRate    = sampleRate;
    p->formatInfo.bytesPerFrame = (PlankI) (((bitsPerSample + (0x00000008 - 1)) & ~(0x00000008 - 1)) * numChannels / 8);
    
    return PlankResult_OK;
}

PlankResult pl_AudioFileWriter_SetFormatAIFC (PlankAudioFileWriterRef p, const int bitsPerSample, const int numChannels, const double sampleRate, const PlankB isFloat, const PlankB isLittleEndian)
{
    if (p->peer)
        return PlankResult_UnknownError;
    
    if (isFloat)
    {
        if ((bitsPerSample != 32) && (bitsPerSample != 64))
            return PlankResult_AudioFileInavlidType;
        
        if (isLittleEndian)
            return PlankResult_AudioFileInavlidType;
    }
    
    p->formatInfo.format        = PLANKAUDIOFILE_FORMAT_AIFC;
    p->formatInfo.encoding      = isFloat ? PLANKAUDIOFILE_ENCODING_FLOAT_BIGENDIAN : isLittleEndian ? PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN : PLANKAUDIOFILE_ENCODING_PCM_BIGENDIAN;
    p->formatInfo.bitsPerSample = bitsPerSample;
    p->formatInfo.numChannels   = numChannels;
    p->formatInfo.sampleRate    = sampleRate;
    p->formatInfo.bytesPerFrame = (PlankI) (((bitsPerSample + (0x00000008 - 1)) & ~(0x00000008 - 1)) * numChannels / 8);
    
    return PlankResult_OK;
}

PlankResult pl_AudioFileWriter_SetFormatOggVorbis (PlankAudioFileWriterRef p, const float quality, const int numChannels, const double sampleRate)
{
    if (p->peer)
        return PlankResult_UnknownError;
    
    p->formatInfo.format        = PLANKAUDIOFILE_FORMAT_OGGVORBIS;
    p->formatInfo.encoding      = PLANK_BIGENDIAN ? PLANKAUDIOFILE_ENCODING_FLOAT_BIGENDIAN : PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN;
    p->formatInfo.bitsPerSample = 0;
    p->formatInfo.quality       = pl_ClipF (quality, 0., 10.f);
    p->formatInfo.numChannels   = numChannels;
    p->formatInfo.sampleRate    = sampleRate;
    p->formatInfo.bytesPerFrame = -1;
    
    return PlankResult_OK;
}

PlankResult pl_AudioFileWriter_SetFormatOpus (PlankAudioFileWriterRef p, const float quality, const int numChannels, const double sampleRate)
{
    if (p->peer)
        return PlankResult_UnknownError;
    
    p->formatInfo.format        = PLANKAUDIOFILE_FORMAT_OPUS;
    p->formatInfo.encoding      = PLANK_BIGENDIAN ? PLANKAUDIOFILE_ENCODING_FLOAT_BIGENDIAN : PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN;
    p->formatInfo.bitsPerSample = 0;
    p->formatInfo.quality       = pl_ClipF (quality, 0., 10.f);
    p->formatInfo.numChannels   = numChannels;
    p->formatInfo.sampleRate    = sampleRate;
    p->formatInfo.bytesPerFrame = -1;
    
    return PlankResult_OK;
}

PlankResult pl_AudioFileWriter_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    PlankResult result = PlankResult_OK;
    
    if (p->formatInfo.format == PLANKAUDIOFILE_FORMAT_WAV)
    {
        result = pl_AudioFileWriter_WAV_Open (p, filepath);
    }
    else if (p->formatInfo.format == PLANKAUDIOFILE_FORMAT_AIFF)
    {
        result = pl_AudioFileWriter_AIFF_Open (p, filepath);
    }
    else if (p->formatInfo.format == PLANKAUDIOFILE_FORMAT_AIFC)
    {
        result = pl_AudioFileWriter_AIFC_Open (p, filepath);
    }
#if PLANK_OGGVORBIS
    else if (p->formatInfo.format == PLANKAUDIOFILE_FORMAT_OGGVORBIS)
    {
        result = pl_AudioFileWriter_OggVorbis_Open (p, filepath);
    }
#endif
#if PLANK_OPUS
    else if (p->formatInfo.format == PLANKAUDIOFILE_FORMAT_OPUS)
    {
        result = pl_AudioFileWriter_Opus_Open (p, filepath);
    }
#endif
    else
    {
        result = PlankResult_AudioFileInavlidType;
    }
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_Close (PlankAudioFileWriterRef p)
{
    PlankResult result = PlankResult_OK;
    
    if (p == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    if ((result = pl_AudioFileWriter_WriteHeader (p)) != PlankResult_OK) goto exit;
    
    switch (p->formatInfo.format)
    {
        case PLANKAUDIOFILE_FORMAT_WAV:
        case PLANKAUDIOFILE_FORMAT_AIFF:
        case PLANKAUDIOFILE_FORMAT_AIFC:
        case PLANKAUDIOFILE_FORMAT_UNKNOWNIFF:
            result = pl_IffFileWriter_Destroy ((PlankIffFileWriter*)p->peer);
            break;
        case PLANKAUDIOFILE_FORMAT_OGGVORBIS:
            result = pl_AudioFileWriter_OggVorbis_Close (p);
            break;
        case PLANKAUDIOFILE_FORMAT_OPUS:
            result = pl_AudioFileWriter_Opus_Close (p);
            break;
        default:
            if (p->peer != PLANK_NULL)
                result = PlankResult_UnknownError;
    }
    
    if (result != PlankResult_OK)
        goto exit;
    
    p->peer = PLANK_NULL;
    
    if (p->metaData != PLANK_NULL)
    {
        if ((result = pl_AudioFileMetaData_Destroy (p->metaData)) != PlankResult_OK) goto exit;
        p->metaData = (PlankAudioFileMetaDataRef)PLANK_NULL;
    }
        
exit:
    return result;
}

PlankResult pl_AudioFileWriter_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    if (!p->writeFramesFunction)
        return PlankResult_FunctionsInvalid;
    
    return ((PlankAudioFileWriterWriteFramesFunction)p->writeFramesFunction)(p, numFrames, data);
}

PlankB pl_AudioFileWriter_IsEncodingNativeEndian (PlankAudioFileWriterRef p)
{
#if PLANK_BIGENDIAN
    return !! (p->formatInfo.encoding & PLANKAUDIOFILE_ENCODING_BIGENDIAN_FLAG);
#elif PLANK_LITTLEENDIAN
    return  ! (p->formatInfo.encoding & PLANKAUDIOFILE_ENCODING_BIGENDIAN_FLAG);
#else
#error Neither PLANK_BIGENDIAN or PLANK_LITTLEENDIAN are set to 1
#endif
}

PlankResult pl_AudioFileWriter_WriteHeader (PlankAudioFileWriterRef p)
{
    if (!p->writeHeaderFunction)
        return PlankResult_OK; // not all formats have a header so this is OK
    
    return ((PlankAudioFileWriterWriteHeaderFunction)p->writeHeaderFunction)(p);
}

PlankResult pl_AudioFileWriter_WAV_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
        
    if (!((p->formatInfo.encoding == PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN) ||
          (p->formatInfo.encoding == PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN)))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (p->formatInfo.numChannels < 1)
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if ((p->formatInfo.bitsPerSample > 32) &&
        (p->formatInfo.encoding != PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if ((p->formatInfo.bitsPerSample < 8) || (p->formatInfo.bitsPerSample > 64))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (p->formatInfo.bytesPerFrame == 0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    if (p->formatInfo.sampleRate <= 0.0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    iff = pl_IffFileWriter_CreateAndInit();
    
    result = pl_IffFileWriter_OpenReplacing (iff, filepath, PLANK_FALSE, pl_FourCharCode ("RIFF"), pl_FourCharCode ("WAVE"));
    
    if (result != PlankResult_OK)
        goto exit;
    
    p->peer = iff;
        
    p->writeFramesFunction = pl_AudioFileWriter_WAV_WriteFrames;
    p->writeHeaderFunction = pl_AudioFileWriter_WAV_WriteHeader;
    p->setFramePositionFunction = 0; // need ?
    p->getFramePositionFunction = 0; // need ?
    
    if ((result = pl_AudioFileWriter_WriteHeader (p)) != PlankResult_OK) goto exit;
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_WAV_WriteHeader (PlankAudioFileWriterRef p)
{
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    PlankFourCharCode fmt;
    PlankUS encoding;
    
    iff = (PlankIffFileWriterRef)p->peer;
    fmt = pl_FourCharCode ("fmt ");
    
    switch (p->formatInfo.encoding)
    {
        case PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN:   encoding = PLANKAUDIOFILE_WAV_COMPRESSION_PCM;   break;
        case PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN: encoding = PLANKAUDIOFILE_WAV_COMPRESSION_FLOAT; break;
        default: result = PlankResult_AudioFileInavlidType; goto exit;
    }
    
    if ((result = pl_IffFileWriter_SeekChunk (iff, fmt, &chunkInfo, 0)) != PlankResult_OK) goto exit;

    if (!chunkInfo)
    {
        if ((result = pl_IffFileWriter_WriteChunk (iff, fmt, 0, PLANKAUDIOFILE_WAV_FMT_LENGTH, PLANKIFFFILEWRITER_MODEAPPEND)) != PlankResult_OK) goto exit;
        if ((result = pl_IffFileWriter_SeekChunk (iff, fmt, &chunkInfo, 0)) != PlankResult_OK) goto exit;
        
        if (!chunkInfo)
        {
            result = PlankResult_FileReadError;
            goto exit;
        }
    }
        
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, encoding)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.numChannels)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)p->formatInfo.sampleRate)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)(p->formatInfo.bytesPerFrame * (int)p->formatInfo.sampleRate))) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.bytesPerFrame)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.bitsPerSample)) != PlankResult_OK) goto exit;

exit:
    return result;
}

PlankResult pl_AudioFileWriter_WAVEXT_WriteHeader (PlankAudioFileWriterRef p)
{
    PlankAudioFileWAVExtensible* ext;
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    PlankFourCharCode fmt;
    
    iff = (PlankIffFileWriterRef)p->peer;
    fmt = pl_FourCharCode ("fmt ");
    
    switch (p->formatInfo.encoding)
    {
        case PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN:   ext = pl_AudioFileWAVExtensible_GetPCM();   break;
        case PLANKAUDIOFILE_ENCODING_FLOAT_LITTLEENDIAN: ext = pl_AudioFileWAVExtensible_GetFloat(); break;
        default: result = PlankResult_AudioFileInavlidType; goto exit;
    }
    
    if ((result = pl_IffFileWriter_SeekChunk (iff, fmt, &chunkInfo, 0)) != PlankResult_OK) goto exit;
    
    if (!chunkInfo)
    {
        if ((result = pl_IffFileWriter_WriteChunk (iff, fmt, 0, PLANKAUDIOFILE_WAV_FMT_EXTENSIBLE_LENGTH, PLANKIFFFILEWRITER_MODEAPPEND)) != PlankResult_OK) goto exit;
        if ((result = pl_IffFileWriter_SeekChunk (iff, fmt, &chunkInfo, 0)) != PlankResult_OK) goto exit;
        
        if (!chunkInfo)
        {
            result = PlankResult_FileReadError;
            goto exit;
        }
    }
    
    // regular WAV fmt
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)PLANKAUDIOFILE_WAV_COMPRESSION_EXTENSIBLE)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.numChannels)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)p->formatInfo.sampleRate)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)(p->formatInfo.bytesPerFrame * (int)p->formatInfo.sampleRate))) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.bytesPerFrame)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.bitsPerSample)) != PlankResult_OK) goto exit;

    // extensible part
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PLANKAUDIOFILE_WAV_FMT_EXTENSIBLE_LENGTH - PLANKAUDIOFILE_WAV_FMT_LENGTH - sizeof (PlankUS)))) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)p->formatInfo.bitsPerSample)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)p->formatInfo.channelMask)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, (PlankUI)ext->ext1)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)ext->ext2)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, (PlankUS)ext->ext3)) != PlankResult_OK) goto exit;
    if ((result = pl_File_Write   ((PlankFileRef)iff, ext->ext4, 8)) != PlankResult_OK) goto exit;

exit:
    return result;
}

PlankResult pl_AudioFileWriter_WAV_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    return pl_AudioFileWriter_Iff_WriteFrames (p, pl_FourCharCode ("data"), numFrames, data);
}

PlankResult pl_AudioFileWriter_AIFF_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    
    if (p->formatInfo.encoding != PLANKAUDIOFILE_ENCODING_PCM_BIGENDIAN)
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (p->formatInfo.numChannels < 1)
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
        
    if ((p->formatInfo.bitsPerSample < 8) || (p->formatInfo.bitsPerSample > 32))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (p->formatInfo.bytesPerFrame == 0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    if (p->formatInfo.sampleRate <= 0.0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    iff = pl_IffFileWriter_CreateAndInit();
    
    result = pl_IffFileWriter_OpenReplacing (iff, filepath, PLANK_TRUE, pl_FourCharCode ("FORM"), pl_FourCharCode ("AIFF"));
    
    if (result != PlankResult_OK)
        goto exit;
    
    p->peer = iff;
        
    p->writeFramesFunction = pl_AudioFileWriter_AIFF_WriteFrames;
    p->writeHeaderFunction = pl_AudioFileWriter_AIFF_WriteHeader;
    p->setFramePositionFunction = 0; // need ?
    p->getFramePositionFunction = 0; // need ?
    
    if ((result = pl_AudioFileWriter_WriteHeader (p)) != PlankResult_OK) goto exit;
    
exit:
    return result;
}

static PlankResult pl_AudioFileWriter_AIFF_GetNumFrames (PlankAudioFileWriterRef p, PlankUI* numFrames)
{
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;

    iff = (PlankIffFileWriterRef)p->peer;

    if ((result = pl_IffFileWriter_SeekChunk (iff, pl_FourCharCode ("SSND"), &chunkInfo, 0)) != PlankResult_OK) goto exit;

    if (chunkInfo)
    {
        *numFrames = chunkInfo->chunkLength / p->formatInfo.bytesPerFrame;
    }
    else
    {
        *numFrames = 0;
    }
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_AIFF_WriteHeader (PlankAudioFileWriterRef p)
{
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    PlankFourCharCode COMM;
    PlankF80 sampleRate;
    PlankUI numFrames;
    
    iff = (PlankIffFileWriterRef)p->peer;
    COMM = pl_FourCharCode ("COMM");
        
    if ((result = pl_IffFileWriter_SeekChunk (iff, COMM, &chunkInfo, 0)) != PlankResult_OK) goto exit;
    
    if (!chunkInfo)
    {
        if ((result = pl_IffFileWriter_WriteChunk (iff, COMM, 0, PLANKAUDIOFILE_AIFF_COMM_LENGTH, PLANKIFFFILEWRITER_MODEAPPEND)) != PlankResult_OK) goto exit;
        if ((result = pl_IffFileWriter_SeekChunk (iff, COMM, &chunkInfo, 0)) != PlankResult_OK) goto exit;
        
        if (!chunkInfo)
        {
            result = PlankResult_FileReadError;
            goto exit;
        }
    }
    
    if ((result = pl_AudioFileWriter_AIFF_GetNumFrames (p, &numFrames)) != PlankResult_OK) goto exit;
    
    sampleRate = pl_I2F80 ((PlankUI)p->formatInfo.sampleRate);
    
    if ((result = pl_File_WriteS  ((PlankFileRef)iff, (PlankS)p->formatInfo.numChannels)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, numFrames)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteS  ((PlankFileRef)iff, (PlankS)p->formatInfo.bitsPerSample)) != PlankResult_OK) goto exit;
    if ((result = pl_File_Write   ((PlankFileRef)iff, sampleRate.data, sizeof (sampleRate))) != PlankResult_OK) goto exit;
    
exit:
    return result;    
}

PlankResult pl_AudioFileWriter_AIFF_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    return pl_AudioFileWriter_Iff_WriteFrames (p, pl_FourCharCode ("SSND"), numFrames, data);
}

PlankResult pl_AudioFileWriter_AIFC_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    
    if (p->formatInfo.numChannels < 1)
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if ((p->formatInfo.bitsPerSample > 32) &&
        (p->formatInfo.encoding != PLANKAUDIOFILE_ENCODING_FLOAT_BIGENDIAN))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }

    if ((p->formatInfo.bitsPerSample < 8) || (p->formatInfo.bitsPerSample > 64))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (p->formatInfo.bytesPerFrame == 0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    if (p->formatInfo.sampleRate <= 0.0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    iff = pl_IffFileWriter_CreateAndInit();
    
    result = pl_IffFileWriter_OpenReplacing (iff, filepath, PLANK_TRUE, pl_FourCharCode ("FORM"), pl_FourCharCode ("AIFC"));
    
    if (result != PlankResult_OK)
        goto exit;
    
    p->peer = iff;
    
    p->writeFramesFunction = pl_AudioFileWriter_AIFC_WriteFrames;
    p->writeHeaderFunction = pl_AudioFileWriter_AIFC_WriteHeader;
    p->setFramePositionFunction = 0; // need ?
    p->getFramePositionFunction = 0; // need ?
    
    if ((result = pl_AudioFileWriter_WriteHeader (p)) != PlankResult_OK) goto exit;
    
exit:
    return result;
}

static PlankResult pl_AudioFileWriter_AIFC_GetNumFrames (PlankAudioFileWriterRef p, PlankUI* numFrames)
{
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    
    iff = (PlankIffFileWriterRef)p->peer;
    
    if ((result = pl_IffFileWriter_SeekChunk (iff, pl_FourCharCode ("SSND"), &chunkInfo, 0)) != PlankResult_OK) goto exit;
    
    if (chunkInfo)
    {
        *numFrames = chunkInfo->chunkLength / p->formatInfo.bytesPerFrame;
    }
    else
    {
        *numFrames = 0;
    }
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_AIFC_WriteHeader (PlankAudioFileWriterRef p)
{
    PlankIffFileWriterChunkInfoRef chunkInfo;
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    PlankFourCharCode COMM, FVER, compressionID;
    PlankF80 sampleRate;
    PlankUI numFrames, fver;
    
    iff = (PlankIffFileWriterRef)p->peer;
    COMM = pl_FourCharCode ("COMM");
    FVER = pl_FourCharCode ("FVER");
    fver = PLANKAUDIOFILE_AIFC_VERSION;
    
#if PLANK_LITTLEENDIAN
    pl_SwapEndianUI (&fver);
#endif
    
    if ((result = pl_AudioFileWriter_AIFC_GetNumFrames (p, &numFrames)) != PlankResult_OK) goto exit;
    if ((result = pl_IffFileWriter_WriteChunk (iff, FVER, &fver, sizeof (fver), PLANKIFFFILEWRITER_MODEREPLACEGROW)) != PlankResult_OK) goto exit;
    if ((result = pl_IffFileWriter_SeekChunk (iff, COMM, &chunkInfo, 0)) != PlankResult_OK) goto exit;
    
    if (!chunkInfo)
    {
        if ((result = pl_IffFileWriter_WriteChunk (iff, COMM, 0, PLANKAUDIOFILE_AIFC_COMM_LENGTH, PLANKIFFFILEWRITER_MODEAPPEND)) != PlankResult_OK) goto exit;
        if ((result = pl_IffFileWriter_SeekChunk (iff, COMM, &chunkInfo, 0)) != PlankResult_OK) goto exit;
        
        if (!chunkInfo)
        {
            result = PlankResult_FileReadError;
            goto exit;
        }
    }
        
    sampleRate = pl_I2F80 ((PlankUI)p->formatInfo.sampleRate);
    
    switch (p->formatInfo.encoding) {
        case PLANKAUDIOFILE_ENCODING_PCM_BIGENDIAN:
            compressionID = pl_FourCharCode ("NONE");
            break;
        case PLANKAUDIOFILE_ENCODING_PCM_LITTLEENDIAN:
            compressionID = pl_FourCharCode ("sowt");
            break;
        case PLANKAUDIOFILE_ENCODING_FLOAT_BIGENDIAN:
            if (p->formatInfo.bitsPerSample == 32)
            {
                compressionID = pl_FourCharCode ("fl32");
            }
            else if (p->formatInfo.bitsPerSample == 64)
            {
                compressionID = pl_FourCharCode ("fl64");
            }
            else
            {
                result = PlankResult_AudioFileInavlidType;
                goto exit;
            }
                
            break;
    }
    
    if ((result = pl_File_WriteS  ((PlankFileRef)iff, (PlankS)p->formatInfo.numChannels)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUI ((PlankFileRef)iff, numFrames)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteS  ((PlankFileRef)iff, (PlankS)p->formatInfo.bitsPerSample)) != PlankResult_OK) goto exit;
    if ((result = pl_File_Write   ((PlankFileRef)iff, sampleRate.data, sizeof (sampleRate))) != PlankResult_OK) goto exit;
    
    if ((result = pl_File_WriteFourCharCode ((PlankFileRef)p->peer, compressionID)) != PlankResult_OK) goto exit;
    if ((result = pl_File_WriteUS ((PlankFileRef)iff, 0)) != PlankResult_OK) goto exit; // compression description as a pascal string
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_AIFC_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    return pl_AudioFileWriter_Iff_WriteFrames (p, pl_FourCharCode ("SSND"), numFrames, data);
}

PlankResult pl_AudioFileWriter_Iff_WriteFrames (PlankAudioFileWriterRef p, const PlankFourCharCode chunkID, const int numFrames, const void* data)
{
    PlankUC buffer[PLANKAUDIOFILEWRITER_BUFFERLENGTH];
    PlankResult result = PlankResult_OK;
    PlankIffFileWriterRef iff;
    PlankUC* ptr;
    PlankUC* swapPtr;
    int numSamplesRemaining, numSamplesThisTime, numBufferSamples, bytesPerSample, numBytes, i, numChannels;
    
    iff = (PlankIffFileWriterRef)p->peer;
    numChannels = p->formatInfo.numChannels;
    bytesPerSample = p->formatInfo.bytesPerFrame / numChannels;

    if ((bytesPerSample == 1) || pl_AudioFileWriter_IsEncodingNativeEndian (p))
    {
        result = pl_IffFileWriter_WriteChunk (iff, chunkID, data, numFrames * p->formatInfo.bytesPerFrame, PLANKIFFFILEWRITER_MODEAPPEND);
        if (result != PlankResult_OK) goto exit;
    }
    else
    {
        numBufferSamples = (PLANKAUDIOFILEWRITER_BUFFERLENGTH / p->formatInfo.bytesPerFrame) * p->formatInfo.numChannels;
        numSamplesRemaining = numFrames * p->formatInfo.numChannels;
        ptr = (PlankUC*)data;
        
        // this is unrolled to help optimisation as this is slower that we want anyway: having to swap endianness...!
        switch (bytesPerSample)
        {
            case 2:
                while (numSamplesRemaining > 0)
                {
                    numSamplesThisTime = pl_MinI (numSamplesRemaining, numBufferSamples);
                    numBytes = numSamplesThisTime * bytesPerSample;
                    pl_MemoryCopy (buffer, ptr, numBytes);
                    
                    swapPtr = buffer;
                    
                    for (i = 0; i < numSamplesThisTime; ++i)
                    {
                        pl_SwapEndianS ((PlankS*)swapPtr);
                        swapPtr += 2;
                    }
                    
                    result = pl_IffFileWriter_WriteChunk (iff, chunkID, buffer, numBytes, PLANKIFFFILEWRITER_MODEAPPEND);
                    if (result != PlankResult_OK) goto exit;

                    numSamplesRemaining -= numSamplesThisTime;
                    ptr += numBytes;
                }
                break;
            case 3:
                while (numSamplesRemaining > 0)
                {
                    numSamplesThisTime = pl_MinI (numSamplesRemaining, numBufferSamples);
                    numBytes = numSamplesThisTime * bytesPerSample;
                    pl_MemoryCopy (buffer, ptr, numBytes);
                    
                    // not sure why!
//                    swapPtr = buffer;
//                    
//                    for (i = 0; i < numSamplesThisTime; ++i)
//                    {
//                        pl_SwapEndianI24 ((PlankI24*)swapPtr);
//                        swapPtr += 3;
//                    }
                    
                    result = pl_IffFileWriter_WriteChunk (iff, chunkID, buffer, numBytes, PLANKIFFFILEWRITER_MODEAPPEND);
                    if (result != PlankResult_OK) goto exit;
                    
                    numSamplesRemaining -= numSamplesThisTime;
                    ptr += numBytes;
                }
                break;
            case 4:
                while (numSamplesRemaining > 0)
                {
                    numSamplesThisTime = pl_MinI (numSamplesRemaining, numBufferSamples);
                    numBytes = numSamplesThisTime * bytesPerSample;
                    pl_MemoryCopy (buffer, ptr, numBytes);
                    
                    swapPtr = buffer;
                    
                    for (i = 0; i < numSamplesThisTime; ++i)
                    {
                        pl_SwapEndianI ((PlankI*)swapPtr);
                        swapPtr += 4;
                    }
                    
                    result = pl_IffFileWriter_WriteChunk (iff, chunkID, buffer, numBytes, PLANKIFFFILEWRITER_MODEAPPEND);
                    if (result != PlankResult_OK) goto exit;
                    
                    numSamplesRemaining -= numSamplesThisTime;
                    ptr += numBytes;
                }
                break;
            case 8:
                while (numSamplesRemaining > 0)
                {
                    numSamplesThisTime = pl_MinI (numSamplesRemaining, numBufferSamples);
                    numBytes = numSamplesThisTime * bytesPerSample;
                    pl_MemoryCopy (buffer, ptr, numBytes);
                    
                    swapPtr = buffer;
                    
                    for (i = 0; i < numSamplesThisTime; ++i)
                    {
                        pl_SwapEndianLL ((PlankLL*)swapPtr);
                        swapPtr += 8;
                    }
                    
                    result = pl_IffFileWriter_WriteChunk (iff, chunkID, buffer, numBytes, PLANKIFFFILEWRITER_MODEAPPEND);
                    if (result != PlankResult_OK) goto exit;
                    
                    numSamplesRemaining -= numSamplesThisTime;
                    ptr += numBytes;
                }
                break;
            default:
                result = PlankResult_AudioFileInavlidType;
                goto exit;
        }        
    }
    
exit:
    return result;
}


#if PLANK_OGGVORBIS

#include "../../containers/plank_DynamicArray.h"
typedef struct PlankOggVorbisFileWriter
{
    PlankFile file;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;
} PlankOggVorbisFileWriter;

typedef PlankOggVorbisFileWriter* PlankOggVorbisFileWriterRef;

static PlankResult pl_AudioFileWriter_OggVorbis_CommentAddTag (PlankOggVorbisFileWriterRef p, const char* key, const char* string)
{    
    if (strlen (string) > 0)
    {
        vorbis_comment_add_tag (&p->vc, key, string);
    }
    
    return PlankResult_OK;
}

static PlankResult pl_AudioFileWriter_OggVorbis_Clear (PlankOggVorbisFileWriterRef p)
{
    PlankResult result;
    PlankMemoryRef m;
    
    result = PlankResult_OK;
    m = pl_MemoryGlobal();

    ogg_stream_clear (&p->os);
    vorbis_block_clear (&p->vb);
    vorbis_dsp_clear (&p->vd);
    vorbis_comment_clear (&p->vc);
    vorbis_info_clear (&p->vi);
    
    result = pl_Memory_Free (m, p);

    return result;
}

PlankResult pl_AudioFileWriter_OggVorbis_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    PlankResult result;
    PlankOggVorbisFileWriterRef ogg;
    PlankRNGRef rng;
    PlankMemoryRef m;    
    int err;
    
    result = PlankResult_OK;
    
    if ((p->formatInfo.numChannels < 1) || (p->formatInfo.numChannels > 2))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (!(p->formatInfo.encoding & PLANKAUDIOFILE_ENCODING_FLOAT_FLAG))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
        
    if (p->formatInfo.sampleRate <= 0.0)
    {
        result = PlankResult_AudioFileNotReady;
        goto exit;
    }
    
    m = pl_MemoryGlobal();
    ogg = (PlankOggVorbisFileWriterRef)pl_Memory_AllocateBytes (m, sizeof (PlankOggVorbisFileWriter));
    
    if (ogg == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    pl_MemoryZero (ogg, sizeof (PlankOggVorbisFileWriter));
    
    if ((result = pl_File_Init ((PlankFileRef)ogg)) != PlankResult_OK) goto exit;
    if ((result = pl_File_OpenBinaryWrite ((PlankFileRef)ogg, filepath, PLANK_FALSE, PLANK_TRUE, PLANK_FALSE)) != PlankResult_OK) goto exit;
    
    vorbis_info_init (&ogg->vi);
        
    err = vorbis_encode_init_vbr (&ogg->vi,
                                  p->formatInfo.numChannels,
                                  (int)p->formatInfo.sampleRate,
                                  pl_ClipF (p->formatInfo.quality, 0.f, 1.f));
    
    if (!err)
    {
        vorbis_comment_init (&ogg->vc);
        pl_AudioFileWriter_OggVorbis_CommentAddTag (ogg, "ENCODER", "Plink|Plonk|Plank");
        
        vorbis_analysis_init (&ogg->vd, &ogg->vi);
        vorbis_block_init (&ogg->vd, &ogg->vb);
        
        rng = pl_RNGGlobal();
        ogg_stream_init (&ogg->os, pl_RNG_Next (rng));
        
        vorbis_analysis_headerout (&ogg->vd, &ogg->vc, &header, &header_comm, &header_code);
        
        ogg_stream_packetin (&ogg->os, &header);
        ogg_stream_packetin (&ogg->os, &header_comm);
        ogg_stream_packetin (&ogg->os, &header_code);
        
        do
        {
            if ((err = ogg_stream_flush (&ogg->os, &ogg->og)) != 0)
            {
                if ((result = pl_File_Write ((PlankFileRef)ogg, ogg->og.header, ogg->og.header_len)) != PlankResult_OK) goto exit;
                if ((result = pl_File_Write ((PlankFileRef)ogg, ogg->og.body,   ogg->og.body_len))   != PlankResult_OK) goto exit;
            }
        } while (err != 0);
        
        p->peer = ogg;
        
        p->writeFramesFunction = pl_AudioFileWriter_OggVorbis_WriteFrames;
        p->writeHeaderFunction = 0;
        p->setFramePositionFunction = 0; // need ?
        p->getFramePositionFunction = 0; // need ?
    }

exit:
    if ((p->peer == 0) && (ogg != 0))
    {
        pl_AudioFileWriter_OggVorbis_Clear (ogg);
    }
    
    return result;
}

static PlankResult pl_AudioFileWriter_OggVorbis_WriteData (PlankAudioFileWriterRef p, const int count)
{
    PlankResult result;
    PlankOggVorbisFileWriterRef ogg;
    int err;
    
    ogg = (PlankOggVorbisFileWriterRef)p->peer;

    vorbis_analysis_wrote (&ogg->vd, count);
    
    while (vorbis_analysis_blockout (&ogg->vd, &ogg->vb) == 1)
    {
        vorbis_analysis (&ogg->vb, 0);
        vorbis_bitrate_addblock (&ogg->vb);
        
        while (vorbis_bitrate_flushpacket (&ogg->vd, &ogg->op))
        {
            ogg_stream_packetin (&ogg->os, &ogg->op);
            
            do
            {
                if ((err = ogg_stream_pageout (&ogg->os, &ogg->og)) != 0)
                {
                    if ((result = pl_File_Write ((PlankFileRef)ogg, ogg->og.header, ogg->og.header_len)) != PlankResult_OK) goto exit;
                    if ((result = pl_File_Write ((PlankFileRef)ogg, ogg->og.body,   ogg->og.body_len))   != PlankResult_OK) goto exit;
                }
            } while ((err != 0) && (ogg_page_eos (&ogg->og) == 0));
        }
    }
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_OggVorbis_Close (PlankAudioFileWriterRef p)
{
    PlankResult result;
    PlankOggVorbisFileWriterRef ogg;

    result = PlankResult_OK;
    ogg = (PlankOggVorbisFileWriterRef)p->peer;
    
    if (ogg)
    {
        pl_AudioFileWriter_OggVorbis_WriteData (p, 0);
        if ((result = pl_File_DeInit ((PlankFileRef)ogg)) != PlankResult_OK) goto exit;
        if ((result = pl_AudioFileWriter_OggVorbis_Clear (ogg)) != PlankResult_OK) goto exit;
    }
    
    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_OggVorbis_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    PlankResult result;
    PlankOggVorbisFileWriterRef ogg;
    float** buffer;
    float* dst;
    const float* src;
    int channel, i, numChannels;
    
    result = PlankResult_OK;
    ogg = (PlankOggVorbisFileWriterRef)p->peer;

    if (numFrames > 0)
    {
        buffer = vorbis_analysis_buffer (&ogg->vd, numFrames);
        numChannels = (int)p->formatInfo.numChannels;
        
        for (channel = 0; channel < numChannels; ++channel)
        {
            src = (const float*)data + channel;
            dst = buffer[channel];
            
            for (i = 0; i < numFrames; ++i)
            {
                dst[i] = *src;
                src += numChannels;
            }
        }        
    }
    
    result = pl_AudioFileWriter_OggVorbis_WriteData (p, numFrames);
    
exit:
    return result;
}
#endif // PLANK_OGGVORBIS


#if PLANK_OPUS

#include "../../containers/plank_DynamicArray.h"
typedef struct PlankOpusFileWriter
{
    PlankFile file;
    OpusHeader header;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    OpusMSEncoder* oe;
    
    float frameDuration;
    int packetSize;
    int frameSize;
    PlankDynamicArray mapping;
    PlankDynamicArray packet;
    PlankDynamicArray buffer;
    int bufferPos;
    
} PlankOpusFileWriter;

typedef PlankOpusFileWriter* PlankOpusFileWriterRef;

static PlankResult pl_AudioFileWriter_Opus_Clear (PlankOpusFileWriterRef p)
{
    PlankResult result;
    PlankMemoryRef m;
    
    result = PlankResult_OK;
    m = pl_MemoryGlobal();
    
    ogg_stream_clear (&p->os);
    opus_multistream_encoder_destroy (p->oe);
    
    result = pl_Memory_Free (m, p);
    
    return result;
}

//ugggh!

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
((buf[base+2]<<16)&0xff0000)| \
((buf[base+1]<<8)&0xff00)| \
(buf[base]&0xff))

#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
buf[base+2]=((val)>>16)&0xff; \
buf[base+1]=((val)>>8)&0xff; \
buf[base]=(val)&0xff; \
}while(0)


static void pl_AudioFileWriter_Opus_CommentInit (char **comments, int* length, const char *vendor_string)
{
    int vendor_length=strlen(vendor_string);
    int user_comment_list_length=0;
    int len=8+4+vendor_length+4;
    char *p=(char*)malloc(len);
    if(p==NULL){
        fprintf(stderr, "malloc failed in comment_init()\n");
        exit(1);
    }
    memcpy(p, "OpusTags", 8);
    writeint(p, 8, vendor_length);
    memcpy(p+12, vendor_string, vendor_length);
    writeint(p, 12+vendor_length, user_comment_list_length);
    *length=len;
    *comments=p;
}

static void pl_AudioFileWriter_Opus_CommentFree (char **comments)
{
    free (*comments);
    *comments = 0;
}

static void pl_AudioFileWriter_Opus_CommentAdd (char **comments, int* length, char *tag, char *val)
{
    char* p=*comments;
    int vendor_length=readint(p, 8);
    int user_comment_list_length=readint(p, 8+4+vendor_length);
    int tag_len=(tag?strlen(tag):0);
    int val_len=strlen(val);
    int len=(*length)+4+tag_len+val_len;
    
    p=(char*)realloc(p, len);
    if(p==NULL){
        fprintf(stderr, "realloc failed in comment_add()\n");
        exit(1);
    }
    
    writeint(p, *length, tag_len+val_len);      /* length of comment */
    if(tag) memcpy(p+*length+4, tag, tag_len);  /* comment */
    memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
    writeint(p, 8+4+vendor_length, user_comment_list_length+1);
    *comments=p;
    *length=len;
}

PlankResult pl_AudioFileWriter_Opus_Open (PlankAudioFileWriterRef p, const char* filepath)
{
    unsigned char headerData[100];
    char* comments;
    int commentsLength;
    PlankResult result;
    PlankOpusFileWriterRef opus;
    PlankRNGRef rng;
    PlankMemoryRef m;
    int err, delay, bitRate, streamCount, coupledStreamCount, quality, headerPacketSize;
    opus_int32 sampleRate;
    
    result = PlankResult_OK;
    
    if ((p->formatInfo.numChannels < 1) || (p->formatInfo.numChannels > 255))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
    
    if (!(p->formatInfo.encoding & PLANKAUDIOFILE_ENCODING_FLOAT_FLAG))
    {
        result = PlankResult_AudioFileInavlidType;
        goto exit;
    }
        
    sampleRate = (opus_int32)p->formatInfo.sampleRate;
    
    switch (sampleRate)
    {
        //case 8000: case 12000: case 16000: case 24000: case 48000: break;
        case 48000: // need to fix this
        default:
            result = PlankResult_AudioFileUnsupportedType;
            goto exit;
    }
    
    m = pl_MemoryGlobal();
    opus = (PlankOpusFileWriterRef)pl_Memory_AllocateBytes (m, sizeof (PlankOpusFileWriter));
    
    if (opus == PLANK_NULL)
    {
        result = PlankResult_MemoryError;
        goto exit;
    }
    
    pl_MemoryZero (opus, sizeof (PlankOggVorbisFileWriter));
    
    if ((result = pl_File_Init ((PlankFileRef)opus)) != PlankResult_OK) goto exit;
    if ((result = pl_File_OpenBinaryWrite ((PlankFileRef)opus, filepath, PLANK_FALSE, PLANK_TRUE, PLANK_FALSE)) != PlankResult_OK) goto exit;
    
    opus->frameDuration     = 0.02f; // 960 @ 48kHz
    opus->frameSize         = (int)(opus->frameDuration * 48000.f);
    streamCount             = p->formatInfo.numChannels;
    coupledStreamCount      = 0;
    bitRate                 = 64000 * streamCount + 32000 * coupledStreamCount;
    opus->packetSize        = opus->frameSize * sampleRate / 48000;
    quality                 = (int)p->formatInfo.quality;
    
    if ((result = pl_DynamicArray_InitWithItemSizeAndSize (&opus->mapping, 1, p->formatInfo.numChannels, PLANK_TRUE)) != PlankResult_OK) goto exit;
    if ((result = pl_DynamicArray_InitWithItemSizeAndSize (&opus->packet, 1, PLANKAUDIOFILE_OPUS_MAXPACKETSIZE * streamCount, PLANK_FALSE)) != PlankResult_OK) goto exit;
    if ((result = pl_DynamicArray_InitWithItemSizeAndSize (&opus->buffer, sizeof (float), p->formatInfo.numChannels * opus->frameSize, PLANK_FALSE)) != PlankResult_OK) goto exit;
    
    opus->bufferPos = 0;
    
    opus->oe = opus_multistream_encoder_create (sampleRate,
                                                p->formatInfo.numChannels, // channels
                                                p->formatInfo.numChannels, // streams
                                                0, // coupled streams
                                                (const unsigned char *)pl_DynamicArray_GetArray (&opus->mapping),
                                                OPUS_APPLICATION_AUDIO,
                                                &err);
    if (err)
    {
        result = PlankResult_UnknownError;
        goto exit;
    }
    
    if ((err = opus_multistream_encoder_ctl (opus->oe, OPUS_SET_BITRATE (bitRate))) != 0) { result = PlankResult_UnknownError; goto exit; }
    if ((err = opus_multistream_encoder_ctl (opus->oe, OPUS_SET_COMPLEXITY (quality))) != 0) { result = PlankResult_UnknownError; goto exit; }
    
//        if ((err = opus_multistream_encoder_ctl (opus->oe, OPUS_SET_VBR (1))) != 0) { result = PlankResult_UnknownError; goto exit; }
//        OPUS_SET_VBR_CONSTRAINT(0)
//        OPUS_SET_PACKET_LOSS_PERC(0)
//        OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)
    
    if ((err = opus_multistream_encoder_ctl (opus->oe, OPUS_GET_LOOKAHEAD (&delay))) != 0) { result = PlankResult_UnknownError; goto exit; }

    opus->header.version           = 1;
    opus->header.channels          = p->formatInfo.numChannels;
    opus->header.preskip           = delay;
    opus->header.input_sample_rate = sampleRate;
    opus->header.gain              = 0;
    opus->header.channel_mapping   = 0;
    
    rng = pl_RNGGlobal();
    ogg_stream_init (&opus->os, pl_RNG_Next (rng));

    headerPacketSize = opus_header_to_packet (&opus->header, headerData, 100);
    
    opus->op.packet     = headerData;
    opus->op.bytes      = headerPacketSize;
    opus->op.b_o_s      = 1;
    opus->op.e_o_s      = 0;
    opus->op.granulepos = 0;
    opus->op.packetno   = 0;
    ogg_stream_packetin (&opus->os, &opus->op);
    
    do
    {
        if ((err = ogg_stream_flush (&opus->os, &opus->og)) != 0)
        {
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.header, opus->og.header_len)) != PlankResult_OK) goto exit;
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.body,   opus->og.body_len))   != PlankResult_OK) goto exit;
        }
    } while (err != 0);
    
    pl_AudioFileWriter_Opus_CommentInit (&comments, &commentsLength, opus_get_version_string());
    pl_AudioFileWriter_Opus_CommentAdd (&comments, &commentsLength, "ENCODER=", "Plink|Plonk|Plank");

    opus->op.packet     = (unsigned char *)comments;
    opus->op.bytes      = commentsLength;
    opus->op.b_o_s      = 0;
    opus->op.e_o_s      = 0;
    opus->op.granulepos = 0;
    opus->op.packetno   = 1;
    ogg_stream_packetin (&opus->os, &opus->op);
    
    do
    {
        if ((err = ogg_stream_flush (&opus->os, &opus->og)) != 0)
        {
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.header, opus->og.header_len)) != PlankResult_OK) goto exit;
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.body,   opus->og.body_len))   != PlankResult_OK) goto exit;
        }
    } while (err != 0);

    pl_AudioFileWriter_Opus_CommentFree (&comments);
    
    p->peer = opus;
    
    p->writeFramesFunction = pl_AudioFileWriter_Opus_WriteFrames;
    p->writeHeaderFunction = 0;
    p->setFramePositionFunction = 0; // need ?
    p->getFramePositionFunction = 0; // need ?

    
exit:
    if ((p->peer == 0) && (opus != 0))
    {
        pl_AudioFileWriter_Opus_Clear (opus);
    }
    
    return result;
}

static PlankResult pl_AudioFileWriter_Opus_WriteBuffer (PlankOpusFileWriterRef opus)
{
    PlankResult result;
    int ret, bufferLength;
    float* buffer;
    unsigned char* packetData;
    opus_int32 packetLength;
    
    result       = PlankResult_OK;
    buffer       = (float*)pl_DynamicArray_GetArray (&opus->buffer);
    bufferLength = (int)pl_DynamicArray_GetSize (&opus->buffer);
    packetData   = (unsigned char*)pl_DynamicArray_GetArray (&opus->packet);
    packetLength = (opus_int32)pl_DynamicArray_GetSize (&opus->packet);

    if (opus->bufferPos < bufferLength)
    {
        // is last buffer - pad with zeros
        pl_MemoryZero (buffer + opus->bufferPos, (bufferLength - opus->bufferPos) * sizeof (float));
    }

    ret = opus_multistream_encode_float (opus->oe, (float*)buffer, opus->packetSize, packetData, packetLength);
    
    if (ret < 0)
    {
        result = PlankResult_UnknownError;
        goto exit;
    }
    
    opus->op.packet     = packetData;
    opus->op.bytes      = ret;
    opus->op.b_o_s      = 0;
    opus->op.e_o_s      = opus->bufferPos < bufferLength ? 1 : 0;
    opus->op.granulepos += opus->frameSize * 48000 / opus->header.input_sample_rate;
    opus->op.packetno++;
    ogg_stream_packetin (&opus->os, &opus->op);
    
    do
    {
        if ((ret = ogg_stream_flush (&opus->os, &opus->og)) != 0)
        {
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.header, opus->og.header_len)) != PlankResult_OK) goto exit;
            if ((result = pl_File_Write ((PlankFileRef)opus, opus->og.body,   opus->og.body_len))   != PlankResult_OK) goto exit;
        }
    } while (ret != 0);

    
    //        if(op.e_o_s){
    //            /*We compute the final GP as ceil(len*48k/input_rate). When a resampling
    //             decoder does the matching floor(len*input/48k) conversion the length will
    //             be exactly the same as the input.*/
    //            op.granulepos=((original_samples*48000+rate-1)/rate)+header.preskip;
    //        }

    
exit:
    return result;
}

PlankResult pl_AudioFileWriter_Opus_Close (PlankAudioFileWriterRef p)
{
    PlankResult result;
    PlankOpusFileWriterRef opus;
    
    result = PlankResult_OK;
    opus = (PlankOpusFileWriterRef)p->peer;
    
    if (opus)
    {
        if ((result = pl_AudioFileWriter_Opus_WriteBuffer (opus)) != PlankResult_OK) goto exit;

        if ((result = pl_File_DeInit ((PlankFileRef)opus)) != PlankResult_OK) goto exit;
        if ((result = pl_DynamicArray_DeInit(&opus->mapping)) != PlankResult_OK) goto exit;
        if ((result = pl_DynamicArray_DeInit(&opus->packet)) != PlankResult_OK) goto exit;
        if ((result = pl_DynamicArray_DeInit(&opus->buffer)) != PlankResult_OK) goto exit;
        if ((result = pl_AudioFileWriter_Opus_Clear (opus)) != PlankResult_OK) goto exit;
    }
    
exit:
    return result;    
}

PlankResult pl_AudioFileWriter_Opus_WriteFrames (PlankAudioFileWriterRef p, const int numFrames, const void* data)
{
    PlankResult result;
    PlankOpusFileWriterRef opus;
    float* buffer;
    const float* src;
    int bufferLength, bufferRemaining, numSamplesRemaining, bufferThisTime;
    int numChannels, numSamples;
    
    result = PlankResult_OK;
    opus = (PlankOpusFileWriterRef)p->peer;
    
    numChannels = p->formatInfo.numChannels;
    buffer = (float*)pl_DynamicArray_GetArray (&opus->buffer);
    bufferLength = (int)pl_DynamicArray_GetSize (&opus->buffer);
    bufferRemaining = bufferLength - opus->bufferPos;
    numSamples = numFrames * numChannels;
    numSamplesRemaining = numSamples;
    src = (const float*)data;
    
    while (numSamplesRemaining > 0)
    {
        bufferThisTime = pl_MinI (bufferRemaining, numSamplesRemaining);
        pl_MemoryCopy (buffer + opus->bufferPos, src, bufferThisTime * sizeof (float));
        
        bufferRemaining -= bufferThisTime;
        opus->bufferPos += bufferThisTime;
        numSamplesRemaining -= bufferThisTime;
        src += bufferThisTime;
        
        if (opus->bufferPos == bufferLength)
        {
            result = pl_AudioFileWriter_Opus_WriteBuffer (opus);
            bufferRemaining = bufferLength;
            opus->bufferPos = 0;
        }
    }
        
exit:
    return result;
}

/*
 if ((result = pl_File_Write ((PlankFileRef)opus, PLANKAUDIOFILE_OPUS_HEAD, PLANKAUDIOFILE_OPUS_HEAD_LEN)) != PlankResult_OK) goto exit;
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, 1)) != PlankResult_OK) goto exit; // version
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, p->formatInfo.numChannels)) != PlankResult_OK) goto exit;
 if ((result = pl_File_WriteUS ((PlankFileRef)opus, (PlankUS)delay)) != PlankResult_OK) goto exit;
 if ((result = pl_File_WriteUI ((PlankFileRef)opus, sampleRate)) != PlankResult_OK) goto exit;
 if ((result = pl_File_WriteUS ((PlankFileRef)opus, 0)) != PlankResult_OK) goto exit; // gain in dB
 
 if (p->formatInfo.numChannels > 2)
 {
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, p->formatInfo.numChannels <= 8 ? 1 : 255)) != PlankResult_OK) goto exit;
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, p->formatInfo.numChannels)) != PlankResult_OK) goto exit; // stream count
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, 0)) != PlankResult_OK) goto exit; // coupled streams
 if ((result = pl_File_WriteDynamicArray ((PlankFileRef)opus, &opus->mapping)) != PlankResult_OK) goto exit; // coupled streams
 }
 else
 {
 if ((result = pl_File_WriteUC ((PlankFileRef)opus, 0)) != PlankResult_OK) goto exit;
 }
*/

#endif // PLANK_OPUS
