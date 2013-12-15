#include "VBA.h"
#include <intrin.h>
#include <windows.h>

#include "../System.h"
#include "../AutoBuild.h"
#include "../gba/cheatSearch.h"
#include "../gba/GBA.h"
#include "../gba/Globals.h"
#include "../gba/Flash.h"
#include "../gba/Globals.h"
#include "../gb/GB.h"
#include "../gb/gbSound.h"
#include "../gb/gbCheats.h"
#include "../gb/gbGlobals.h"
#include "../gba/RTC.h"
#include "../gba/Sound.h"
#include "../Util.h"
#include "../gba/GBALink.h"
#include "../common/Patch.h"
#include "../common/SoundDriver.h"

#include <thread>

namespace Sm60FPS
{
	float					K_fCpuSpeed = 100.0f; // was 98.0f before, but why?
	float					K_fTargetFps = 60.0f * K_fCpuSpeed / 100;
	float					K_fDT = 1000.0f / K_fTargetFps;

	u32					dwTimeElapse;
	u32					dwTime0;
	u32					dwTime1;
	u32					nFrameCnt;
	float					fWantFPS;
	float					fCurFPS;
	bool					bLastSkip;
	int					nCurSpeed;
	int					bSaveMoreCPU;
};


extern SoundDriver *newXAudio2_Output();

u16 systemColorMap16[0x10000];
u32 systemColorMap32[0x10000];
u16 systemGbPalette[24];
int systemRedShift = 0;
int systemGreenShift = 0;
int systemBlueShift = 0;
int systemColorDepth = 32;
int systemDebug = 0;
int systemVerbose = 0;
int systemFrameSkip = 0;
int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED ;
int systemSpeed = 0;

void winSignal(int,int);
void winOutput(const char *, u32);

void (*dbgSignal)(int,int) = winSignal;
void (*dbgOutput)(const char *, u32) = winOutput;

u32 systemGetClock()
{
	if ( GetTickCount64() > 4294967296 )
		return 0;
	return (u32)GetTickCount64();
}

void log(const char * msg,...)
{
	char buffer[256];
	va_list args;
	va_start (args, msg);
	vsprintf (buffer,msg, args);
	OutputDebugString(buffer);
	va_end (args);
}

bool systemPauseOnFrame()
{
	return false;
}

void systemGbPrint(u8 *,int,int,int,int,int)
{
}

void systemScreenCapture(int)
{
}

void systemDrawScreen()
{
	if ( Sm60FPS_CanSkipFrame() )
	{
		return;
	}

	if ( theVBA.display )
	{
		theVBA.display->render();
		Sm60FPS_Sleep();
	}

	if ( theVBA.displaySP )
	{
		theVBA.displaySP->render();
		Sm60FPS_Sleep();
	}
}

// updates the joystick data
bool systemReadJoypads()
{
	return theVBA.input->readDevices();
}
// return information about the given joystick, -1 for default joystick
u32 systemReadJoypad( int which )
{
	which = which > 0 && which < 4 ? which : theVBA.joypadDefault;
	return theVBA.input->readDevice( which );
}

void systemMessage(int, const char *, ...)
{
}

void systemSetTitle(const char *)
{
}

SoundDriver * systemSoundInit()
{
	SoundDriver * drv = 0;
	soundShutdown();

	drv = newXAudio2_Output();

	if( drv ) 
	{
		if ( theVBA.throttle )
			drv->setThrottle( theVBA.throttle );
	}

	return drv;
}

void systemOnWriteDataToSoundBuffer(const u16 * finalWave, int length)
{
}

void systemOnSoundShutdown()
{
}

void systemScreenMessage(const char *)
{
}

void systemUpdateMotionSensor()
{
}

int  systemGetSensorX()
{
	return 0;
}

int  systemGetSensorY()
{
	return 0;
}

bool systemCanChangeSoundQuality()
{
	return false;
}

void systemShowSpeed(int)
{
}

void system10Frames(int)
{
}

void systemFrame()
{
}

void systemGbBorderOn()
{
}

void Sm60FPS_Init()
{
	Sm60FPS::dwTimeElapse = 0;
	Sm60FPS::fWantFPS = 60.f;
	Sm60FPS::fCurFPS = 0.f;
	Sm60FPS::nFrameCnt = 0;
	Sm60FPS::bLastSkip = false;
	Sm60FPS::nCurSpeed = 100;
}

void Sm60FPS_SetCUPSpeed( float speed )
{
	Sm60FPS::K_fCpuSpeed = speed;
	Sm60FPS::K_fTargetFps = 60.0f * Sm60FPS::K_fCpuSpeed / 100;
	Sm60FPS::K_fDT = 1000.0f / Sm60FPS::K_fTargetFps;
}

bool Sm60FPS_CanSkipFrame()
{
  if( theVBA.autoFrameSkip ) {
	  if( Sm60FPS::nFrameCnt == 0 ) {
		  Sm60FPS::nFrameCnt = 0;
		  Sm60FPS::dwTimeElapse = 0;
		  Sm60FPS::dwTime0 = systemGetClock();
	  } else {
		  if( Sm60FPS::nFrameCnt >= 10 ) {
			  Sm60FPS::nFrameCnt = 0;
			  Sm60FPS::dwTimeElapse = 0;

			  if( Sm60FPS::nCurSpeed > Sm60FPS::K_fCpuSpeed ) {
				  Sm60FPS::fWantFPS += 1;
				  if( Sm60FPS::fWantFPS > Sm60FPS::K_fTargetFps ){
					  Sm60FPS::fWantFPS = Sm60FPS::K_fTargetFps;
				  }
			  } else {
				  if( Sm60FPS::nCurSpeed < (Sm60FPS::K_fCpuSpeed - 5) ) {
					  Sm60FPS::fWantFPS -= 1;
					  if( Sm60FPS::fWantFPS < 30.f ) {
						  Sm60FPS::fWantFPS = 30.f;
					  }
				  }
			  }
		  } else { // between frame 1-10
			  Sm60FPS::dwTime1 = systemGetClock();
			  Sm60FPS::dwTimeElapse += (Sm60FPS::dwTime1 - Sm60FPS::dwTime0);
			  Sm60FPS::dwTime0 = Sm60FPS::dwTime1;
			  if( !Sm60FPS::bLastSkip &&
				  ( (Sm60FPS::fWantFPS < Sm60FPS::K_fTargetFps) || Sm60FPS::bSaveMoreCPU) ) {
					  Sm60FPS::fCurFPS = (float)Sm60FPS::nFrameCnt * 1000 / Sm60FPS::dwTimeElapse;
					  if( (Sm60FPS::fCurFPS < Sm60FPS::K_fTargetFps) || Sm60FPS::bSaveMoreCPU ) {
						  Sm60FPS::bLastSkip = true;
						  Sm60FPS::nFrameCnt++;
						  return true;
					  }
			  }
		  }
	  }
	  Sm60FPS::bLastSkip = false;
	  Sm60FPS::nFrameCnt++;
  }
  return false;
}

void Sm60FPS_Sleep()
{
	if( theVBA.autoFrameSkip ) {
		u32 dwTimePass = Sm60FPS::dwTimeElapse + (systemGetClock() - Sm60FPS::dwTime0);
		u32 dwTimeShould = (u32)(Sm60FPS::nFrameCnt * Sm60FPS::K_fDT);
		if( dwTimeShould > dwTimePass ) 
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((dwTimeShould - dwTimePass)));
			//Sleep(dwTimeShould - dwTimePass);
		}
	}
}

void DbgMsg(const char *msg, ...)
{
	char buffer[256];
	va_list args;
	va_start (args, msg);
	vsprintf (buffer,msg, args);
	OutputDebugString(buffer);
	va_end (args);
}

void winlog(const char *msg,...)
{
	char buffer[256];
	va_list args;
	va_start (args, msg);
	vsprintf (buffer,msg, args);
	OutputDebugString(buffer);
	va_end (args);
}

void winSignal(int, int)
{
}

#define CPUReadByteQuick(addr) \
  map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]

void winOutput(const char *s, u32 addr)
{
}