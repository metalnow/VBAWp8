#include "VBA.h"
#include <intrin.h>

#include "../System.h"
#include "../gba/agbprint.h"
#include "../gba/cheatSearch.h"
#include "../gba/GBA.h"
#include "../gba/Globals.h"
#include "../gba/RTC.h"
#include "../gba/Sound.h"
#include "../Util.h"
#include "../gb/gbGlobals.h"
#include "../gb/gbPrinter.h"
#include "../gb/gbSound.h"
#include "../common/SoundDriver.h"

#include "../version.h"

extern IDisplay * newDirect3DDisplay();
extern Input * newWp8Input();

VBA theVBA;
int emulating = 0;

VBA::VBA() :
	d3dMotionBlur(false)
{
	useBiosFileGBA = false;
	useBiosFileGBC = false;
	useBiosFileGB = false;
	skipBiosFile = false;
	winGbBorderOn = false;

	winFlashSize = 0x10000;

	winRtcEnable = false;

	mirroring = false;

	soundInitialized = false;

	frameskipadjust = 0;
	renderedFrames = 0;
	autoFrameSkipLastTime = 0;
	autoFrameSkip = false;

	throttle = 0;
	cartridgeType = IMAGE_GBA;

	display = newDirect3DDisplay();
	switch( cartridgeType )
	{
	case IMAGE_GBA:
		sizeX = 240;
		sizeY = 160;
		break;
	case IMAGE_GB:
		if( gbBorderOn ) 
		{
			sizeX = 256;
			sizeY = 224;
		} 
		else 
		{
			sizeX = 160;
			sizeY = 144;
		}
		break;
	}

	joypadDefault = 0;
	speedupToggle = false;
    autoFireToggle = false;
    autoFire = 0;

	input = newWp8Input();

	cheatsEnabled = false;

	Sm60FPS_Init();
	displaySP = 0;
}

VBA::~VBA()
{
	soundPause();
	soundShutdown();

	if(gbRom != NULL || rom != NULL) 
	{
		//if(autoSaveLoadCheatList)
		//	winSaveCheatListDefault();
		writeBatteryFile();
		cheatSearchCleanup(&cheatSearchData);
		emulator.emuCleanUp();
	}
}

void VBA::setSoundVolume(float volume)
{
	soundSetVolume(volume);
}

void VBA::updateWindowSize()
{

  sizeX = 240;
  sizeY = 160;

  if(cartridgeType == IMAGE_GB) {
    if(gbBorderOn) {
      sizeX = 256;
      sizeY = 224;
      gbBorderLineSkip = 256;
      gbBorderColumnSkip = 48;
      gbBorderRowSkip = 40;
    } else {
      sizeX = 160;
      sizeY = 144;
      gbBorderLineSkip = 160;
      gbBorderColumnSkip = 0;
      gbBorderRowSkip = 0;
    }
  }

}

void VBA::updateFrameSkip()
{
	switch(cartridgeType) 
	{
	case 0:
		systemFrameSkip = frameSkip;
	break;
	case 1:
		systemFrameSkip = gbFrameSkip;
	break;
	}
}

void VBA::render()
{
	if ( emulating )
	{
		emulator.emuMain(emulator.emuCount);
	}
}

void VBA::setAVSync( bool sync, unsigned short throttle )
{
	synchronize = sync;
	this->updateThrottle(throttle);
}

void VBA::updateThrottle( unsigned short throttle )
{
	this->throttle = throttle;

	if( throttle ) 
	{
		Sm60FPS_SetCUPSpeed((float)throttle);
		autoFrameSkip = false;
		frameSkip = 0;
		systemFrameSkip = 0;
	}

	soundSetThrottle(throttle);
}

void VBA::setTurboMode( bool turbo )
{
	this->speedupToggle = turbo;
}