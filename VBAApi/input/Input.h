#pragma once

#include "../System.h"
#include <vector>

#define JOYPADS 4
#define MOTION_KEYS 4
#define KEYS_PER_PAD 13
#define MOTION(i) ((JOYPADS*KEYS_PER_PAD)+i)
#define JOYPAD(i,j) ((i*KEYS_PER_PAD)+j)

#define DEVICEOF(key) (key >> 8)
#define KEYOF(key) (key & 255)

enum {
  KEY_LEFT, KEY_RIGHT,
  KEY_UP, KEY_DOWN,
  KEY_BUTTON_A, KEY_BUTTON_B,
  KEY_BUTTON_START, KEY_BUTTON_SELECT,
  KEY_BUTTON_L, KEY_BUTTON_R,
  KEY_BUTTON_SPEED, KEY_BUTTON_CAPTURE,
  KEY_BUTTON_GS
};

class Input {

 public:
  unsigned char joypaddata[JOYPADS * KEYS_PER_PAD + MOTION_KEYS];

  Input() {};
  virtual ~Input() {};

  virtual bool initialize() = 0;

  virtual bool readDevices() = 0;
  virtual u32 readDevice(int which) = 0;
  virtual void checkKeys() = 0;
  virtual void checkMotionKeys() = 0;
  virtual void checkDevices() = 0;
  virtual void activate() = 0;
  virtual void setKey( int which, int key, unsigned char pressed ) = 0;
};
