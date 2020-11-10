
// USE PEDAL POSITION AS T VALUE

#define DEBUG

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

// CAN bus - MCP2515
MCP2515 HS_CAN(CAN_CS);        // SPI CS Pin 10             
struct can_frame canMsg; 
bool HS_CAN_MSG = false;
uint32_t CAN_ID;
CanSignal ThrottlePos;
CanSignal BrakePos;
CanSignal RPM;

// Opto 
uint8_t lights = 0;


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
    while (Serial.available() == 0) ;  // Wait here until input buffer has a character
    Serial.parseInt();
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

  Left.Set_Func();
  Right.Set_Func();
  #ifdef DEBUG
  Serial.println("Left Animations");
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
  HS_CAN.setFilter(MCP2515::RXF1 ,false, BrakePos.ID);
  HS_CAN.setFilter(MCP2515::RXF2 ,false, ThrottlePos.ID);
  HS_CAN.setFilter(MCP2515::RXF3 ,false, RPM.ID);
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
            Left.Set_Func();
            Right.Set_Func();
            break;
          case 0x03: //Update Can Ids
            break;            
        }
      } else if (canMsg.can_id == BrakePos.ID) {
        // BrakePos.signal = 0;
      } else if (canMsg.can_id == ThrottlePos.ID) {
        // ThrottlePos.signal = 0;
      } else if (canMsg.can_id == RPM.ID) {
        // RPM.signal = 0;
      }
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
      if (buttonState == HIGH) {
        CAN_ID = 0;
        save_EEPROM();
        Serial.println("BTN Press");
      }
    }
  }
  lastButtonState = reading;
}

TimedAction animationThread = TimedAction(20,Animation_Task);
TimedAction canThread = TimedAction(10,CAN_Task);
TimedAction optoThread = TimedAction(50,OPTO_Task);

void loop() {
  BTN_Task();
  animationThread.check();
  canThread.check();
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
}

void HS_CAN_ISR(){
  //Serial.println("CAN");
  HS_CAN_MSG = true;
}

void save_EEPROM(){
  #ifdef DEBUG
  Serial.println("Saving ");
  #endif
  uint32_t address = 0;
  uint32_t uint32;
  EEPROM.get(address, uint32);
  if (uint32 != CAN_ID){
    EEPROM.put(address, CAN_ID);
  }
  address += sizeof(CAN_ID);
  Left.SaveEEProm(&address);
  Right.SaveEEProm(&address);
  ThrottlePos.SaveEEProm(&address);
  BrakePos.SaveEEProm(&address);
  RPM.SaveEEProm(&address);
}

void load_EEPROM(){
  #ifdef DEBUG
  Serial.println("Loading");
  #endif
  uint32_t address = 0;
  EEPROM.get(address, CAN_ID);
  address += sizeof(CAN_ID);
  Left.LoadEEProm(&address);
  Right.LoadEEProm(&address);
  ThrottlePos.LoadEEProm(&address);
  BrakePos.LoadEEProm(&address);
  RPM.LoadEEProm(&address);

}

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
