#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <string>

#include "hardwareInfo.h"

#include "rp.h"
#include "rp_hw-calib.h"
#include "log.h"

uint8_t getADCChannels() {
	uint8_t c = 0;
	if (rp_HPGetFastADCChannelsCount(&c) != RP_HP_OK){
		Error("Can't get fast ADC channels count\n");
	}
	return c;
}

uint8_t getDACChannels() {
	uint8_t c = 0;
	if (rp_HPGetFastDACChannelsCount(&c) != RP_HP_OK){
		Error("Can't get fast DAC channels count\n");
	}
	return c;
}

uint32_t getADCRate() {
	uint32_t c = 0;
	if (rp_HPGetBaseSpeedHz(&c) != RP_HP_OK){
		Error("an't get fast ADC channels count\n");
	}
	return c;
}

uint32_t getDACRate() {
	uint32_t c = 0;
	if (rp_HPGetBaseFastDACSpeedHz(&c) != RP_HP_OK){
		Error("Can't get fast DAC channels count\n");
	}
	return c;
}

double getClock() {
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return ((double)tp.tv_sec * 1000.f) + ((double)tp.tv_nsec / 1000000.f);
}

models_t getModel() {
	rp_HPeModels_t c = STEM_125_14_v1_0;
	if (rp_HPGetModel(&c) != RP_HP_OK){
		Error("Can't get board model\n");
	}

	switch (c) {
		case STEM_125_10_v1_0:
		case STEM_125_14_v1_0:
		case STEM_125_14_v1_1:
		case STEM_125_14_LN_v1_1:
		case STEM_125_14_Z7020_v1_0:
		case STEM_125_14_Z7020_LN_v1_1:
			return RP_125_14;

		case STEM_122_16SDR_v1_0:
		case STEM_122_16SDR_v1_1:
			return RP_122_16;

		case STEM_125_14_Z7020_4IN_v1_0:
		case STEM_125_14_Z7020_4IN_v1_2:
		case STEM_125_14_Z7020_4IN_v1_3:
			return RP_125_14_4CH;

		case STEM_250_12_v1_0:
		case STEM_250_12_v1_1:
		case STEM_250_12_v1_2:
		case STEM_250_12_v1_2a:
		case STEM_250_12_v1_2b:
			return RP_250_12;
		case STEM_250_12_120:
			return RP_250_12_120;
		default: {
			Error("Can't get board model\n");
			//exit(-1);
		}
	}
	return RP_125_14;
}

float getMaxFreqRate() {
	uint32_t rate = getADCRate();
	models_t model = getModel();
	switch (model) {
		case RP_125_14:
		case RP_122_16:
		case RP_125_14_4CH:
			return rate / 2;
		case RP_250_12:
			return rate / 4;
		case RP_250_12_120:
			return rate / 2;
		default:
			return 0;
	}
}

float getMaxTriggerLevel() {
	models_t model = getModel();
	switch (model) {
		case RP_125_14:
		case RP_122_16:
		case RP_125_14_4CH:
			return 2;
		case RP_250_12:
		case RP_250_12_120:
			return 5;
		default:
			return 0;
	}
}

bool isZModePresent() {
	models_t model = getModel();
	switch (model) {
		case RP_125_14:
		case RP_122_16:
		case RP_125_14_4CH:
			return false;
		case RP_250_12:
		case RP_250_12_120:
			return true;
		default:
			return false;
	}
}

float outAmpDef() {
	models_t model = getModel();
	switch (model) {
		case RP_125_14:
			return 0.9;
		case RP_122_16:
			return 0.4;
		case RP_125_14_4CH:
			return 0.9;
		case RP_250_12:
		case RP_250_12_120:
			return 0.9;
		default:
			return 0;
	}
}

float outAmpMax() {
	models_t model = getModel();
	switch (model) {
		case RP_125_14:
			return 1;
		case RP_122_16:
			return 0.5;
		case RP_125_14_4CH:
			return 1;
		case RP_250_12:
		case RP_250_12_120:
			return 10.0;
		default:
			return 0;
	}
}