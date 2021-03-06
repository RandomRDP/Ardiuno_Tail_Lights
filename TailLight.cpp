#include "TailLight.h"
#include "Arduino.h"
#include "FastLED.h"
#include <EEPROM.h>


TailLight::TailLight(){
  
}

void TailLight::Fin_Ani(){
  if (startup_idx < STARTUP_L){
    startup_idx++; 
    if (Ans[startup_idx].ani != 255) {
      Ani.ani = Ans[startup_idx].ani;
      Ani.before = Ans[startup_idx].before;
      Ani.after  = Ans[startup_idx].after;
      Ani.speed = Ans[startup_idx].speed;
      Ani.eyesize = Ans[startup_idx].eyesize;
    } else {
      startup_idx = STARTUP_L;
      Fin_Ani();
    }
    t = 0;
  } else if (*settings == true) {
    Ani.ani = Ans[SETTINGS_MENU_ANI].ani;
    Ani.before = Ans[SETTINGS_MENU_ANI].before;
    Ani.after  = Ans[SETTINGS_MENU_ANI].after;
    Ani.speed = Ans[SETTINGS_MENU_ANI].speed;
    Ani.eyesize = Ans[SETTINGS_MENU_ANI].eyesize;
  } else if (*brake == true) {
    Ani.ani = Ans[BRAKE_ANI].ani;
    Ani.before = Ans[BRAKE_ANI].before;
    Ani.after  = Ans[BRAKE_ANI].after;
    Ani.speed = Ans[BRAKE_ANI].speed;
    Ani.eyesize = Ans[BRAKE_ANI].eyesize;
  } else {
    Ani.ani = 255;
    Ani.before = CRGB::Black;
    Ani.after  = CRGB::Red;
    Ani.speed = 1;
    Ani.eyesize = 10;
    t = 0;
  }  
  Set_Ani();
}


void TailLight::setPixel(int pixel, CRGB colour, CRGB leds[]) {
  if ( pixel < NUM_LEDS && pixel >= 0){
    leds[pixel].r = colour.red;
    leds[pixel].g = colour.green;
    leds[pixel].b = colour.blue;
  } else {
    //Serial.print("sumthing don fucked up:");
    //Serial.println(pixel);
  }
}

void TailLight::LoadEEProm(uint32_t * address){
  for (int i = 0; i < TOTAL_ANI; i++){
    Ans[i].ani = EEPROM.read(*address);
    *address += 1;
    Ans[i].before.r = EEPROM.read(*address);
    *address += 1;
    Ans[i].before.g = EEPROM.read(*address);
    *address += 1;
    Ans[i].before.b = EEPROM.read(*address);
    *address += 1;
    Ans[i].after.r = EEPROM.read(*address);
    *address += 1;
    Ans[i].after.g = EEPROM.read(*address);
    *address += 1;
    Ans[i].after.b = EEPROM.read(*address);
    *address += 1;
    EEPROM.get(*address, Ans[i].speed);
    *address += 1;
    EEPROM.get(*address, Ans[i].eyesize);
    *address += 1;
  }
}

void TailLight::SaveEEProm(uint32_t * address){
  uint8_t uint8;
  byte Colour;
  for (int i = 0; i < TOTAL_ANI; i++){
    EEPROM.get(*address, uint8);
    if (uint8 != Ans[i].ani){
      EEPROM.put(*address, Ans[i].ani);
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].before.r ){
      EEPROM.put(*address, Ans[i].before.r );
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].before.g ){
      EEPROM.put(*address, Ans[i].before.g );
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].before.b ){
      EEPROM.put(*address, Ans[i].before.b );
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].after.r ){
      EEPROM.put(*address, Ans[i].after.r );
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].after.g ){
      EEPROM.put(*address, Ans[i].after.g );
    }
    *address += 1;
    EEPROM.get(*address, Colour);
    if (Colour != Ans[i].after.b ){
      EEPROM.put(*address, Ans[i].after.b );
    }
    *address += 1;
    EEPROM.get(*address, uint8);
    if (uint8 != Ans[i].speed ){
      EEPROM.put(*address, Ans[i].speed );
    }
    *address += 1;
    EEPROM.get(*address, uint8);
    if (uint8 != Ans[i].speed ){
      EEPROM.put(*address, Ans[i].eyesize );
    }
    *address += 1;
  }
}

void TailLight::Set_Ani(){
  switch (Ani.ani){
    case 0: 
      Ani_Ptr = &CenterToOutside;
      break;
    case 1:
      Ani_Ptr = &OutsideToCenter;
      break;
    case 2:
      Ani_Ptr = &LeftToRight;
      break;
    case 3:
      Ani_Ptr = &RightToLeft;
      break;
    case 4:
      Ani_Ptr = &Dim;
      break;
    case 5:
      Ani_Ptr = &LeftFill;
      break;
    case 6:
      Ani_Ptr = &RightFill;
      break;
    case 7:
      Ani_Ptr = &FillTwo;
      break;
    case 8:
      Ani_Ptr = &EmptyTwo;
      break;
    case 9:
      Ani_Ptr = &Flash;
      break;
    case 10:
      Ani_Ptr = &Number;
      break;
    case 11:
      Ani_Ptr = &Menu;
      break;
    default:
      Ani_Ptr = &Blank_;
      break;
  }
}

void TailLight::Ani_Func(){
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = Ani.before;
  }
  
  (this->*Ani_Ptr)();
  if (startup_idx < STARTUP_L || *settings == true){
    t++;
  } else if (*brake == true){
    t = map(brakePressure->signal.u, brakePressure->min.u, brakePressure->max.u, 0, (NUM_LEDS / 2 ) + 1);
  }
}
 
void TailLight::SetMenu(bool *menu){
  brake = &menu[2];
  indi = &menu[3];
  hazard = &menu[4];
}

void TailLight::Blank_(){
  t = 0;
}

void TailLight::CenterToOutside() {
  int i = (((NUM_LEDS - Ani.eyesize)/2) - (t / Ani.speed));
  
  setPixel(i, Ani.after, leds);
  for (int j = 1; j <= Ani.eyesize; j++) {
    setPixel(i + j, Ani.after, leds);
  }
  setPixel(i + Ani.eyesize + 1, Ani.after, leds);

  setPixel(NUM_LEDS - i, Ani.after, leds);
  for (int j = 1; j <= Ani.eyesize; j++) {
    setPixel(NUM_LEDS - i - j, Ani.after, leds);
  }
  setPixel(NUM_LEDS - i - Ani.eyesize - 1, Ani.after, leds);
  if ( i < 1) {
    Fin_Ani();
  }
}

void TailLight::OutsideToCenter() {
  int i = (t / Ani.speed);

  setPixel(i, Ani.after, leds);
  for (int j = 1; j <= Ani.eyesize; j++) {
    setPixel(i + j, Ani.after, leds);
  }
  setPixel(i + Ani.eyesize + 1, Ani.after, leds);

  setPixel(NUM_LEDS - i, Ani.after, leds);
  for (int j = 1; j <= Ani.eyesize; j++) {
    setPixel(NUM_LEDS - i - j, Ani.after, leds);
  }
  setPixel(NUM_LEDS - i - Ani.eyesize - 1, Ani.after, leds);
  if ( i > ((NUM_LEDS - Ani.eyesize)/2)) {
    Fin_Ani();
  }
}

void TailLight::LeftToRight() {
  int i = (t / Ani.speed);

  for (int j = 0; j <= Ani.eyesize + 1; j++) {
    setPixel(i + j, Ani.after, leds);
  }

  if ( i > NUM_LEDS - Ani.eyesize - 2) {
    Fin_Ani();
  }
  
}

void TailLight::RightToLeft() {
  int i = NUM_LEDS - Ani.eyesize - 2 - (t / Ani.speed);

  for (int j = 0; j <= Ani.eyesize + 1; j++) {
    setPixel(i + j, Ani.after, leds);
  }

  if ( i < 0) {
    Fin_Ani();
  }
}

void TailLight::Dim() {
  int i = (t / Ani.speed);
  uint8_t brightness = map( i, 0, 64, Ani.before.r, Ani.after.r);
  CRGB colour = CRGB(brightness,0,0);

  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = colour;
  }
  if ( i > 64) {
    Fin_Ani();
  }
}

void TailLight::LeftFill() {
  int i = (t / Ani.speed);

  for (int j = 0; j < i; j++){
    setPixel(j, Ani.after, leds);
  }
  if ( i > NUM_LEDS) {
    Fin_Ani();
  }
}

void TailLight::RightFill() {
  int i = NUM_LEDS - (t / Ani.speed);

  for (int j = NUM_LEDS; j > i; j--){
    setPixel(j, Ani.after, leds);
  }
  if ( i < 0) {
    Fin_Ani();
  }
}


void TailLight::FillTwo() {
  int i = (t / Ani.speed);

  for (int j = 0; j < i; j++){
    setPixel(j, Ani.after, leds);
  }
  
  for (int j = NUM_LEDS - 1; j > NUM_LEDS - i - 1; j--){
    setPixel(j, Ani.after, leds);
  }

  if ( i == (NUM_LEDS / 2 ) + 2) {
    Fin_Ani();
  }
}

void TailLight::EmptyTwo() {
  int i = (NUM_LEDS/2) - (t / Ani.speed) + 1;

  for (int j = 0; j < i; j++){
    setPixel(j, Ani.after, leds);
  }
  
  for (int j = NUM_LEDS - 1; j > NUM_LEDS - i - 1; j--){
    setPixel(j, Ani.after, leds);
  }

  if ( i < 0) {
    Fin_Ani();
  }
}

void TailLight::Flash() {
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = Ani.after;
  }
  if ( t > Ani.speed ) {
    Fin_Ani();
    t = 0;
  }
}

void TailLight::Number() {
  uint8_t i = 0;
  for (uint8_t j = 0; j < Ani.speed; j++){
    for (uint8_t k = 0; k < Ani.eyesize; k++){
      setPixel(i, Ani.after, leds);
      i++;     
    }
    i += Ani.eyesize;
  }
  Fin_Ani();             
}

void TailLight::Menu() {
  uint8_t i = 0;
  CRGB colour = CRGB(((t / Ani.speed) % 2) * 255,0,0);
  
  if ((t / Ani.speed) == 2){
    t = 0; 
  }
  
  for (uint8_t j = 0; j < 5; j++){
    for (uint8_t k = 0; k < NUM_LEDS/9; k++){
      setPixel(i, Ani.after, leds);
      i++;     
    }
    i += (NUM_LEDS/9);
  }

  for (uint8_t k = Ani.eyesize * 2 * (NUM_LEDS/9); k < (Ani.eyesize * 2 * (NUM_LEDS/9)) + (NUM_LEDS/9); k++){
    setPixel(k, colour, leds);
    i++;     
  } 

  Fin_Ani();             
}
  

//void TailLight::FillRight_t(byte red, byte green, byte blue, int EyeSize, int * tie, CRGB leds[], bool side) {
//  int i = NUM_LEDS - *tie;
//
//  for (int j = NUM_LEDS - 1; j > i; j--){
//    setPixel(j, red, green, blue, leds);
//  }
//  if ( i < 0) {
//    Set_Blank(tie, side);
//  }
//}
//
//void TailLight::EmptyLeft_t(byte red, byte green, byte blue, int EyeSize, int * tie, CRGB leds[], bool side) {
//  int i = NUM_LEDS - *tie;
//  
//  for (int j = 0; j < i; j++){
//    setPixel(j, red, green, blue, leds);
//  }
//  if ( i < 0) {
//    Set_Blank(tie, side);
//  }
//}
//
//void TailLight::EmptyRight_t(byte red, byte green, byte blue, int EyeSize, int * tie, CRGB leds[], bool side) {
//  int i = *tie;
//
//  for (int j = NUM_LEDS - 1; j > i; j--){
//    setPixel(j, red, green, blue, leds);
//  }
//  if ( i > NUM_LEDS) {
//    Set_Blank(tie, side);
//  }
//}
