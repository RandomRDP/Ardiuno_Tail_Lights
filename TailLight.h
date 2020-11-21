#include "Arduino.h"
#include "CanSignal.h"
#include "FastLED.h"
#include "pin_defs.h"

#ifndef LED_h
#define LED_h

#define TOTAL_ANI    32
#define STARTUP_L   24
#define TRIGGER     25
#define NUM_LEDS    72 

#define BRAKE_ANI   25
#define SETTINGS_FLASH_ANI 31
#define SETTINGS_MENU_ANI 30


// TODO create animation parent class
//  child classes inhert attributes
//    uint8_t length - number of frames in animation
//  overwrites animation function
//    animation function sets LEDs on frame t


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
    bool * settings;
    uint8_t * opto;
    CanSignal * brakePressure;
    bool * brake;
    bool * indi;
    bool * hazard;
    
    int t = 0;
    CRGB leds[NUM_LEDS];

    void TailLight::Fin_Ani ();
    void TailLight::setPixel(int pixel, CRGB colour, CRGB leds[]);
    void TailLight::LoadEEProm(uint32_t* address);
    void TailLight::SaveEEProm(uint32_t* address);
    void TailLight::SetMenu(bool *menu);
    
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
    void TailLight::Flash ();
    void TailLight::Number ();
    void TailLight::Menu ();

    void TailLight::Ani_Func ();  
    void (TailLight::*Ani_Ptr)();
};

#endif
