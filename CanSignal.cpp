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
}

void CanSignal::SaveEEProm(uint32_t * address){
  uint16_t uint16;
  EEPROM.get(*address, uint16);
  if (uint16 != ID ){
    EEPROM.put(*address, ID );
  }
  *address += 2;
}
