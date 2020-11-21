#define DEBUG
// #define FULL_CAN

#include "Arduino.h"
#include <EEPROM.h>
#include <FastLED.h>
#include <TimedAction.h>
#include "mcp2515.h"
#include "TailLight.h"
#include "CanSignal.h"
#include "pin_defs.h"

// LEDS Setup
TailLight Left;
TailLight Right;

// Btn Handling:
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned long timePressed = 0;        // millis() when button first pressed
bool longPress = false;               // has longPress been triggered
bool xxxlPress = false;               // has xxxlPress been triggered
#define LONG_PRESS_LEN  1000
#define XXXL_PRESS_LEN  30000

// CAN bus - MCP2515
MCP2515 HS_CAN(CAN_CS);        // SPI CS Pin 10             
struct can_frame canMsg; 
bool HS_CAN_MSG = false;
uint32_t CAN_ID;
CanSignal BrakePressure;
bool BrakeSwitch;
#ifdef FULL_CAN
  CanSignal ThrottlePos;
  CanSignal RPM;
  CanSignal SteeringAngle;
#endif

// Opto 
uint8_t lights = 0;

// Settings Menu
#define NUM_MENU_ITEMS 5
bool SettingsOpen = false;
uint8_t MenuIDX = 0;  // Menus = None, Start, Brake, Indi, Hazard
bool MenuItems[] = {false,false,false,false,false};   // Put Me into EEPROM



void setup() {

  //Inital Setup
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("Start");    
  #endif
  // Load from eeprom
  load_EEPROM();
  #ifdef DEBUG
    Serial.print("ID = "); Serial.println(CAN_ID, DEC);
  #endif
  
  if (CAN_ID > 0x7FF || CAN_ID == 0){
    Serial.begin(115200);
    Serial.println("Enter CAN ID");
//    while (Serial.available() == 0) ;  // Wait here until input buffer has a character
//    Serial.parseInt();
    CAN_ID = 2000;
    Serial.print("ID = "); Serial.println(CAN_ID, DEC);
    set_default();

    save_EEPROM();
    Serial.println("Please Reset");
    while(true) {}; // Die here and wait for reset.
  }

  // Btn Setup
  pinMode(BTN_PIN, INPUT);

  // Opto Setup
  pinMode(A0, INPUT); 
  pinMode(A1, INPUT); 
  pinMode(A2, INPUT); 
  pinMode(A3, INPUT); 
  pinMode(A4, INPUT); 
  pinMode(A5, INPUT); 

  // LED Setup
  FastLED.addLeds<WS2812,  LEFT_LED_PIN, GRB>(Left.leds, NUM_LEDS); //WS2812 
  FastLED.addLeds<WS2812, RIGHT_LED_PIN, GRB>(Right.leds, NUM_LEDS); //WS2812 
  FastLED.setBrightness(63);

  Left.settings = &SettingsOpen;
  Left.opto = &lights;
  Left.brakePressure = &BrakePressure;
  Left.SetMenu(MenuItems);
  Right.settings = &SettingsOpen;
  Right.opto = &lights;
  Right.brakePressure = &BrakePressure;
  Right.SetMenu(MenuItems);

  Left.Ans[BRAKE_ANI].ani = 7;
  Left.Ans[BRAKE_ANI].speed = 1;
  Right.Ans[BRAKE_ANI].ani = 7;
  Right.Ans[BRAKE_ANI].speed = 1;
  
  if (MenuItems[1]){ // if starup enabled
    Left.Ani.ani = Left.Ans[0].ani;
    Left.Ani.before = Left.Ans[0].before;
    Left.Ani.after  = Left.Ans[0].after;
    Left.Ani.speed = Left.Ans[0].speed;
    Left.Ani.eyesize = Left.Ans[0].eyesize;
  
    Right.Ani.ani = Right.Ans[0].ani;
    Right.Ani.before = Right.Ans[0].before;
    Right.Ani.after  = Right.Ans[0].after;
    Right.Ani.speed = Right.Ans[0].speed;
    Right.Ani.eyesize = Right.Ans[0].eyesize;
  } else {
    Left.startup_idx = STARTUP_L;
    Right.startup_idx = STARTUP_L;
    Left.Fin_Ani();
    Right.Fin_Ani();
  }
  Left.Set_Ani();
  Right.Set_Ani();
  #ifdef DEBUG
    Serial.println("L          eft Animations");
    for (int i = 0; i < TOTAL_ANI; i++){
      printANI(&Left.Ans[i]);  
    }
    Serial.println("Right Animations");
    for (int i = 0; i < TOTAL_ANI; i++){
      printANI(&Right.Ans[i]);  
    }
  #endif

  // CAN Setup
  HS_CAN.reset();        
  // ADD FILTER                  
  HS_CAN.setBitrate(CAN_500KBPS,MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz 
  HS_CAN.setFilterMask(MCP2515::MASK0 ,false ,0x07FF);
  HS_CAN.setFilterMask(MCP2515::MASK1 ,false ,0x07FF);
  HS_CAN.setFilter(MCP2515::RXF0 ,false, CAN_ID);
  HS_CAN.setFilter(MCP2515::RXF1 ,false, 133);//BrakePressure.ID);
  #ifdef FULL_CAN
    HS_CAN.setFilter(MCP2515::RXF2 ,false, ThrottlePos.ID);
    HS_CAN.setFilter(MCP2515::RXF3 ,false, RPM.ID);
    HS_CAN.setFilter(MCP2515::RXF4 ,false, SteeringAngle.ID);
  #endif
  HS_CAN.setNormalMode();                  //Sets CAN at normal mode
  attachInterrupt(digitalPinToInterrupt(CAN_INT), HS_CAN_ISR,FALLING);
  Serial.println("Fin Setup");
}

// TASK NUMBER THE FRIST
void Animation_Task(){
  Left.Ani_Func();
  Right.Ani_Func();
  FastLED.show();
}

// TASK NUMBER THE SECOND
void CAN_Task(){
  if (HS_CAN_MSG){
    HS_CAN_MSG = false;
    while (HS_CAN.readMessage(&canMsg) == MCP2515::ERROR_OK){ 
      #ifdef DEBUG
        printCanMsg("HS",&canMsg);
      #endif
      if (canMsg.can_id == CAN_ID){
        switch(canMsg.data[0]){
          case 0x00: // Ans Left Update
            Left.Ans[canMsg.data[1]].ani = canMsg.data[2];
            Left.Ans[canMsg.data[1]].before.r = canMsg.data[3];
            Left.Ans[canMsg.data[1]].after.r  = canMsg.data[4];
            Left.Ans[canMsg.data[1]].speed = canMsg.data[5];
            Left.Ans[canMsg.data[1]].eyesize = canMsg.data[6];
            save_EEPROM();
            break;
          case 0x01: // Ans Right Update
            Right.Ans[canMsg.data[1]].ani = canMsg.data[2];
            Right.Ans[canMsg.data[1]].before.r = canMsg.data[3];
            Right.Ans[canMsg.data[1]].after.r  = canMsg.data[4];
            Right.Ans[canMsg.data[1]].speed = canMsg.data[5];
            Right.Ans[canMsg.data[1]].eyesize = canMsg.data[6];
            save_EEPROM();
            break;  
          case 0x02: // Trigger
            Left.Ani = Left.Ans[25];
            Right.Ani = Right.Ans[25];
            Left.Set_Ani();
            Right.Set_Ani();
            break;
          case 0x03: // Update Settings
            for (int i = 0; i < NUM_MENU_ITEMS; i++){
              if (canMsg.data[1] & (1 << i))
                MenuItems[i] = true;
              else
                MenuItems[i] = false;
            }
          case 0x04: // Update Can Data
            break;            
        }
      } else if (canMsg.can_id == BrakePressure.ID) {
        BrakePressure.signal.u = ((canMsg.data[0] * 256) + canMsg.data[1]);
        if (canMsg.data[2] & 0x40 )
          BrakeSwitch = true;
        else 
          BrakeSwitch = false;
      } 
      #ifdef FULL_CAN 
        else if (canMsg.can_id == ThrottlePos.ID) {
          ThrottlePos.signal.u = canMsg.data[7] * 0.5;
        } else if (canMsg.can_id == RPM.ID) {
          RPM.signal.u = ((canMsg.data[0] * 256) + canMsg.data[1]) * 0.25;
        } else if (canMsg.can_id == SteeringAngle.ID) {
          SteeringAngle.signal.i = (int)((canMsg.data[2] * 256) + canMsg.data[3]);
        }
      #endif
    }
  }
}

// TASK NUMBER THE THIRD
void OPTO_Task(){
  if (digitalRead(A0) == 0){
    lights |= 0x01;
  } else {
    lights &= 0xFE;
  }
  if (digitalRead(A1) == 0){
    lights |= 0x02;
  } else {
    lights &= 0xFD;
  }
  if (digitalRead(A2) == 0){
    lights |= 0x04;
  } else {
    lights &= 0xFB;
  }
  if (digitalRead(A3) == 0){
    lights |= 0x08;
  } else {
    lights &= 0xF7;
  }
  if (digitalRead(A4) == 0){
    lights |= 0x10;
  } else {
    lights &= 0xEF;
  }
  if (digitalRead(A5) == 0){
    lights |= 0x20;
  } else {
    lights &= 0xDF;
  }
  #ifdef DEBUG
    //Serial.println(lights,BIN);
  #endif
}

// TASK NUMBER THE FORTH
void BTN_Task(){
  // Btn Debouncing
  // https://www.arduino.cc/en/Tutorial/BuiltInExamples/Debounce
  int reading = digitalRead(BTN_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {  // Happens once when button first goes high
        timePressed = millis();
      } else {                    // Happens once when button first goes low
        if (not longPress) {
          short_Press();
        }
        longPress = false;
        xxxlPress = false;
      }
    }
  }
  lastButtonState = reading;

  if (buttonState == HIGH){
    unsigned long Pressed = millis() - timePressed;
    if (Pressed > LONG_PRESS_LEN) {
      if (not longPress) {
          longPress = true;
          long_Press();
      }
    } 
    if (Pressed > XXXL_PRESS_LEN){
      if (not xxxlPress) {
          xxxlPress = true;
          xxxl_Press();
      }
    }
  }
}

void short_Press() {
  Serial.println("shortly");
  if (SettingsOpen) {
     MenuIDX++;
     if (MenuIDX ==  NUM_MENU_ITEMS) {
      MenuIDX = 0;
     }
    Left.Ans[SETTINGS_MENU_ANI].eyesize = MenuIDX;
    Left.Ans[SETTINGS_MENU_ANI].speed = ((!MenuItems[MenuIDX]) * 16) + 12;
    Left.t = 0;
  }
}

void long_Press() {
  Serial.println("longly");
  if (SettingsOpen) {
    if (MenuIDX > 0){
      MenuItems[MenuIDX] = !MenuItems[MenuIDX];
      Left.Ans[SETTINGS_MENU_ANI].speed = ((!MenuItems[MenuIDX]) * 16) + 12;
      Left.t = 0;    
    } else if (MenuIDX == 0) {
      SettingsOpen = false;
      save_EEPROM();
    }
  }
}

void xxxl_Press() {
  Serial.println("xxxly");
  if (!SettingsOpen) {
    SettingsOpen = true;
    Left.Ans[SETTINGS_MENU_ANI].eyesize = MenuIDX;
    Left.Ans[SETTINGS_MENU_ANI].speed = ((!MenuItems[MenuIDX]) * 16) + 12;
    Left.Ani = Left.Ans[SETTINGS_FLASH_ANI];
    Right.Ani = Right.Ans[SETTINGS_FLASH_ANI];
    Left.Set_Ani();
    Right.Set_Ani();
  }
}

TimedAction animationThread = TimedAction(20,Animation_Task);
TimedAction btnThread = TimedAction(10,BTN_Task);
TimedAction optoThread = TimedAction(50,OPTO_Task);

void loop() {
  CAN_Task();
  animationThread.check();
  btnThread.check();
  optoThread.check();
}

void set_default(){

  for (int i = 0; i < TOTAL_ANI; i++){
    Left.Ans[i].ani = 255;
    Left.Ans[i].before = CRGB::Black;
    Left.Ans[i].after  = CRGB::Red;
    Left.Ans[i].speed = 2;
    Left.Ans[i].eyesize = 10;

    Right.Ans[i].ani = 255;
    Right.Ans[i].before = CRGB::Black;
    Right.Ans[i].after  = CRGB::Red;
    Right.Ans[i].speed = 2;
    Right.Ans[i].eyesize = 10;
  }
  
  Left.Ans[0].ani = 2;
  Left.Ans[0].before = CRGB::Black;
  Left.Ans[0].after  = CRGB(0x3F,0x00,0x00);
  Left.Ans[0].speed = 2;
  Left.Ans[0].eyesize = 10;

  Left.Ans[1].ani = 3;
  Left.Ans[1].before = CRGB::Black;
  Left.Ans[1].after  = CRGB(0x3F,0x00,0x00);
  Left.Ans[1].speed = 2;
  Left.Ans[1].eyesize = 10;

  Left.Ans[2].ani = 5;
  Left.Ans[2].before = CRGB::Black;
  Left.Ans[2].after  = CRGB(0x3F,0x00,0x00);
  Left.Ans[2].speed = 2;
  Left.Ans[2].eyesize = 10;

  Left.Ans[3].ani = 6;
  Left.Ans[3].before = CRGB(0x3F,0x00,0x00);
  Left.Ans[3].after  = CRGB::Red;
  Left.Ans[3].speed = 2;
  Left.Ans[3].eyesize = 10;

  Left.Ans[4].ani = 4;
  Left.Ans[4].before = CRGB::Red;
  Left.Ans[4].after  = CRGB(0x3F,0x00,0x00);
  Left.Ans[4].speed = 2;
  Left.Ans[4].eyesize = 10;

  Left.Ans[5].ani = 8;
  Left.Ans[5].before = CRGB::Black;
  Left.Ans[5].after  = CRGB(0x3F,0x00,0x00);
  Left.Ans[5].speed = 2;
  Left.Ans[5].eyesize = 10;

  Right.Ans[0].ani = 2;
  Right.Ans[0].before = CRGB::Black;
  Right.Ans[0].after  = CRGB(0x3F,0x00,0x00);
  Right.Ans[0].speed = 2;
  Right.Ans[0].eyesize = 10;

  Right.Ans[1].ani = 3;
  Right.Ans[1].before = CRGB::Black;
  Right.Ans[1].after  = CRGB(0x3F,0x00,0x00);
  Right.Ans[1].speed = 2;
  Right.Ans[1].eyesize = 10;

  Right.Ans[2].ani = 5;
  Right.Ans[2].before = CRGB::Black;
  Right.Ans[2].after  = CRGB(0x3F,0x00,0x00);
  Right.Ans[2].speed = 2;
  Right.Ans[2].eyesize = 10;

  Right.Ans[3].ani = 6;
  Right.Ans[3].before = CRGB(0x3F,0x00,0x00);
  Right.Ans[3].after  = CRGB::Red;
  Right.Ans[3].speed = 2;
  Right.Ans[3].eyesize = 10;

  Right.Ans[4].ani = 4;
  Right.Ans[4].before = CRGB::Red;
  Right.Ans[4].after  = CRGB(0xF,0x00,0x00);
  Right.Ans[4].speed = 2;
  Right.Ans[4].eyesize = 10;

  Right.Ans[5].ani = 8;
  Right.Ans[5].before = CRGB::Black;
  Right.Ans[5].after  = CRGB(0xF,0x00,0x00);
  Right.Ans[5].speed = 2;
  Right.Ans[5].eyesize = 10; 

  // Brake Can Bus
  Left.Ans[BRAKE_ANI].ani = 9;
  Left.Ans[BRAKE_ANI].speed = 1;
  Right.Ans[BRAKE_ANI].ani = 9;
  Right.Ans[BRAKE_ANI].speed = 1;
 
  // Menu 
  Left.Ans[SETTINGS_MENU_ANI].ani = 11;
  Left.Ans[SETTINGS_MENU_ANI].speed = 16;
  Left.Ans[SETTINGS_MENU_ANI].eyesize = 0;

  // 1/2 second flash
  Left.Ans[SETTINGS_FLASH_ANI].ani = 9;
  Left.Ans[SETTINGS_FLASH_ANI].speed = 50;
  Left.Ans[SETTINGS_FLASH_ANI].eyesize = 10;

  Right.Ans[SETTINGS_FLASH_ANI].ani = 9;
  Right.Ans[SETTINGS_FLASH_ANI].speed = 50;
  Right.Ans[SETTINGS_FLASH_ANI].eyesize = 10;

  BrakePressure.ID = 133;
  BrakePressure.min.u = 0x66;
  BrakePressure.max.u = 0xFF;
  #ifdef FULL_CAN
    ThrottlePos.ID = 512;
    RPM.ID = 513;
    SteeringAngle.ID = 129;
  #endif

  for (uint8_t i = 0; i < NUM_MENU_ITEMS; i++){
    MenuItems[i] = true;
  }
}

void HS_CAN_ISR(){
  HS_CAN_MSG = true;
}

void save_EEPROM(){
  #ifdef DEBUG
    Serial.println("Saving ");
  #endif
  uint32_t address = 0;
  uint32_t uint32;
  bool boo;
  EEPROM.get(address, uint32);
  if (uint32 != CAN_ID){
    EEPROM.put(address, CAN_ID);
  }
  address += sizeof(CAN_ID);
  for (uint8_t i = 0; i < NUM_MENU_ITEMS; i++){
    EEPROM.get(address, boo);
    if (boo != MenuItems[i]){
      EEPROM.put(address, MenuItems[i]);  
    }
    address += sizeof(boo);
  }
  Left.SaveEEProm(&address);
  Right.SaveEEProm(&address);
  BrakePressure.SaveEEProm(&address);
  #ifdef FULL_CAN
    ThrottlePos.SaveEEProm(&address);
    SteeringAngle.SaveEEProm(&address);
    RPM.SaveEEProm(&address);
  #endif
}

void load_EEPROM(){
  #ifdef DEBUG
    Serial.println("Loading");
  #endif
  uint32_t address = 0;
  EEPROM.get(address, CAN_ID);
  address += sizeof(CAN_ID);
  for (uint8_t i = 0; i < NUM_MENU_ITEMS; i++){
    EEPROM.get(address, MenuItems[i]);
    address += sizeof(MenuItems[i]);
  }
  Left.LoadEEProm(&address);
  Right.LoadEEProm(&address);
  BrakePressure.LoadEEProm(&address);
  #ifdef FULL_CAN
    ThrottlePos.LoadEEProm(&address);
    SteeringAngle.LoadEEProm(&address);
    RPM.LoadEEProm(&address);
  #endif
}

#ifdef DEBUG
void printCANSig(){
  #ifdef FULL_CAN
    Serial.print(BrakePressure.signal.u);
    Serial.print(";");
    Serial.print(ThrottlePos.signal.u);
    Serial.print(";");
    Serial.print(RPM.signal.u);
    Serial.print(";");
    Serial.println(SteeringAngle.signal.i);
  #else
    Serial.println(BrakePressure.signal.u);
  #endif
}
#endif

#ifdef DEBUG
void printANI(ANIMATION * ani){
  Serial.print("<");
  Serial.print(ani->ani,DEC);
  Serial.print(";");
  Serial.print(ani->before.r,HEX);
  Serial.print(ani->before.g,HEX);
  Serial.print(ani->before.b,HEX);
  Serial.print(";");
  Serial.print(ani->after.r,HEX);
  Serial.print(ani->after.g,HEX);
  Serial.print(ani->after.b,HEX);
  Serial.print(";");
  Serial.print(ani->speed,DEC);
  Serial.print(";");
  Serial.print(ani->eyesize,DEC);
  Serial.println(">");
}
#endif

#ifdef DEBUG
void printCanMsg(char* name, struct can_frame *frame){
  Serial.print("<");
  Serial.print(name);
  Serial.print(";");
  if (frame->can_id < 256) {
    Serial.print("0");  
  }
  Serial.print(frame->can_id, HEX);
  Serial.print(";");
  for(int i=0;i<frame->can_dlc-1;i++) {  
    if (frame->data[i] < 16) {
      Serial.print("0");  
    }
    Serial.print(frame->data[i], HEX);
    Serial.print(",");
  }  
  if (frame->data[frame->can_dlc-1] < 16) {
      Serial.print("0");  
  }               
  Serial.print(frame->data[frame->can_dlc-1], HEX);             
  Serial.println(">");
}
#endif
