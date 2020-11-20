#include "Arduino.h"
#include "mcp2515.h"

#include "pin_defs.h"

#ifndef CAN_S_h
#define CAN_S_h

class CanSignal{
  public:
    CanSignal();

    uint16_t ID;
    bool b_endian;
    uint8_t start;
    uint8_t length;
    
    union signal_t {
      uint32_t u;
      int32_t i;
      double d;
      float f;
    } signal;
    union offset {
      int32_t i;
      double d;
    };
    union multipler {
      int32_t i;
      double d;
    };
    union min {
      uint32_t u;
      int32_t i;
      double d;
      float f;
    };
    union max {
      uint32_t u;
      int32_t i;
      double d;
      float f;
    };

    void CanSignal::DecodeFrame(struct can_frame *frame);
    void CanSignal::UpdateFrame(struct can_frame *frame);

    void CanSignal::LoadEEProm(uint32_t* address);
    void CanSignal::SaveEEProm(uint32_t* address);
};

#endif
