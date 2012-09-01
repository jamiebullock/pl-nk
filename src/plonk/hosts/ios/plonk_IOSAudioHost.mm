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

#ifdef PLONK_AUDIOHOST_IOS

#include "../../core/plonk_StandardHeader.h"

BEGIN_PLONK_NAMESPACE

#include "../../core/plonk_Headers.h"

END_PLONK_NAMESPACE
#include "plonk_IOSAudioHost.h"
static const plonk::Text categoryKey    = "category";
static const plonk::Text modeKey        = "mode";
BEGIN_PLONK_NAMESPACE

//------------------------------------------------------------------------------

static inline OSStatus Render (void							*inRefCon, 
                               AudioUnitRenderActionFlags 	*ioActionFlags, 
                               const AudioTimeStamp         *inTimeStamp, 
                               UInt32 						inBusNumber, 
                               UInt32 						inNumberFrames, 
                               AudioBufferList				*ioData)
{	
	IOSAudioHost *host = (IOSAudioHost *)inRefCon;
	const OSStatus result =  host->renderCallback (inNumberFrames, 
                                                   ioActionFlags, 
                                                   inTimeStamp, 
                                                   ioData);
	return result;
}

static inline void PropertyListener (void *                  inClientData,
                                     AudioSessionPropertyID  inID,
                                     UInt32                  inDataSize,
                                     const void *            inPropertyValue)
{	
	IOSAudioHost *host = (IOSAudioHost *)inClientData;
    host->propertyCallback (inID, inDataSize, inPropertyValue);
}

static inline void InterruptionListener (void *inClientData, 
                                         UInt32 inInterruption)
{	
	IOSAudioHost *host = (IOSAudioHost *)inClientData;
    host->interruptionCallback (inInterruption);
}

static inline void audioFloatToShort (const float * const src, short* const dst, const unsigned int length) throw()
{
	static const float scale = 32767.f;
    pl_VectorMulF_N1N (src, scale, src, length);
    pl_VectorConvertF2S_NN (dst, src, length);
}

static inline void audioFloatToShortChannels (const float * const src[], AudioBufferList* const dst, const unsigned int length, const unsigned int numChannels) throw()
{
	for (UInt32 channel = 0; channel < numChannels; ++channel)
	{
		AudioSampleType* const audioUnitBuffer = (AudioSampleType*)dst->mBuffers[channel].mData;		
		audioFloatToShort (src[channel], audioUnitBuffer, length);
	}
}

static inline void audioShortToFloat (const short * const src, float* const dst, const unsigned int length) throw()
{
	static const float scale = 1.f / 32767.f;	
	pl_VectorConvertS2F_NN (dst, src, length);
    pl_VectorMulF_N1N (dst, scale, dst, length);
}

static inline void audioShortToFloatChannels (AudioBufferList* src, float* const dst[], const unsigned int length, const unsigned int numChannels) throw()
{
	for (UInt32 channel = 0; channel < numChannels; ++channel)
	{
		AudioSampleType* const audioUnitBuffer = (AudioSampleType*)src->mBuffers[0].mData; // need this other than 0?...		
		audioShortToFloat (audioUnitBuffer, dst[channel], length);
	}	
}

//------------------------------------------------------------------------------

IOSAudioHost::IOSAudioHost (ObjectMemoryBase* omb) throw()
:   AudioHostBase (omb),
    hwSampleRate (0.0),         // let the hardware choose
    cpuUsage (0.0)//,
//    audioCategory (kAudioSessionCategory_PlayAndRecord)
{    
}

IOSAudioHost::~IOSAudioHost()
{
    stopHost();
}

Text IOSAudioHost::getHostName() const throw()
{
    return "iOS";
}

Text IOSAudioHost::getNativeHostName() const throw()
{
    return "RemoteIO";
}

Text IOSAudioHost::getInputName() const throw()
{
    Text result = "Default Input";
    
    CFDictionaryRef dictionary = 0;
	UInt32 propertySize = sizeof (dictionary);
    
	if (AudioSessionGetProperty (kAudioSessionProperty_AudioRouteDescription, &propertySize, &dictionary) == noErr)
	{
        CFArrayRef array = (CFArrayRef)CFDictionaryGetValue (dictionary, kAudioSession_AudioRouteKey_Inputs);
        
        if (array != nil)
        {
            for (int i = 0; i < CFArrayGetCount (array); ++i)
            {  
                CFDictionaryRef dictionary2 = (CFDictionaryRef)CFArrayGetValueAtIndex (array, i);
                NSString* name = (NSString*)CFDictionaryGetValue (dictionary2, kAudioSession_AudioRouteKey_Type);
                result = [name UTF8String];
            }
        }
        
		CFRelease (dictionary);
	}
    
    return result;
}

Text IOSAudioHost::getOutputName() const throw()
{    
    Text result = "Default Output";
    
    CFDictionaryRef dictionary = 0;
	UInt32 propertySize = sizeof (dictionary);
    
	if (AudioSessionGetProperty (kAudioSessionProperty_AudioRouteDescription, &propertySize, &dictionary) == noErr)
	{
        CFArrayRef array = (CFArrayRef)CFDictionaryGetValue (dictionary, kAudioSession_AudioRouteKey_Outputs);
        
        if (array != nil)
        {
            for (int i = 0; i < CFArrayGetCount (array); ++i)
            {  
                CFDictionaryRef dictionary2 = (CFDictionaryRef)CFArrayGetValueAtIndex (array, i);
                NSString* name = (NSString*)CFDictionaryGetValue (dictionary2, kAudioSession_AudioRouteKey_Type);
                result = [name UTF8String];
            }
        }
        
		CFRelease (dictionary);
	}
    
    return result;
}

void IOSAudioHost::stopHost() throw()
{
    if (getIsRunning())
    {
        AudioOutputUnitStop (rioUnit);
        setIsRunning (false);
        hostStopped();
    }
}

void IOSAudioHost::startHost() throw()
{    
    UInt32 size;
    
	// render proc
	inputProc.inputProc = Render;
	inputProc.inputProcRefCon = this;
		
	// session
	AudioSessionInitialize (NULL, NULL, InterruptionListener, this);
	AudioSessionSetActive (true);
    
    UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord; 
        
    if (getOtherOptions().containsKey (categoryKey))
        audioCategory = (UInt32)getOtherOptions().at (categoryKey).asUnchecked<IntVariable>();
	
	AudioSessionSetProperty (kAudioSessionProperty_AudioCategory, sizeof (audioCategory), &audioCategory);
    
    if (getOtherOptions().containsKey (modeKey))
    {
        UInt32 mode = (UInt32)getOtherOptions().at (modeKey).asUnchecked<IntVariable>();
        AudioSessionSetProperty (kAudioSessionProperty_Mode, sizeof (mode), &mode);
    }
    
	AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, PropertyListener, this);
	fixAudioRouteIfSetToReceiver();
	
    const double preferredSampleRate = getPreferredSampleRate();
    size = sizeof (preferredSampleRate);
    
	if (preferredSampleRate > 0.0)
	{
		AudioSessionSetProperty (kAudioSessionProperty_PreferredHardwareSampleRate, size, &preferredSampleRate);
	}		
    
    size = sizeof (hwSampleRate);
    AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareSampleRate, &size, &hwSampleRate);
	
    if (getPreferredBlockSize() > 0)
    {
        bufferDuration = getPreferredBlockSize() / hwSampleRate;
        AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof (bufferDuration), &bufferDuration);
	}
    
	size = sizeof (UInt32);
	AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareInputNumberChannels, &size, &numInputChannels);
	AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareOutputNumberChannels, &size, &numOutputChannels);
	AudioSessionGetProperty (kAudioSessionProperty_AudioInputAvailable, &size, &audioInputIsAvailable);
    
    rioUnit = NULL;

    setNumInputs (numInputChannels);
    setNumOutputs (numOutputChannels);
    	
	size = sizeof (bufferDuration);
	AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareIOBufferDuration, &size, &bufferDuration);
	
	reciprocalBufferDuration = 1.0 / double (bufferDuration); 
	
	bufferSize = (int)(hwSampleRate * bufferDuration + 0.5);
	floatBuffer = new float[bufferSize * plonk::max (getNumOutputs(), getNumInputs())];
    
//	printf("IOSAudioHost: SR=%f buffer=%fs (%d samples)\n", hwSampleRate, bufferDuration, bufferSize);
    
    SampleRate::getDefault().setValue (hwSampleRate);
    BlockSize::getDefault().setValue (bufferSize); 
    startHostInternal();
    
    restart();
    
    size = sizeof (format);
	AudioUnitGetProperty (rioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &format, &size);
}


// need to call hostStopped() - may be in the property listener?

void IOSAudioHost::setFormat() throw()
{
	memset (&format, 0, sizeof (AudioStreamBasicDescription));
	format.mSampleRate = hwSampleRate;
	format.mFormatID = kAudioFormatLinearPCM;
	int sampleSize = sizeof (AudioSampleType);
	format.mFormatFlags = kAudioFormatFlagsCanonical;
	format.mBitsPerChannel = 8 * sampleSize;
	format.mChannelsPerFrame = getNumOutputs();
	format.mFramesPerPacket = 1;
	format.mBytesPerPacket = format.mBytesPerFrame = sampleSize;
	format.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;		
}

int IOSAudioHost::setupRemoteIO() throw()
{	
	// Open the output unit
	AudioComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	AudioComponent comp = AudioComponentFindNext (NULL, &desc);
	AudioComponentInstanceNew (comp, &rioUnit);
	
	const UInt32 one = 1;
	AudioUnitSetProperty (rioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &one, sizeof (one));	
	AudioUnitSetProperty (rioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &inputProc, sizeof (inputProc));
	
	AudioUnitSetProperty (rioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof (format));
	AudioUnitSetProperty (rioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &format, sizeof (format));
    
	AudioUnitInitialize (rioUnit);
	
	return 0;	
}

void IOSAudioHost::restart() throw()
{
    UInt32 size = sizeof(UInt32);
	AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareInputNumberChannels, &size, &numInputChannels);
	AudioSessionGetProperty (kAudioSessionProperty_CurrentHardwareOutputNumberChannels, &size, &numOutputChannels);
	AudioSessionGetProperty (kAudioSessionProperty_AudioInputAvailable, &size, &audioInputIsAvailable);
	
//	printf ("inputs=%d outputs=%d audioInputIsAvailable=%d\n", (int)numInputChannels, (int)numOutputChannels, (int)audioInputIsAvailable);
	
	if (rioUnit)	
        AudioComponentInstanceDispose (rioUnit);
        
    rioUnit = NULL;
        
    setFormat();
	setupRemoteIO();
	
	AudioSessionSetActive (true);
	AudioOutputUnitStart (rioUnit);
}

void IOSAudioHost::fixAudioRouteIfSetToReceiver() throw()
{
	CFStringRef audioRoute = 0;
	UInt32 propertySize = sizeof (audioRoute);
    
	if (AudioSessionGetProperty (kAudioSessionProperty_AudioRoute, &propertySize, &audioRoute) == noErr)
	{
		NSString* route = (NSString*)audioRoute;
        
		if ([route hasPrefix: @"Receiver"])
		{
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride);
		}
		
		CFRelease (audioRoute);
	}
}

OSStatus IOSAudioHost::renderCallback (UInt32                     inNumberFrames,
                                       AudioUnitRenderActionFlags *ioActionFlags, 
                                       const AudioTimeStamp       *inTimeStamp, 
                                       AudioBufferList            *ioData) throw()
{
    OSStatus err = 0;
    int i;
   
	double renderTime = CFAbsoluteTimeGetCurrent();
	
	if (inNumberFrames > bufferSize)
	{
		delete [] floatBuffer;
		bufferSize = inNumberFrames;
		
        // should use a FloatArray!!
		floatBuffer = new float[inNumberFrames * plonk::max (getNumInputs(), getNumOutputs())];
	}
	
    BlockSize::getDefault().setValue (inNumberFrames);
	
	float *floatBufferData[2];
	floatBufferData[0] = floatBuffer;
	floatBufferData[1] = floatBufferData[0] + inNumberFrames;	
    
	if (audioInputIsAvailable)
	{
		err = AudioUnitRender (rioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
		
//        if (err) 
//        { 
//            printf ("renderCallback: error %d %s\n", (int)err, (err == -10863) ? "(harmless)" : ""); 
//            return err; 
//        }
		
		audioShortToFloatChannels (ioData, floatBufferData, inNumberFrames, numInputChannels);				
	}
	else memset (floatBuffer, 0, numInputChannels * inNumberFrames * sizeof (float));
	        
    for (i = 0; i < numInputChannels; ++i)
        getInputs().atUnchecked (i) = floatBufferData[i];//.referTo (inNumberFrames, floatBufferData[i]);
    
    for (i = 0; i < numOutputChannels; ++i)
        getOutputs().atUnchecked (i) = floatBufferData[i];//.referTo (inNumberFrames, floatBufferData[i]);

    process();
        
	audioFloatToShortChannels (floatBufferData, ioData, inNumberFrames, ioData->mNumberBuffers);
    
	renderTime = CFAbsoluteTimeGetCurrent() - renderTime;
	
	const double timeRatio = renderTime * reciprocalBufferDuration;
	cpuUsage += 0.2 * (timeRatio - cpuUsage); 
	
	return err;	
}

void IOSAudioHost::propertyCallback (AudioSessionPropertyID inID,
                                     UInt32                 inDataSize,
                                     const void *           inPropertyValue) throw()
{
    if (getIsRunning()) return;
	
    if (inID == kAudioSessionProperty_AudioRouteChange)
    {
        if (inPropertyValue)
        {
            CFDictionaryRef routeChangeDictionary = (CFDictionaryRef)inPropertyValue;
            CFNumberRef routeChangeReasonRef = 
                (CFNumberRef)CFDictionaryGetValue (routeChangeDictionary, CFSTR (kAudioSession_AudioRouteChangeKey_Reason));
            
            SInt32 routeChangeReason;
            CFNumberGetValue (routeChangeReasonRef, kCFNumberSInt32Type, &routeChangeReason);
            
            CFStringRef newAudioRoute;
            UInt32 propertySize = sizeof (CFStringRef);
            AudioSessionGetProperty (kAudioSessionProperty_AudioRoute, &propertySize, &newAudioRoute);
            
            printf ("route=%s\n", CFStringGetCStringPtr (newAudioRoute, CFStringGetSystemEncoding()));
        }

        restart();
    }
}

void IOSAudioHost::interruptionCallback (UInt32 inInterruption) throw()
{
    if (inInterruption == kAudioSessionEndInterruption) 
    {
		// make sure we are again the active session
		AudioSessionSetActive(true);
		setIsRunning (true);
		AudioOutputUnitStart (rioUnit);
	}
}

//------------------------------------------------------------------------------


END_PLONK_NAMESPACE

#ifdef __OBJC__

BEGIN_PLONK_NAMESPACE

class IOAudioHostPeer : public IOSAudioHost
{
public:
    typedef IOSAudioHost::UnitType UnitType;
    
    IOAudioHostPeer (PLAudioHost* peerToUse)
    :   peer (peerToUse)
    {
    }
    
    UnitType constructGraph() throw()
    {
        plonk_assert (peer.delegate != nil);
        return [peer.delegate constructGraph:peer];
    }
    
    void hostStarting() throw()
    {
        if ([peer.delegate respondsToSelector:@selector(hostStarting:)])
            [peer.delegate hostStarting:peer];
    }
    
    void hostStopped() throw()
    {
        if ([peer.delegate respondsToSelector:@selector(hostStopped:)])
            [peer.delegate hostStopped:peer];
    }
    
private:
    PLAudioHost* peer; // no need to retain as the Obj-C peer owns this object
    
    IOAudioHostPeer();
};

END_PLONK_NAMESPACE

using namespace plonk;

#define PLPEERTYPE IOAudioHostPeer
#define PLPEER ((PLPEERTYPE*)peer)

@implementation PLAudioHost

@synthesize delegate;

- (id)init
{
    if (self = [super init])
    {
        peer = new PLPEERTYPE (self);
    }
    
    return self;
}

- (void)dealloc
{
    delete PLPEER;
    [super dealloc];
}

- (NSString*)hostName
{
    return [NSString stringWithUTF8String:PLPEER->getHostName().getArray()];
}

- (NSString*)nativeHostName
{
    return [NSString stringWithUTF8String:PLPEER->getNativeHostName().getArray()];
}

- (NSString*)inputName
{
    return [NSString stringWithUTF8String:PLPEER->getInputName().getArray()];
}

- (NSString*)outputName
{
    return [NSString stringWithUTF8String:PLPEER->getOutputName().getArray()];
}

- (double)cpuUsage
{
    return PLPEER->getCpuUsage();
}

- (BOOL)isRunning
{
    return PLPEER->getIsRunning() ? YES : NO;
}

- (PLUNIT)outputUnit
{
    return PLPEER->getOutputUnit();
}

- (int)numInputs
{
    return PLPEER->getNumInputs();
}

- (void)setNumInputs:(int)numInputs
{
    PLPEER->setNumInputs (numInputs);
}

- (int)numOutputs
{
    return PLPEER->getNumOutputs();
}

- (void)setNumOutputs:(int)numOutputs
{
    PLPEER->setNumOutputs (numOutputs);
}

- (int)preferredBlockSize
{
    return PLPEER->getPreferredBlockSize();
}

- (void)setPreferredBlockSize:(int)preferredBlockSize
{
    PLPEER->setPreferredBlockSize (preferredBlockSize);
}

- (double)preferredSampleRate
{
    return PLPEER->getPreferredSampleRate();
}

- (void)setPreferredSampleRate:(double)preferredSampleRate
{
    PLPEER->setPreferredSampleRate (preferredSampleRate);
}

- (UInt32)category
{    
    if (PLPEER->getOtherOptions().containsKey (categoryKey))
    {
        return (UInt32)PLPEER->getOtherOptions().at (categoryKey).asUnchecked<IntVariable>().getValue();
    }
    else 
    {
        UInt32 property;
        UInt32 size = sizeof (property); 
        AudioSessionGetProperty (kAudioSessionProperty_AudioCategory, &size, &property);
        return property;
    }
}

- (void)setCategory:(UInt32)category
{
    PLPEER->getOtherOptions().at (categoryKey) = IntVariable ((int)category);
}

- (UInt32)mode
{    
    if (PLPEER->getOtherOptions().containsKey (categoryKey))
    {
        return (UInt32)PLPEER->getOtherOptions().at (modeKey).asUnchecked<IntVariable>().getValue();
    }
    else 
    {
        UInt32 property;
        UInt32 size = sizeof (property); 
        AudioSessionGetProperty (kAudioSessionProperty_Mode, &size, &property);
        return property;
    }
}

- (void)setMode:(UInt32)mode
{
    PLPEER->getOtherOptions().at (modeKey) = IntVariable ((int)mode);
}

- (void)startHost
{
    PLPEER->startHost();
}

- (void)stopHost
{
    PLPEER->stopHost();
}

@end

#undef PLPEERTYPE
#undef PLPEER

#endif


#endif // PLONK_AUDIOHOST_IOS

