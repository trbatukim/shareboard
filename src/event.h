#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct InputEvent
{
    uint8_t kind;    // 1 = keyboard, 2 = mouse
    uint8_t action;  
    uint16_t code;    
    uint16_t scan;    
    uint16_t flags;   
    int32_t x;       
    int32_t y;
    int32_t data; 
};
#pragma pack(pop)