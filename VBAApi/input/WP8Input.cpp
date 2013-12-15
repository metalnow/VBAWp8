#include "../vba/VBA.h"
#include "Input.h"

#define POV_UP    1
#define POV_DOWN  2
#define POV_RIGHT 4
#define POV_LEFT  8

class Wp8Input : public Input 
{
public:
    Wp8Input();
    virtual ~Wp8Input();

    virtual void checkDevices();
    virtual bool initialize();
    virtual bool readDevices();
    virtual u32 readDevice(int which);
    virtual void checkKeys();
    virtual void checkMotionKeys();
    virtual void activate();
	virtual void setKey( int which, int key, unsigned char pressed );
};

static int numDevices = 1;
static int axisNumber = 0;

Wp8Input::Wp8Input()
{
	memset( joypaddata, 0, JOYPADS * KEYS_PER_PAD + MOTION_KEYS );
}

Wp8Input::~Wp8Input()
{
}

bool Wp8Input::initialize()
{
	return true;
}

bool Wp8Input::readDevices()
{
    bool ok = true;
    return ok;
}

void Wp8Input::setKey( int which, int key, unsigned char pressed )
{
	joypaddata[JOYPAD(which,key)] = pressed;
}

u32 Wp8Input::readDevice(int which)
{
  u32 res = 0;
  int i = theVBA.joypadDefault;
  if(which >= 0 && which <= 3)
    i = which;

  if((joypaddata[JOYPAD(i,KEY_BUTTON_A)]))
    res |= 1;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_B)]))
    res |= 2;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_SELECT)]))
    res |= 4;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_START)]))
    res |= 8;
  if((joypaddata[JOYPAD(i,KEY_RIGHT)]))
    res |= 16;
  if((joypaddata[JOYPAD(i,KEY_LEFT)]))
    res |= 32;
  if((joypaddata[JOYPAD(i,KEY_UP)]))
    res |= 64;
  if((joypaddata[JOYPAD(i,KEY_DOWN)]))
    res |= 128;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_R)]))
    res |= 256;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_L)]))
    res |= 512;

  if((joypaddata[JOYPAD(i,KEY_BUTTON_GS)]))
    res |= 4096;

  if(theVBA.autoFire) {
    res &= (~theVBA.autoFire);
    if(theVBA.autoFireToggle)
      res |= theVBA.autoFire;
    theVBA.autoFireToggle = !theVBA.autoFireToggle;
  }

  // disallow L+R or U+D of being pressed at the same time
  if((res & 48) == 48)
    res &= ~16;
  if((res & 192) == 192)
    res &= ~128;

  // we don't record speed up or screen capture buttons
  if((joypaddata[JOYPAD(i,KEY_BUTTON_SPEED)]) || theVBA.speedupToggle)
    res |= 1024;
  if((joypaddata[JOYPAD(i,KEY_BUTTON_CAPTURE)]))
    res |= 2048;

  return res;
}

void Wp8Input::checkKeys()
{
}

void Wp8Input::checkMotionKeys()
{

}

Input *newWp8Input()
{
    return new Wp8Input;
}


void Wp8Input::checkDevices()
{
}

void Wp8Input::activate()
{
}
