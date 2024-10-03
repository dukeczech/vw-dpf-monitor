#pragma once

#include <Arduino.h>
#include <ELMduino.h>

// This (extern) tells the compiler that the variable wat exists somewhere (in .cpp)
extern ELM327 myELM327;

class OBD {
public:

static void init();

// Returns DBL_MAX if send/receive error
static double getPID(const uint8_t& service, const uint16_t& pid, const uint8_t& numResponses, const uint8_t& numExpectedBytes,
              const double& scaleFactor = 1.0, const float& bias = 0.0, const bool signedValue = false);
};