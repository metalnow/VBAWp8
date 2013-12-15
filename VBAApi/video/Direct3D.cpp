#pragma once

#include "../vba/Display.h"

#include "../System.h"
#include "../gba/GBA.h"
#include "../gba/Globals.h"
#include "../Util.h"
#include "../gb/gbGlobals.h"

#include <memory.h>
#include <cassert>

#include <wrl/client.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;

extern int Init_2xSaI(u32); // initializes all pixel filters
extern int systemSpeed;


class Direct3DDisplay : public IDisplay 
{

public:
	Direct3DDisplay();
	virtual ~Direct3DDisplay();
	virtual DISPLAY_TYPE getType() { return DIRECT_3D; };

	virtual bool initialize( void * window );
	virtual void cleanup();
	virtual void clear();
	virtual void render();

	virtual bool changeRenderSize( int w, int h );
	virtual void resize( int w, int h );
	virtual void setOption( const char *option, int value );
	virtual bool selectFullScreenMode( VIDEO_MODE &mode );
};


Direct3DDisplay::Direct3DDisplay()
{

}


Direct3DDisplay::~Direct3DDisplay()
{
	cleanup();
}


void Direct3DDisplay::cleanup()
{
}


bool Direct3DDisplay::initialize( void * window )
{
	reinterpret_cast<IUnknown*>(window);
	return true;
}


void Direct3DDisplay::clear()
{
}


void Direct3DDisplay::render()
{

	return;
}


bool Direct3DDisplay::changeRenderSize( int w, int h )
{
	return true;
}


void Direct3DDisplay::resize( int w, int h )
{
}


bool Direct3DDisplay::selectFullScreenMode( VIDEO_MODE &mode )
{
	return true;
}

void Direct3DDisplay::setOption( const char *option, int value )
{

}

IDisplay *newDirect3DDisplay()
{
	return new Direct3DDisplay();
}
