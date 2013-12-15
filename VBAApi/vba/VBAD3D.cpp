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

int RGB_LOW_BITS_MASK = 0;
extern int Init_2xSaI(u32); // initializes all pixel filters

void VBA::initForD3D()
{
	systemColorDepth = 32;
	// D3DFMT_X8R8G8B8 for screen display mode
	systemRedShift = 19;
	systemGreenShift = 11;
	systemBlueShift = 3;
	//DXGI_FORMAT_R8G8B8A8_UNORM
//	systemRedShift = 19;
//	systemGreenShift = 11;
//	systemBlueShift = 3;

	Init_2xSaI(32);
	
	utilUpdateSystemColorMaps(theVBA.cartridgeType == IMAGE_GBA && gbColorOption == 1);
}