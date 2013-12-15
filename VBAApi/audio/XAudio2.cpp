#ifndef NO_XAUDIO2

#include "../VBA/VBA.h"

// Interface
#include "../common/SoundDriver.h"

// XAudio2
#include <xaudio2.h>

#include <vector>
#include <string>

// Internals
#include "../System.h" // for systemMessage()
#include "../gba/Globals.h"

class XAudio2_Output;

// Synchronization Event
class XAudio2_BufferNotify : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;

	XAudio2_BufferNotify() {
		hBufferEndEvent = NULL;
		hBufferEndEvent = CreateEventEx( NULL, FALSE, FALSE, NULL );
		_ASSERT( hBufferEndEvent != NULL );
	}

	~XAudio2_BufferNotify() {
		CloseHandle( hBufferEndEvent );
		hBufferEndEvent = NULL;
	}

    STDMETHOD_( void, OnBufferEnd ) ( void *pBufferContext ) {
		_ASSERT( hBufferEndEvent != NULL );
		SetEvent( hBufferEndEvent );
	}


	// dummies:
	STDMETHOD_( void, OnVoiceProcessingPassStart ) ( UINT32 BytesRequired ) {}
	STDMETHOD_( void, OnVoiceProcessingPassEnd ) () {}
	STDMETHOD_( void, OnStreamEnd ) () {}
	STDMETHOD_( void, OnBufferStart ) ( void *pBufferContext ) {}
	STDMETHOD_( void, OnLoopEnd ) ( void *pBufferContext ) {}
	STDMETHOD_( void, OnVoiceError ) ( void *pBufferContext, HRESULT Error ) {};
};


// Class Declaration
class XAudio2_Output : public SoundDriver
{
public:
	XAudio2_Output();
	~XAudio2_Output();

	// Initialization
	bool init(long sampleRate);

	// Sound Data Feed
	void write(u16 * finalWave, int length);

	// Play Control
	void pause();
	void resume();
	void reset();
	void close();
	void device_change();

	// Configuration Changes
	void setThrottle( unsigned short throttle );

private:
	bool   failed;
	bool   initialized;
	bool   playing;
	UINT32 freq;
	UINT32 bufferCount;
	BYTE  *buffers;
	int    currentBuffer;
	int    soundBufferLen;

	volatile bool device_changed;

	IXAudio2               *xaud;
	IXAudio2MasteringVoice *mVoice; // listener
	IXAudio2SourceVoice    *sVoice; // sound source
	XAUDIO2_BUFFER          buf;
	XAUDIO2_VOICE_STATE     vState;
	XAudio2_BufferNotify    notify; // buffer end notification
};


// Class Implementation
XAudio2_Output::XAudio2_Output()
{
	failed = false;
	initialized = false;
	playing = false;
	freq = 0;
	bufferCount = 4;
	buffers = NULL;
	currentBuffer = 0;
	device_changed = false;

	xaud = NULL;
	mVoice = NULL;
	sVoice = NULL;
	ZeroMemory( &buf, sizeof( buf ) );
	ZeroMemory( &vState, sizeof( vState ) );	
}


XAudio2_Output::~XAudio2_Output()
{
	close();
}

void XAudio2_Output::close()
{
	initialized = false;

	if( sVoice ) 
	{
		if( playing ) 
		{
			HRESULT hr = sVoice->Stop( 0 );
			_ASSERT( hr == S_OK );
		}
		sVoice->DestroyVoice();
		sVoice = NULL;
	}

	if( buffers ) {
		free( buffers );
		buffers = NULL;
	}

	if( mVoice ) {
		mVoice->DestroyVoice();
		mVoice = NULL;
	}

	if( xaud ) {
		xaud->Release();
		xaud = NULL;
	}
}

void XAudio2_Output::device_change()
{
	device_changed = true;
}


bool XAudio2_Output::init(long sampleRate)
{
	if( failed || initialized ) return false;

	HRESULT hr;

	// Initialize XAudio2
	UINT32 flags = 0;
//#ifdef _DEBUG
//	flags = XAUDIO2_DEBUG_ENGINE;
//#endif

	hr = XAudio2Create( &xaud, flags );
	if( hr != S_OK ) {
		systemMessage( 0, "The XAudio2 interface failed to initialize!" );
		failed = true;
		return false;
	}


	freq = sampleRate;

	// calculate the number of samples per frame first
	// then multiply it with the size of a sample frame (16 bit * stereo)
	soundBufferLen = ( freq / 60 ) * 4;

	// create own buffers to store sound data because it must not be
	// manipulated while the voice plays from it
	buffers = (BYTE *)malloc( ( bufferCount + 1 ) * soundBufferLen );
	// + 1 because we need one temporary buffer when all others are in use

	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof( wfx ) );
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = freq;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = wfx.nChannels * ( wfx.wBitsPerSample / 8 );
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;


	// create sound receiver
	hr = xaud->CreateMasteringVoice(
		&mVoice,
		XAUDIO2_DEFAULT_CHANNELS,
		XAUDIO2_DEFAULT_SAMPLERATE,
		0,
		0,
		NULL );
	if( hr != S_OK ) {
		systemMessage( 0, "The XAudio2 interface failed to create master voice!" );
		failed = true;
		return false;
	}


	// create sound emitter
	hr = xaud->CreateSourceVoice( &sVoice, &wfx, 0, 4.0f, &notify );
	if( hr != S_OK ) {
		systemMessage( 0, "The XAudio2 interface failed to create source voice!" );
		failed = true;
		return false;
	}


	hr = sVoice->Start( 0 );
	_ASSERT( hr == S_OK );
	playing = true;

	currentBuffer = 0;
	device_changed = false;

	initialized = true;
	return true;
}


void XAudio2_Output::write(u16 * finalWave, int length)
{
	if( !initialized || failed ) return;

	while( true ) 
	{
		if ( device_changed ) 
		{
			close();
			if (!init(freq)) return;
		}

		sVoice->GetState( &vState );

		_ASSERT( vState.BuffersQueued <= bufferCount );

		if( vState.BuffersQueued < bufferCount ) 
		{
			if( vState.BuffersQueued == 0 ) 
			{
				// buffers ran dry
				if( systemVerbose & VERBOSE_SOUNDOUTPUT ) 
				{
					static unsigned int i = 0;
					log( "XAudio2: Buffers were not refilled fast enough (i=%i)\n", i++ );
				}
			}
			// there is at least one free buffer
			break;
		} 
		else 
		{
			
			// the maximum number of buffers is currently queued
			if( synchronize && !speedup && !theVBA.throttle ) 
			{
				/*
				// wait for one buffer to finish playing
				if (WaitForSingleObject( notify.hBufferEndEvent, 10000 ) == WAIT_TIMEOUT) 
				{
					device_changed = true;
				}
				*/
			} 
			else 
			{
				// drop current audio frame
				return;
			}
			
		}
	}

	// copy & protect the audio data in own memory area while playing it
	CopyMemory( &buffers[ currentBuffer * soundBufferLen ], finalWave, soundBufferLen );

	buf.AudioBytes = soundBufferLen;
	buf.pAudioData = &buffers[ currentBuffer * soundBufferLen ];

	currentBuffer++;
	currentBuffer %= ( bufferCount + 1 ); // + 1 because we need one temporary buffer

	HRESULT hr = sVoice->SubmitSourceBuffer( &buf ); // send buffer to queue
	_ASSERT( hr == S_OK );
}


void XAudio2_Output::pause()
{
	if( !initialized || failed ) return;

	if( playing ) {
		HRESULT hr = sVoice->Stop( 0 );
		_ASSERT( hr == S_OK );
		playing = false;
	}
}


void XAudio2_Output::resume()
{
	if( !initialized || failed ) return;

	if( !playing ) {
		HRESULT hr = sVoice->Start( 0 );
		_ASSERT( hr == S_OK );
		playing = true;
	}
}


void XAudio2_Output::reset()
{
	if( !initialized || failed ) return;

	if( playing ) {
		HRESULT hr = sVoice->Stop( 0 );
		_ASSERT( hr == S_OK );
	}

	sVoice->FlushSourceBuffers();
	sVoice->Start( 0 );
	playing = true;
}


void XAudio2_Output::setThrottle( unsigned short throttle )
{
	if( !initialized || failed ) return;

	if( throttle == 0 ) throttle = 100;
	HRESULT hr = sVoice->SetFrequencyRatio( (float)throttle / 100.0f );
	_ASSERT( hr == S_OK );
}

SoundDriver *newXAudio2_Output()
{
	return new XAudio2_Output();
}


#endif // #ifndef NO_XAUDIO2
