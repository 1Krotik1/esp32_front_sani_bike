#pragma once

#include <Arduino.h>

void printCanErrorFlags(uint8_t errFlags);
void handleCanMessage(uint32_t id, const byte* data, uint8_t len);
