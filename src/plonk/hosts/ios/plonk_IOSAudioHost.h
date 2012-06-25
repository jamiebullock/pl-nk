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

#ifndef PLONK_IOSAUDIOHOST_H
#define PLONK_IOSAUDIOHOST_H

#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioServices.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

BEGIN_PLONK_NAMESPACE

class IOSAudioHost : public AudioHostBase<float>
{
public:
    typedef AudioHostBase<float>::UnitType UnitType;
    
    IOSAudioHost() throw();
    ~IOSAudioHost();
    
    Text getHostName() const throw();
    Text getNativeHostName() const throw();
    Text getInputName() const throw();
    Text getOutputName() const throw();
    double getCpuUsage() const throw() { return cpuUsage; }
    
    void startHost() throw();
    void stopHost() throw();

    OSStatus renderCallback (UInt32                     inNumberFrames,
                             AudioUnitRenderActionFlags *ioActionFlags, 
                             const AudioTimeStamp 		*inTimeStamp, 
                             AudioBufferList            *ioData) throw();
    
    void propertyCallback (AudioSessionPropertyID   inID,
                           UInt32                   inDataSize,
                           const void *             inPropertyValue) throw();
    
    void interruptionCallback (UInt32 inInterruption) throw();
        
private:
    void setFormat() throw();    
    int setupRemoteIO() throw();
    void restart() throw();
    void fixAudioRouteIfSetToReceiver() throw();

    AudioStreamBasicDescription format;
	AURenderCallbackStruct		inputProc;
	Float64						hwSampleRate;
	AudioUnit					rioUnit;
    
    int							bufferSize;
	float						bufferDuration;
	double						reciprocalBufferDuration;
	float						*floatBuffer;
	UInt32						audioInputIsAvailable;
	UInt32						numInputChannels;
	UInt32						numOutputChannels;
//	bool						isRunning;    
	double						cpuUsage;
    UInt32                      audioCategory;
};

END_PLONK_NAMESPACE

#ifdef __OBJC__

#define PLUNIT plonk::IOSAudioHost::UnitType

@protocol PLAudioGraph <NSObject>
@required
- (PLUNIT)constructGraph;
@end

@interface PLAudioHost : NSObject  
{
    void* peer;
    id<PLAudioGraph> delegate;
}

@property (nonatomic, retain) id delegate;
@property (nonatomic, readonly) NSString* hostName; 
@property (nonatomic, readonly) NSString* nativeHostName; 
@property (nonatomic, readonly) NSString* inputName;
@property (nonatomic, readonly) NSString* outputName; 
@property (nonatomic, readonly) double cpuUsage;
@property (nonatomic, readonly) BOOL isRunning;
@property (nonatomic, readonly) PLUNIT outputUnit;
@property (nonatomic) int numInputs;
@property (nonatomic) int numOutputs;
@property (nonatomic) int preferredBlockSize;
@property (nonatomic) double preferredSampleRate;

- (void)startHost;
- (void)stopHost;

@end

#endif


#endif  // PLONK_IOSAUDIOHOST_H
