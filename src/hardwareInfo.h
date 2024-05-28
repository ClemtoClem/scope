#pragma one

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "rp.h"

typedef enum {
	RP_125_14,
	RP_250_12,
	RP_125_14_4CH,
	RP_122_16,
	RP_250_12_120
} models_t;

uint8_t getADCChannels();
uint8_t getDACChannels();
uint32_t getADCRate();
uint32_t getDACRate();

// return current CPU clock
double getClock();

models_t getModel();

float getMaxFreqRate();
float getMaxTriggerLevel();
float outAmpMax();
float outAmpDef();
bool isZModePresent();