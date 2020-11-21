#include "mcp2515.h"

#include "Arduino.h"
#include "CanSignal.h"
#include <EEPROM.h>

CanSignal::CanSignal(){
  
}

void CanSignal::DecodeFrame(struct can_frame *frame){

}

void CanSignal::UpdateFrame(struct can_frame *frame){

}

void CanSignal::LoadEEProm(uint32_t * address){
  ID = EEPROM.read(*address);
  *address += 2;
  min.u = EEPROM.read(*address);
  *address += 4;
  max.u = EEPROM.read(*address);
  *address += 4;
}

void CanSignal::SaveEEProm(uint32_t * address){
  uint16_t uint16;
  uint32_t uint32;
  EEPROM.get(*address, uint16);
  if (uint16 != ID ){
    EEPROM.put(*address, ID );
  }
  *address += 2;
  EEPROM.get(*address, uint32);
  if (uint32 != min.u ){
    EEPROM.put(*address, min.u );
  }
  *address += 4;
  EEPROM.get(*address, uint32);
  if (uint32 != max.u ){
    EEPROM.put(*address, max.u );
  }
  *address += 4;
}
