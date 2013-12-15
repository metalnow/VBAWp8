#pragma once

#include "../System.h"
#include "../Util.h"
#include <string>
#include "Display.h"
#include "../input/Input.h"

using namespace std;

class VBA
{
public:
	VBA();
	~VBA();

	bool FileRun( const char * filePath );
	void render();
	void updateThrottle( unsigned short throttle );
	void initForD3D();
	void setSoundVolume(float volume);
	void setAVSync( bool sync, unsigned short throttle );
	void setTurboMode( bool turbo );

	struct EmulatedSystem emulator;
	IMAGE_TYPE cartridgeType;

	bool d3dMotionBlur;

	string filename;
	string filePathName;

	bool useBiosFileGBA;
	bool useBiosFileGBC;
	bool useBiosFileGB;
	bool skipBiosFile;
	string biosFileNameGBA;
	string biosFileNameGBC;
	string biosFileNameGB;

	bool winGbBorderOn;

	int winFlashSize;
	bool winRtcEnable;

	bool mirroring;

	bool soundInitialized;

	int frameskipadjust;
	int renderedFrames;
	u32 autoFrameSkipLastTime;
	bool autoFrameSkip;

	int throttle;

	int sizeX;
	int sizeY;

	IDisplay * display;
	IDisplaySP * displaySP;

	Input * input;
	int joypadDefault;
	bool speedupToggle;
    bool autoFireToggle;
    int autoFire;

private:
	void readBatteryFile();
	void writeBatteryFile();
	void updateFrameSkip();
	void updateWindowSize();
};

extern VBA theVBA;
extern int emulating;