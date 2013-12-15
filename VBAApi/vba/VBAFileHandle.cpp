#include "VBA.h"
#include <intrin.h>

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

bool VBA::FileRun( const char * filePath )
{
	this->filePathName = filePath;

	// save battery file before we change the filename...
	if ( rom != NULL || gbRom != NULL ) 
	{
		//if (theVBA.autoSaveLoadCheatList)
		//	winSaveCheatListDefault();
		writeBatteryFile();
		//cheatSearchCleanup(&cheatSearchData);

		this->emulator.emuCleanUp();
		emulating = false;
	}
	char tempName[2048];
	char file[2048];
	string oldFile = this->filename;

	utilStripDoubleExtension( this->filePathName.c_str(), tempName);

	_fullpath(file, tempName, 2048);
	this->filename = file;

	int index = this->filename.rfind('.');
	if ( index !=  string::npos )
		this->filename = this->filename.substr( 0, index );

	if( this->filename != oldFile ) 
	{
		// clear cheat list when another game is loaded
		cheatsDeleteAll( false );
		gbCheatRemoveAll();
	}

	IMAGE_TYPE type = utilFindType( this->filePathName.c_str() );
	if ( type == IMAGE_UNKNOWN ) 
	{
		return false;
	}
	systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
	theVBA.cartridgeType = type;


	if(type == IMAGE_GB) 
	{
		if( !gbLoadRom( theVBA.filePathName.c_str() ) )
			return false;

		gbGetHardwareType();

		// used for the handling of the gb Boot Rom
		if ( gbHardware & 5 )
		{
			skipBios = theVBA.skipBiosFile;
			gbCPUInit( theVBA.biosFileNameGB.c_str(), theVBA.useBiosFileGB );
		}

		gbReset();
		theVBA.emulator = GBSystem;
		gbBorderOn = theVBA.winGbBorderOn;


	} 
	else 
	{
		int size = CPULoadRom( theVBA.filePathName.c_str() );
		if( !size )
			return false;

		flashSetSize( theVBA.winFlashSize );
		rtcEnable( theVBA.winRtcEnable );
		cpuSaveType = 0; // zero for auto

		doMirroring( theVBA.mirroring );

		theVBA.emulator = GBASystem;
		/* disabled due to problems
		if(theVBA.removeIntros && rom != NULL) {
			*((u32 *)rom)= 0xea00002e;
		}
		*/

	}

	if ( theVBA.soundInitialized ) 
	{
		if( theVBA.cartridgeType == 1 )
			gbSoundReset();
		else
			soundReset();
	} 
	else 
	{
		soundInit();
		theVBA.soundInitialized = true;
	}

	if( type == IMAGE_GBA )
	{
		skipBios = theVBA.skipBiosFile;
		CPUInit( theVBA.biosFileNameGBA.c_str(), theVBA.useBiosFileGBA );
		CPUReset();
	}

	readBatteryFile();

	/*
	if ( theVBA.autoSaveLoadCheatList )
		winLoadCheatListDefault();
	*/

	updateWindowSize();

	updateFrameSkip();

	emulating = true;

	/*
	if ( theVBA.autoLoadMostRecent )
		OnFileLoadgameMostrecent();
	*/

	frameskipadjust = 0;
	renderedFrames = 0;
	autoFrameSkipLastTime = systemGetClock();

	return true;
}

void VBA::writeBatteryFile()
{
	string filename = theVBA.filename + ".sav";

	bool res = false;
	if( theVBA.emulator.emuWriteBattery )
		res = theVBA.emulator.emuWriteBattery( filename.c_str() );
}

void VBA::readBatteryFile()
{
	string filename = theVBA.filename + ".sav";

	bool res = false;
	if( theVBA.emulator.emuReadBattery )
		res = theVBA.emulator.emuReadBattery( filename.c_str() );
}