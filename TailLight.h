#include "Arduino.h"
#include "FastLED.h"
#include "pin_defs.h"

#ifndef LED_h
#define LED_h

#define TOTAL_ANI    32
#define STARTUP_L   24
#define TRIGGER     25
#define NUM_LEDS    72 

typedef struct ANIMATION{
  uint8_t ani;
  CRGB before = CRGB::Black;
  CRGB after = CRGB::Red;
  uint8_t speed = 1;
  uint8_t eyesize = 10;
} ANIMATION;


class TailLight{
  public:
    TailLight();

    ANIMATION Ani;
    ANIMATION Ans[TOTAL_ANI];
    uint8_t startup_idx = 0;
    int t = 0;
    CRGB leds[NUM_LEDS];

    void TailLight::Fin_Ani ();
    void TailLight::setPixel(int pixel, CRGB colour, CRGB leds[]);
    void TailLight::LoadEEProm(uint32_t* address);
    void TailLight::SaveEEProm(uint32_t* address);
    
    void TailLight::Set_Ani();

    void TailLight::Blank_();
    void TailLight::CenterToOutside ();
    void TailLight::OutsideToCenter ();
    void TailLight::Dim ();
    void TailLight::LeftToRight ();
    void TailLight::RightToLeft ();
    void TailLight::LeftFill ();
    void TailLight::RightFill ();
    void TailLight::FillTwo ();
    void TailLight::EmptyTwo ();
    void TailLight::Full ();

    void TailLight::Ani_Func ();  
    void (TailLight::*Ani_Ptr)();
};

#endif
