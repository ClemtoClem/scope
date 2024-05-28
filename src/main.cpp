#include <fstream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mutex>

#include "rp_hw-calib.h"
#include "rp_hw-profiles.h"
#include "rpApp.h"

#include "hardwareInfo.h"
#include "log.h"
#include "main.h"
#include "synthesis.h"


/********************************************************/
/* Global variables                                     */
/********************************************************/

bool g_appState = false;

static const uint8_t g_dac_channels = getDACChannels();
static const uint8_t g_adc_channels = getADCChannels();

const float LEVEL_AMPS_MAX = outAmpMax();
const float LEVEL_AMPS_DEF = outAmpDef();
const bool is_z_present = isZModePresent();

bool g_need_update_sig_gen = false;
bool g_updateOutWaveFormCh[2] = {true, true};

std::mutex g_need_update_sig_gen_mtx;
std::mutex g_updateOutWaveFormCh_mtx;

float g_time_scale = 0;


/********************************************************/
/* Redpitaya parameters                                 */
/********************************************************/

PINT(SIGNAL_INTERVAL, RW, 20, FPGA_UPDATE_NONE, 1, 1000);
PINT(PARAMETER_INTERVAL, RW, 50, FPGA_UPDATE_NONE, 1, 1000);
PBOOL(DIGITAL_LOOP, RW, false, FPGA_UPDATE_NONE);

PBOOL(APP_RUN, RW, true, FPGA_UPDATE_NONE);
PINT(ADC_COUNT, RO, getADCChannels(), FPGA_UPDATE_NONE, 0, 4);
PINT(ADC_RATE, RO, getADCRate(), FPGA_UPDATE_NONE, getADCRate(), getADCRate());

// température de la carte
PINT2(OUT_TEMP_RUNTIME_CH, RW, 0, FPGA_UPDATE, 0, 200);
PINT2(OUT_TEMP_LATCHED_CH, RW, 0, FPGA_UPDATE, 0, 200);

PFLOAT(SLOW_OUT_CH1, RW, 0, FPGA_UPDATE_NONE, 0, rp_HPGetSlowDACFullScaleOrDefault(0));
PFLOAT(SLOW_OUT_CH2, RW, 0, FPGA_UPDATE_NONE, 0, rp_HPGetSlowDACFullScaleOrDefault(1));
PFLOAT(SLOW_OUT_CH3, RW, 0, FPGA_UPDATE_NONE, 0, rp_HPGetSlowDACFullScaleOrDefault(2));
PFLOAT(SLOW_OUT_CH4, RW, 0, FPGA_UPDATE_NONE, 0, rp_HPGetSlowDACFullScaleOrDefault(3));


/********************************************************/
/* Signals variables                                    */
/********************************************************/

SFLOAT4(OSC_SIGNAL_CH, CH_SIGNAL_SIZE_DEFAULT, 0.0f);
SFLOAT2(GEN_SIGNAL_CH, CH_SIGNAL_SIZE_DEFAULT, 0.0f);
SFLOAT2(MATH_SIGNAL_CH, CH_SIGNAL_SIZE_DEFAULT, 0.0f);


/********************************************************/
/* Oscilloscope parameters                              */
/********************************************************/

PBOOL(OSC_RESET, RW, false, FPGA_UPDATE_NONE);
PBOOL(OSC_SINGLE, RW, false, FPGA_UPDATE_NONE);
PBOOL(OSC_AUTOSCALE, RW, false, FPGA_UPDATE_NONE);

PFLOAT(OSC_TIME_OFFSET, RW, 0, FPGA_UPDATE_NONE, -100000, 100000);
PDOUBLE(OSC_TIME_SCALE, RW, 1, FPGA_UPDATE_NONE, 0, 1e8);
PINT(OSC_VIEW_START_POS, RO, 0, FPGA_UPDATE_NONE, 0, 16384);
PINT(OSC_VIEW_END_POS, RO, 0, FPGA_UPDATE_NONE, 0, 16384);
PINT(OSC_SAMPLE_RATE, RW, RP_DEC_1, FPGA_UPDATE_NONE, RP_DEC_1, RP_DEC_65536);
PFLOAT(OSC_VIEW_PART, RO, 0.1, FPGA_UPDATE_NONE, 0, 1);

PBOOL4(OSC_SHOW_CH, RW, false, FPGA_UPDATE);

PINT4(OSC_SMOOTH_CH, RW, 0, FPGA_UPDATE_NONE, 0, 3);
PFLOAT4(OSC_AMPLITUDE_SCALE_CH, RW, 1, FPGA_UPDATE_NONE, 0.00005, 1000);
PFLOAT4(OSC_OFFSET_CH, RW, 0, FPGA_UPDATE_NONE, -5000, 5000);

// Coupling channels
//	0 - DC (RP_DC)
//	1 - AC (RP_AC)
PINT4(OSC_COUPLING_CH, RW, RP_DC, FPGA_UPDATE_NONE, RP_DC, RP_AC);

// inversion du signe signal
PBOOL4(OSC_INVERT_CH, RW, false, FPGA_UPDATE); 

//User probe attenuation setting for channel 1 & 2:
//	0 - 1x
//	1 - 10x
//	2 - 100x
PINT4(OSC_PROBE_CH, RW, 0, FPGA_UPDATE, 0, 2);

// User jumper gain setting for channel 1 & 2:
//	0 - high gain (-1/1 [V] Full-scale)
//	1 - low gain (-20/20 [V] Full-scale)
PINT4(OSC_GAIN_CH, RW, 0, FPGA_UPDATE, 0, 1);

PFLOAT4(OSC_MEAS_MIN_CH, RO, 0, FPGA_UPDATE, -1000, 1000);
PFLOAT4(OSC_MEAS_MAX_CH, RO, 0, FPGA_UPDATE, -1000, 1000);
PFLOAT4(OSC_MEAS_AMP_CH, RO, 0, FPGA_UPDATE, -1000, 1000);
PFLOAT4(OSC_MEAS_AVG_CH, RO, 0, FPGA_UPDATE, -1000, 1000); // Average
PFLOAT4(OSC_MEAS_FREQ_CH, RO, 0, FPGA_UPDATE, -1000, 1000); // Frequency
PFLOAT4(OSC_MEAS_PER_CH, RO, 0, FPGA_UPDATE, -1000, 1000); // Periode


/********************************************************/
/* Trigger parameters                                   */
/********************************************************/

PFLOAT(OSC_TRIGGER_LEVEL, RW, 0, FPGA_UPDATE_NONE, -20, 20);
PFLOAT(OSC_TRIGGER_LIMIT, RW, 0, FPGA_UPDATE_NONE, -20, 20);

// Trigger source:
//	0 - ChA (RPAPP_OSC_TRIG_SRC_CH1)
//	1 - ChB (RPAPP_OSC_TRIG_SRC_CH2)
//	2 - ChC (RPAPP_OSC_TRIG_SRC_CH3)
//	3 - ChD (RPAPP_OSC_TRIG_SRC_CH4)
//	4 - external (RPAPP_OSC_TRIG_SRC_EXTERNAL)
PINT(OSC_TRIGGER_SOURCE, RW, RPAPP_OSC_TRIG_SRC_CH1, FPGA_UPDATE_NONE, RPAPP_OSC_TRIG_SRC_CH1, RPAPP_OSC_TRIG_SRC_EXTERNAL);

// Trigger slope
//  0 - Trigger source slope negative (RPAPP_OSC_TRIG_SLOPE_NE)
//  1 - Trigger source slope positive (RPAPP_OSC_TRIG_SLOPE_PE)
PINT(OSC_TRIGGER_SLOPE, RW, RPAPP_OSC_TRIG_SLOPE_PE, FPGA_UPDATE_NONE, RPAPP_OSC_TRIG_SLOPE_NE, RPAPP_OSC_TRIG_SLOPE_PE);

PFLOAT(OSC_TRIGGER_HYST, RW, 0.005, FPGA_UPDATE_NONE, 0, 1.0);

PBOOL(OSC_TRIGGER_LIMIT_IS_PRESENT, RO, rp_HPGetIsExternalTriggerLevelPresentOrDefault(), FPGA_UPDATE_NONE);

PINT(OSC_TRIGGER_SWEET, RW, RPAPP_OSC_TRIG_AUTO, FPGA_UPDATE_NONE, RPAPP_OSC_TRIG_AUTO, RPAPP_OSC_TRIG_SINGLE);

PINT(OSC_TRIGGER_INFO, RW, 0, FPGA_UPDATE, 0, 3);

PINT(EXT_CLOCK_ENABLE, RW, 0, FPGA_UPDATE, 0, 1);
PINT(EXT_CLOCK_LOCKED, RW, 0, FPGA_UPDATE, 0, 1);

// Trigger Edge:
//	0 - Rising
//	1 - Falling
PINT(OSC_TRIGGER_EDGE, RW, 0, FPGA_UPDATE, 0, 1);

PFLOAT(OSC_TRIGGER_DELAY, RW, 0, FPGA_UPDATE, -1e7,  1e7);


/********************************************************/
/* Generator parameters                                 */
/********************************************************/

PBOOL2(GEN_SHOW_CH, RW, false, FPGA_UPDATE_NONE);
PBOOL2(GEN_ENABLE_CH, RW, false, FPGA_UPDATE);
PFLOAT2(GEN_AMPLITUDE_CH, RW, LEVEL_AMPS_DEF, FPGA_UPDATE, 0, LEVEL_AMPS_MAX);
PFLOAT2(GEN_OFFSET_CH, RW, 0, FPGA_UPDATE, -LEVEL_AMPS_MAX, LEVEL_AMPS_MAX);
PFLOAT2(GEN_FREQUENCY_CH, RW, 1000, FPGA_UPDATE, 1, (int) getDACRate());
PFLOAT2(GEN_AMPLITUDE_SCALE_CH, RW, 1, FPGA_UPDATE_NONE, 0.00005, 1000);

PFLOAT2(GEN_PHASE_CH, RW, 0, FPGA_UPDATE_NONE, -360.0f, 360.0f);
PFLOAT2(GEN_DUTY_CYCLE_CH, RW, 25.0, FPGA_UPDATE, 0.0, 100.0f);
PFLOAT2(GEN_RISE_TIME_CH, RW, 1, FPGA_UPDATE_NONE, 0.1f, 100.0f);
PFLOAT2(GEN_FALL_TIME_CH, RW, 1, FPGA_UPDATE_NONE, 0.1f, 100.0f);

PINT2(GEN_WAVEFORM_CH, RW, 0, FPGA_UPDATE, 0, 8);
PINT2(GEN_TRIGGER_SOURCE_CH, RW, RP_GEN_TRIG_SRC_INTERNAL, FPGA_UPDATE_NONE, RP_GEN_TRIG_SRC_INTERNAL, RP_GEN_TRIG_SRC_EXT_NE);

PFLOAT2(GEN_SHOW_OFFSET_CH, RW, 0, FPGA_UPDATE, -40.0f, 40.0f);

PINT2(GEN_IMPEDANCE_CH,  RW, 0, FPGA_UPDATE_NONE, 0, 1);

PINT2(GEN_GAIN_CH, RW, RP_GAIN_1X, FPGA_UPDATE_NONE, RP_GAIN_1X, RP_GAIN_5X);
PBOOL(GEN_SYNCHRONISE, RW, true, FPGA_UPDATE_NONE);

PINT2(GEN_MODE_CH, RW, 0, FPGA_UPDATE, 0, 2); 

PFLOAT2(GEN_BURST_COUNT_CH, RW, 1, FPGA_UPDATE, 0, 1000);
PBOOL2(GEN_BURST_INF_CH, RW, false, FPGA_UPDATE_NONE);
PFLOAT2(GEN_BURST_PER_CH, RW, 1, FPGA_UPDATE, 0.0f, 50e6);
PFLOAT2(GEN_BURST_REP_CH, RW, 1, FPGA_UPDATE, 0.0f, 1000.0f);


/********************************************************/
/* Math parameters                                      */
/********************************************************/

PBOOL2(MATH_SHOW_CH, RW, false, FPGA_UPDATE_NONE);
// Operator
// 0 - Addition
// 1 - Soustration
// 2 - produit
// 3 - absolute
// 4 - invert
PINT2(MATH_OPERATOR_CH, RW, 0, FPGA_UPDATE, 0, 4);
PINT2(MATH_FIRST_SIGN_CH, RW, 0, FPGA_UPDATE, 0, 5);
PINT2(MATH_SECOND_SIGN_CH, RW, 0, FPGA_UPDATE, 0, 6);
PFLOAT2(MATH_CONST_CH, RW, 1.0, FPGA_UPDATE, -10.0, 10.0);

// Save signals
// 0 - json format
// 1 - csv format
// 2 - wav format
// 3 - dat format
PINT(SAVE_FORMAT, RW, 0, FPGA_UPDATE_NONE, 0, 3);




/********************************************************/
/* Generator functions                                  */
/********************************************************/

void Gen_init()
{
	if (!rp_HPIsFastDAC_PresentOrDefault())
		return;

	for(uint8_t i = 0; i < g_dac_channels; i++) {
		auto ch = (rp_channel_t)i;

		if (rp_HPGetFastDACIsTempProtectionOrDefault()) {
			bool state = false;
			if (rp_GetEnableTempProtection(ch, &state) == RP_OK){
				if (!state) {
					rp_SetEnableTempProtection(ch, true);
				}
			}
		}
	}
}

/// Updates temperature-related parameters to the web interface.
/// This function checks the runtime and latched temperature alarms for each DAC channel,
/// and sends the updated values to the web interface if they have changed.
void Gen_updateParamsToWeb()
{
	// Mesure de la température de la carte
	if (rp_HPGetFastDACIsTempProtectionOrDefault()) {
		for (uint8_t i = 0; i < g_dac_channels; i++) {
			auto ch = (rp_channel_t)i;
			bool temperature_runtime = false;
			if (rp_GetRuntimeTempAlarm(ch, &temperature_runtime) == RP_OK) {
				if (OUT_TEMP_RUNTIME_CH[ch].Value() != temperature_runtime) {
					OUT_TEMP_RUNTIME_CH[ch].SendValue(temperature_runtime);
				}
			}
			bool temperature_latched = false;
			if (rp_GetLatchTempAlarm(ch, &temperature_latched) == RP_OK) {
				if (OUT_TEMP_LATCHED_CH[ch].Value() != temperature_latched) {
					OUT_TEMP_LATCHED_CH[ch].SendValue(temperature_latched);
				}
			}
		}
	}
}

/**
 * Checks if the burst delay has changed for the specified channel and updates the burst period if necessary.
 * @param ch The channel to check for burst delay changes.
 */
void checkBurstDelayChanged(rp_channel_t ch)
{
	if (!rp_HPIsFastDAC_PresentOrDefault())
		return;

	uint32_t value = 0;
	rp_GenGetBurstPeriod(ch, &value);
	if (value != GEN_BURST_PER_CH[ch].Value()) {
		GEN_BURST_PER_CH[ch].SendValue(value);
	}
}

/**
 * @brief Updates the parameters of the signal generator channels.
 * This function is responsible for updating various parameters of the signal generator channels, such as enabling/disabling the output,
 * updating the amplitude, offset, phase, duty cycle, rise/fall time, waveform, and trigger source. It also handles the case where the gain DAC is set to 5x.
 * @param force If true, the function will update all parameters regardless of whether they have changed or not.
 */
void Gen_updateParams(bool force)
{
	bool requestSendScale = false;

	// S'il n'a pas été possible de déterminer le modèle alors on ne fait rien
	if (!rp_HPIsFastDAC_PresentOrDefault())
		return;

	for (int i = 0; i < g_dac_channels; i++) {
		auto ch = (rp_channel_t)i;
		if (GEN_ENABLE_CH[i].IsNewValue() || GEN_SHOW_CH[i].IsNewValue() ||
			GEN_AMPLITUDE_CH[i].IsNewValue() || GEN_OFFSET_CH[i].IsNewValue() || GEN_PHASE_CH[i].IsNewValue() ||
			GEN_DUTY_CYCLE_CH[i].IsNewValue() || GEN_WAVEFORM_CH[i].IsNewValue() || GEN_TRIGGER_SOURCE_CH[i].IsNewValue() ||
			GEN_RISE_TIME_CH[i].IsNewValue() || GEN_FALL_TIME_CH[i].IsNewValue() || GEN_IMPEDANCE_CH[i].IsNewValue() || force) {

			if (GEN_ENABLE_CH[i].IsNewValue() || force) {
				GEN_ENABLE_CH[i].Update();
				if (GEN_ENABLE_CH[i].Value() == true) {
					Log("Enable output channel %d\n", i + 1);
					// Détermine s'il y a la présence de la protection contre la surchauffe FAST DAC
					if (rp_HPGetFastDACIsTempProtectionOrDefault()) {
						rp_SetLatchTempAlarm(ch, false);
					}
					if (!force) {
						// Réinitialisez la machine à états pour le canal sélectionné.
						rp_GenResetChannelSM(ch);
						usleep(1000);
					}
					// Active la sortie pour le canal sélectionné.
					rp_GenOutEnable(ch);
					if (!force) {
						// Émettre un déclencheur pour le canal sélectionné
						rp_GenTriggerOnly(ch);
					}
				} else {
					Log("Disable output channel %d\n", i + 1);
					// Désactive la sortie pour le canal sélectionné
					rp_GenOutDisable(ch);
				}
			}

			if (GEN_AMPLITUDE_CH[ch].IsNewValue() || GEN_OFFSET_CH[ch].IsNewValue() || GEN_IMPEDANCE_CH[ch].IsNewValue() || force) {
				Log("Update amlpitude/offset/impedance output channel %d\n", i + 1);
				// Test la présence de l'amplificateur générateur en x5
				if (rp_HPGetIsGainDACx5OrDefault()) {
					auto prevGain = GEN_GAIN_CH[ch].Value();
					auto prevAmp = GEN_AMPLITUDE_CH[ch].Value();
					auto prevOff = GEN_OFFSET_CH[ch].Value();
					auto prevGainAPI = RP_GAIN_1X;
					auto prevAmpAPI = 0.0f;
					auto prevOffAPI = 0.0f;
					// Obtient l’amplitude crête à crête du signal de canal.
					rp_GenGetAmp((rp_channel_t)ch, &prevAmpAPI);
					// Obtient le décalage CC du signal
					rp_GenGetOffset((rp_channel_t)ch, &prevOffAPI);
					int res = 0;
					auto newFpgaGain = RP_GAIN_1X;
					float Coff = GEN_IMPEDANCE_CH[ch].Value() == 1 ? 2.0 : 1.0;
					if ((fabs(GEN_AMPLITUDE_CH[ch].NewValue()) + fabs(GEN_OFFSET_CH[ch].NewValue())) * Coff > 1.0) {
						newFpgaGain = RP_GAIN_5X;
						// Règle l'amplitude crête à crête du signal de canal.
						float amp = GEN_AMPLITUDE_CH[ch].NewValue() / 5.0 * Coff;
						float off = GEN_OFFSET_CH[ch].NewValue() / 5.0 * Coff;
						res |= rp_GenAmp((rp_channel_t)ch, amp);
						res |= rp_GenOffset((rp_channel_t)ch, off);
						Log("fpga gain x5; set amplitude = %f, offset = %f\n", amp, off);
					} else {
						float amp = GEN_AMPLITUDE_CH[ch].NewValue() * Coff;
						float off = GEN_OFFSET_CH[ch].NewValue() * Coff;
						res |= rp_GenAmp((rp_channel_t)ch, amp);
						res |= rp_GenOffset((rp_channel_t)ch, off);
						Log("fpga gain x1; set amplitude = %f, offset = %f\n", amp, off);
					}

					auto curGenStatus = RP_GAIN_1X;
					rp_GenGetGainOut(ch, &curGenStatus);
					prevGainAPI = curGenStatus;

					if (curGenStatus != newFpgaGain && rp_GenSetGainOut((rp_channel_t)ch, newFpgaGain) == RP_OK) {
						GEN_GAIN_CH[ch].SendValue(newFpgaGain);
					}

					if (res == RP_OK) {
						if (GEN_IMPEDANCE_CH[ch].IsNewValue() && !force) {
							Log("impedance new value\n");
							float impCoff = GEN_IMPEDANCE_CH[ch].NewValue() == 1 ? 0.5 : 2.0;
							GEN_IMPEDANCE_CH[i].Update();
							GEN_AMPLITUDE_CH[ch].Value() = GEN_AMPLITUDE_CH[ch].Value() * impCoff;
							GEN_AMPLITUDE_CH[ch].Update();
							GEN_OFFSET_CH[ch].Value() = GEN_OFFSET_CH[ch].Value() * impCoff;
							GEN_OFFSET_CH[ch].Update();
						}
					} else {
						rp_GenAmp((rp_channel_t)ch, prevAmpAPI);
						rp_GenOffset((rp_channel_t)ch, prevGainAPI);
						rp_GenSetGainOut((rp_channel_t)ch, prevGainAPI);
						GEN_GAIN_CH[ch].Update();
						GEN_OFFSET_CH[ch].Update();
						GEN_AMPLITUDE_CH[ch].Update();
						GEN_GAIN_CH[ch].SendValue(prevGain);
						GEN_AMPLITUDE_CH[ch].SendValue(prevAmp);
						GEN_OFFSET_CH[ch].SendValue(prevOff);
					}
					GEN_GAIN_CH[ch].Update();
					GEN_OFFSET_CH[ch].Update();
					GEN_AMPLITUDE_CH[ch].Update();
				} else {
					Log("rp_HPGetIsGainDACx5OrDefault NO, Set amplitude, offset")
					if (rp_GenAmp((rp_channel_t)ch, GEN_AMPLITUDE_CH[ch].NewValue()) == RP_OK) {
						GEN_AMPLITUDE_CH[ch].Update();
					} else {
						GEN_AMPLITUDE_CH[ch].SendValue(GEN_AMPLITUDE_CH[ch].Value());
					}
					if (rp_GenOffset((rp_channel_t)ch, GEN_OFFSET_CH[ch].NewValue()) == RP_OK) {
						GEN_OFFSET_CH[ch].Update();
					} else {
						GEN_OFFSET_CH[ch].SendValue(GEN_OFFSET_CH[ch].Value());
					}
				}
				requestSendScale = true;
				g_updateOutWaveFormCh[ch] = true;
			}

			if (GEN_PHASE_CH[i].IsNewValue() || force) {
				Log("Update phase output channel %d\n", i + 1);
				rp_GenPhase(ch, GEN_PHASE_CH[i].NewValue());
				GEN_PHASE_CH[i].Update();
			}

			if (GEN_DUTY_CYCLE_CH[i].IsNewValue() || force) {
				Log("Update duty cycle output channel %d\n", i + 1);
				rp_GenDutyCycle(ch, GEN_DUTY_CYCLE_CH[i].NewValue() / 100);
				GEN_DUTY_CYCLE_CH[i].Update();
			}

			if (GEN_RISE_TIME_CH[i].IsNewValue() || force) {
				Log("Update rise time output channel %d\n", i + 1);
				rp_GenRiseTime(ch, GEN_RISE_TIME_CH[i].NewValue());
				GEN_RISE_TIME_CH[i].Update();
			}

			if (GEN_FALL_TIME_CH[i].IsNewValue() || force) {
				Log("Update fall time output channel %d\n", i + 1);
				rp_GenFallTime(ch, GEN_FALL_TIME_CH[i].NewValue());
				GEN_FALL_TIME_CH[i].Update();
			}

			if (GEN_WAVEFORM_CH[i].IsNewValue() || force) {
				Log("Update waveform output channel %d\n", i + 1);
				rp_waveform_t wf = (rp_waveform_t)GEN_WAVEFORM_CH[i].NewValue();
				switch (wf) {
					case RP_WAVEFORM_SINE:
					case RP_WAVEFORM_SQUARE:
					case RP_WAVEFORM_TRIANGLE:
					case RP_WAVEFORM_RAMP_UP:
					case RP_WAVEFORM_RAMP_DOWN:
					case RP_WAVEFORM_DC:
					case RP_WAVEFORM_PWM:
					case RP_WAVEFORM_DC_NEG:
						rp_GenWaveform(ch, wf);
						GEN_WAVEFORM_CH[i].Update();
						break;
					case RP_WAVEFORM_SWEEP:     // non utilisé
					case RP_WAVEFORM_ARBITRARY: // non utilisé
					default:
						rp_GenWaveform(ch, RP_WAVEFORM_SINE);
						GEN_WAVEFORM_CH[i].Update();
						GEN_WAVEFORM_CH[i].Value() = RP_WAVEFORM_SINE;
						break;
				}
			}

			if (GEN_TRIGGER_SOURCE_CH[i].IsNewValue() || force) {
				Log("Update trigger source output channel %d\n", i + 1);
				rp_GenTriggerSource(ch, (rp_trig_src_t)GEN_TRIGGER_SOURCE_CH[i].NewValue());
				GEN_TRIGGER_SOURCE_CH[i].Update();
				if (!force)
					rp_GenResetTrigger(ch);
			}
		}

		if (GEN_AMPLITUDE_SCALE_CH[i].IsNewValue() || force) {
			Log("Update amplitude scale output channel %d\n", i + 1);
			GEN_AMPLITUDE_SCALE_CH[i].Update();
			std::lock_guard<std::mutex> lock(g_updateOutWaveFormCh_mtx);
			g_updateOutWaveFormCh[i] = true;
		}

		if (GEN_FREQUENCY_CH[i].IsNewValue() || force) {
			Log("Update frequency output channel %d\n", i + 1);
			float period = 1000000.0 / GEN_FREQUENCY_CH[ch].NewValue();
			GEN_RISE_TIME_CH[ch].SetMin(period * RISE_FALL_MIN_RATIO);
			GEN_RISE_TIME_CH[ch].SetMax(period * RISE_FALL_MAX_RATIO);
			GEN_RISE_TIME_CH[ch].Update();
			GEN_FALL_TIME_CH[ch].SetMin(period * RISE_FALL_MIN_RATIO);
			GEN_FALL_TIME_CH[ch].SetMax(period * RISE_FALL_MAX_RATIO);
			GEN_FALL_TIME_CH[ch].Update();
			rp_GenFreq(ch, GEN_FREQUENCY_CH[i].NewValue());
			GEN_FREQUENCY_CH[i].Update();
			if (!force)
				checkBurstDelayChanged(ch);
		}

		GEN_SHOW_CH[i].Update();
		GEN_SHOW_OFFSET_CH[i].Update();

		if (requestSendScale) {
			GEN_AMPLITUDE_SCALE_CH[i].SendValue(GEN_AMPLITUDE_SCALE_CH[i].Value());
		}
	}
}

void generate(rp_channel_t channel, float tscale)
{
	if (!rp_HPIsFastDAC_PresentOrDefault()) return;

	CFloatSignal *signal;
	rp_waveform_t waveform;
	rp_gen_sweep_mode_t sweep_mode;
	rp_gen_sweep_dir_t sweep_dir;
	rp_gen_mode_t gen_mode;
	float frequency, phase, amplitude, offset, showOff, duty_cycle, riseTime, fallTime;
	int burstCount, burstPeriod,burstReps, index_buffer;
	// std::vector<float> data;

	signal = &GEN_SIGNAL_CH[channel];
	waveform =  (rp_waveform_t) GEN_WAVEFORM_CH[channel].Value();
	frequency = GEN_FREQUENCY_CH[channel].Value();
	phase = (float) (GEN_PHASE_CH[channel].Value() / 180.0f * M_PI);
	amplitude = GEN_AMPLITUDE_CH[channel].Value() / GEN_AMPLITUDE_SCALE_CH[channel].Value();
	offset = GEN_OFFSET_CH[channel].Value() / GEN_AMPLITUDE_SCALE_CH[channel].Value();
	showOff = GEN_SHOW_OFFSET_CH[channel].Value();
	duty_cycle = GEN_DUTY_CYCLE_CH[channel].Value()/100;
	riseTime = GEN_RISE_TIME_CH[channel].Value();
	fallTime = GEN_FALL_TIME_CH[channel].Value();
	gen_mode = (rp_gen_mode_t) GEN_MODE_CH[channel].Value();
	burstCount = GEN_BURST_COUNT_CH[channel].Value();
	burstPeriod = GEN_BURST_PER_CH[channel].Value();
	burstReps = GEN_BURST_REP_CH[channel].Value();
	riseTime = GEN_RISE_TIME_CH[channel].Value();
	fallTime = GEN_FALL_TIME_CH[channel].Value();

	// float tscale = atof(inTimeScale.Value().c_str());
	if (tscale == 0)
		return;

	
	switch (waveform) {
		case RP_WAVEFORM_SINE:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS) {
				synthesis_sin(signal, frequency, phase, amplitude, offset, showOff, tscale);
			} else
				synthesis_sin_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_TRIANGLE:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_triangle(signal, frequency, phase, amplitude, offset, showOff, tscale);
			else
				synthesis_triangle_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_SQUARE:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_square(signal, frequency, phase, amplitude, offset, showOff, tscale, riseTime, fallTime);
			else
				synthesis_square_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale, riseTime, fallTime);
			break;
		case RP_WAVEFORM_RAMP_UP:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_rampUp(signal, frequency, phase, amplitude, offset, showOff, tscale);
			else
				synthesis_rampUp_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_RAMP_DOWN:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_rampDown(signal, frequency, phase, amplitude, offset, showOff, tscale);
			else
				synthesis_rampDown_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_DC:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_DC(signal, frequency, phase, amplitude, offset, showOff);
			else
				synthesis_DC_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_DC_NEG:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_DC_NEG(signal, frequency, phase, amplitude, offset, showOff);
			else
				synthesis_DC_NEG_burst(signal, frequency, phase, amplitude, offset, showOff, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_PWM:
			if (gen_mode == RP_GEN_MODE_CONTINUOUS)
				synthesis_PWM(signal, frequency, phase, amplitude, offset, showOff, duty_cycle, tscale);
			else
				synthesis_PWM_burst(signal, frequency, phase, amplitude, offset, showOff, duty_cycle, burstCount, burstPeriod, burstReps, tscale);
			break;
		case RP_WAVEFORM_SWEEP:
		default:
			break;
	}
}

void Gen_updateSignalsToWeb(float tscale)
{
	if (rp_HPIsFastDAC_PresentOrDefault()){
		for(int i = 0; i < g_dac_channels; i++){
			if (GEN_SHOW_CH[i].Value() == true && GEN_ENABLE_CH[i].Value() == true) {
				//Log("Gen_updateSignalsToWeb channel %d\n", i+1);
				if (GEN_SIGNAL_CH[i].GetSize() != CH_SIGNAL_SIZE_DEFAULT) {
					GEN_SIGNAL_CH[i].Resize(CH_SIGNAL_SIZE_DEFAULT);
				}
				std::lock_guard<std::mutex> lock(g_updateOutWaveFormCh_mtx);
				if (g_updateOutWaveFormCh[i] || tscale != 0) {
					generate((rp_channel_t)i,tscale);
					g_updateOutWaveFormCh[i] = false;
				}
				GEN_SIGNAL_CH[i].ForceSend();
				//Log("Send generator signal %d\n", i+1);
			} else {
				GEN_SIGNAL_CH[i].Resize(0);
			}
		}
	}
}


/********************************************************/
/* Scope functions                                      */
/********************************************************/

void Osc_init() {
	// Test la prise en charge des modes LV (1:1) et HV (1:20) pour les entrées ADC
	if (rp_HPGetFastADCIsAC_DCOrDefault()) {
		for(uint8_t i = 0; i < g_adc_channels ; i++) {
			rp_AcqSetAC_DC((rp_channel_t)i, (OSC_COUPLING_CH[i].Value() == 0) ? RP_AC : RP_DC);
		}
	}

	// Test la présence de la fonctionnalité PLL. Présent dans les planches 250-12
	if (rp_HPGetIsPLLControlEnableOrDefault()){
		rp_SetPllControlEnable(EXT_CLOCK_ENABLE.Value());
	}

	// Tets la prise en charge des modes AC et DC pour les entrées ADC
	if (rp_HPGetFastADCIsLV_HVOrDefault()) {
		for(uint8_t i = 0; i < g_adc_channels ; i++){
			rp_AcqSetGain((rp_channel_t) i, (OSC_GAIN_CH[i].Value() == 0) ? RP_LOW : RP_HIGH);
		}
	}
}

void Osc_updateTriggerLimit(bool force)
{
	// Checking trigger limitation
	float trigg_limit = 0;
	bool  is_signed = true;
	auto trigg_channel = (rpApp_osc_trig_source_t) OSC_TRIGGER_SOURCE.Value();
	switch(trigg_channel) {
		case RPAPP_OSC_TRIG_SRC_CH1:
			// Renvoie le gain d'acquisition actuellement défini dans la bibliothèque.
			// Il ne peut pas être réglé à la même valeur que celle définie sur le matériel Red Pitaya par les cavaliers de gain LV/HV.
			rp_AcqGetGainV(RP_CH_1, &trigg_limit);
			trigg_limit = trigg_limit * OSC_PROBE_CH[RP_CH_1].Value();
			break;
		case RPAPP_OSC_TRIG_SRC_CH2:
			rp_AcqGetGainV(RP_CH_2, &trigg_limit);
			trigg_limit = trigg_limit * OSC_PROBE_CH[RP_CH_2].Value();
			break;
		case RPAPP_OSC_TRIG_SRC_CH3:
			if (g_adc_channels >= 3){
				rp_AcqGetGainV(RP_CH_3, &trigg_limit);
				trigg_limit = trigg_limit * OSC_PROBE_CH[RP_CH_3].Value();
			}
			break;
		case RPAPP_OSC_TRIG_SRC_CH4:
			if (g_adc_channels >= 4){
				rp_AcqGetGainV(RP_CH_4, &trigg_limit);
				trigg_limit = trigg_limit * OSC_PROBE_CH[RP_CH_4].Value();
			}
			break;
		case RPAPP_OSC_TRIG_SRC_EXTERNAL:
			// Renvoie la pleine échelle pour le déclenchement externe
			trigg_limit = rp_HPGetIsExternalTriggerFullScalePresentOrDefault();
			// Indique si le déclencheur externe a une valeur signée
			is_signed = rp_HPGetIsExternalTriggerIsSignedOrDefault();
			break;
		default:
			Error("Unknown trigger source: %d",trigg_channel);
			rp_AcqGetGainV(RP_CH_1, &trigg_limit);
			trigg_limit = trigg_limit;
	}

	if (trigg_limit != OSC_TRIGGER_LIMIT.Value() || force){
		OSC_TRIGGER_LIMIT.SetMin(is_signed ? -trigg_limit : 0);
		OSC_TRIGGER_LIMIT.SetMax(trigg_limit);
		OSC_TRIGGER_LIMIT.SendValue(trigg_limit);
		// Need update trigger value
		float trigg_level = 0;
		auto trig_invert = false;
		if (trigg_channel != RPAPP_OSC_TRIG_SRC_EXTERNAL){
			trig_invert = OSC_INVERT_CH[trigg_channel].Value();
		}
		// Obtient le niveau de déclenchement
		if (rpApp_OscGetTriggerLevel(&trigg_level) == RP_OK && std::to_string(trigg_level) != "nan") {
			OSC_TRIGGER_LEVEL.SetMin(is_signed ? -trigg_limit : 0);
			OSC_TRIGGER_LEVEL.SetMax(trigg_limit);
			OSC_TRIGGER_LEVEL.SendValue(trig_invert ? -trigg_level : trigg_level);
			Log("Trigger level : %f\n", trig_invert ? -trigg_level : trigg_level);
		} else {
			Error("rpApp_OscGetTriggerLevel == nan\n");
		}
	}
}

int Osc_updateParamsToWeb() {	
	bool running;
	// Obtient l’état de l’oscilloscope. Si l'exécution est vraie, l'oscilloscope acquiert de nouvelles données,
	// sinon les données ne sont pas actualisées.
	rpApp_OscIsRunning(&running);
	if (APP_RUN.Value() != running)
		APP_RUN.SendValue(running);

	// Renvoie la présence de la fonctionnalité PLL. Présent dans les planches 250-12.
	if (rp_HPGetIsPLLControlEnableOrDefault()){
		bool pll_control_enable = false;
		if (rp_GetPllControlEnable(&pll_control_enable) == RP_OK){
			if (EXT_CLOCK_ENABLE.Value() != pll_control_enable){
				EXT_CLOCK_ENABLE.SendValue(pll_control_enable);
			}
		}
		bool pll_control_locked = false;
		if (rp_GetPllControlLocked(&pll_control_locked) == RP_OK){
			if (EXT_CLOCK_LOCKED.Value() != pll_control_locked){
				EXT_CLOCK_LOCKED.SendValue(pll_control_locked);
			}
		}
	}

	// Obtient la position du rapport de taille de vue proportionnelle à la taille du tampon ADC
	float portion;
	rpApp_OscGetViewPart(&portion);
	if (OSC_VIEW_PART.Value() != portion){
		OSC_VIEW_PART.SendValue(portion);
	}

	Osc_updateTriggerLimit(false);

	// Send current decimation
	rp_acq_decimation_t sampling_rate;
	rp_AcqGetDecimation(&sampling_rate);
	if (OSC_SAMPLE_RATE.Value() != sampling_rate){
		OSC_SAMPLE_RATE.SendValue(sampling_rate);
	}
	
	float value;
	rpApp_OscGetTimeOffset(&value);
	if (OSC_TIME_OFFSET.Value() != value){
		OSC_TIME_OFFSET.SendValue(value);
		OSC_TIME_SCALE.Update();
		OSC_VIEW_PART.Update();
	}

	double dvalue;
	for(uint8_t i = 0; i < g_adc_channels; i++){
		dvalue = 0;
		rpApp_OscGetAmplitudeScale((rpApp_osc_source) i, &dvalue);
		if (OSC_AMPLITUDE_SCALE_CH[i].Value() != dvalue) {
			OSC_AMPLITUDE_SCALE_CH[i].SendValue(dvalue);
		}
	}

	if (OSC_AUTOSCALE.NewValue()){
		bool as = false;
		rpApp_OscGetAutoScale(&as);
		OSC_AUTOSCALE.SendValue(as);
	} else {
		uint32_t start, end;
		rpApp_OscGetViewLimits(&start, &end);
		if (start != OSC_VIEW_START_POS.Value()){
			OSC_VIEW_START_POS.SendValue(start);
		}

		if (end != OSC_VIEW_END_POS.Value()){
			OSC_VIEW_END_POS.SendValue(end);
		}
	}

	// Obtient l'échelle de temps
	rpApp_OscGetTimeScale(&value);
	if (value != g_time_scale){
		g_time_scale = value;
		OSC_TIME_SCALE.SendValue(value * 1000);
	}

	for(uint8_t i = 0; i < g_adc_channels; i++){
		double dvalue = 0;
		rpApp_OscGetAmplitudeOffset((rpApp_osc_source) i, &dvalue);
		if (dvalue > OSC_OFFSET_CH[i].GetMax()){
			dvalue = OSC_OFFSET_CH[i].GetMax();
			rpApp_OscSetAmplitudeOffset((rpApp_osc_source) i, dvalue);
		}

		if (dvalue < OSC_OFFSET_CH[i].GetMin()){
			dvalue = OSC_OFFSET_CH[i].GetMin();
			rpApp_OscSetAmplitudeOffset((rpApp_osc_source) i, dvalue);
		}

		if (OSC_OFFSET_CH[i].Value() != dvalue){
			OSC_OFFSET_CH[i].SendValue(dvalue);
		}
	}
}

void Osc_updateParams(bool force)
{
	bool requestSendForTimeCursor = false;
	bool requestSendTriggerLevel = false;

	if (rp_HPGetIsPLLControlEnableOrDefault()){
		if(EXT_CLOCK_ENABLE.IsNewValue() || force) {
			if (rp_SetPllControlEnable(EXT_CLOCK_ENABLE.NewValue()) == RP_OK)
				EXT_CLOCK_ENABLE.Update();
		}
	}

	for (uint8_t i = 0; i <= g_adc_channels; i++) {
		if (OSC_SHOW_CH[i].IsNewValue() || force) {
			OSC_SHOW_CH[i].Update();
		}
	}

	// Activer ou désaciver l'ocilloscope
	if (APP_RUN.IsNewValue()) {
		if (APP_RUN.NewValue() == true) {
			rpApp_OscRun();
		} else {
			rpApp_OscStop();
		}
		APP_RUN.Update();
	}

	if (OSC_RESET.NewValue()) {
		rpApp_OscReset();
		OSC_RESET.Update();
		OSC_RESET.Value() = false;
	}

	if (OSC_SINGLE.NewValue()) {
		rpApp_OscSingle();
		OSC_SINGLE.Update();
		OSC_SINGLE.Value() = false;
		rpApp_osc_trig_sweep_t sweep;
		rpApp_OscGetTriggerSweep(&sweep);
		OSC_TRIGGER_SWEET.Value() = sweep;
	}

	OSC_AUTOSCALE.Update();
	if (OSC_AUTOSCALE.Value()) {
		if (rp_HPGetIsAttenuatorControllerPresentOrDefault()){
			for(uint8_t i = 0; i < g_adc_channels; i++){
				rpApp_OscSetInputGain((rp_channel_t)i, (rpApp_osc_in_gain_t) RPAPP_OSC_IN_GAIN_HV);
				OSC_SHOW_CH[i].Update();
				OSC_SHOW_CH[i].SendValue(RPAPP_OSC_IN_GAIN_HV);
			}
            sleep(1);
		}
		rpApp_OscAutoScale();
		return;
	}

	if (OSC_TRIGGER_HYST.IsNewValue() || force) {
		int res = rp_AcqSetTriggerHyst(OSC_TRIGGER_HYST.NewValue());
		if (res == RP_OK) { OSC_TRIGGER_HYST.Update(); }
	}

	if (OSC_TIME_SCALE.IsNewValue() || force){
		if (rpApp_OscSetTimeScale(OSC_TIME_SCALE.NewValue() / 1000.0) == RP_OK){
			OSC_TIME_SCALE.Update();
			std::lock_guard<std::mutex> lock(g_need_update_sig_gen_mtx);
			g_need_update_sig_gen = true;
		}
	}

	if (OSC_TIME_OFFSET.IsNewValue() || force){
		if (rpApp_OscSetTimeOffset(OSC_TIME_OFFSET.NewValue()) == RP_OK){
			OSC_TIME_OFFSET.Update();
			requestSendForTimeCursor = true;
		}
	}

	int trig_source_new = OSC_TRIGGER_SOURCE.NewValue();
	bool update_trig_level = OSC_TRIGGER_SOURCE.Value() != trig_source_new;
	bool trig_inversion_changed = false;
	bool trig_invert = false;

	// Checking the signal inversion
	for(uint8_t i = 0; i < g_adc_channels; i++){

		if (OSC_OFFSET_CH[i].IsNewValue() || force){
			if (rpApp_OscSetAmplitudeOffset((rpApp_osc_source)i,  OSC_OFFSET_CH[i].NewValue()) == RP_OK){
				OSC_OFFSET_CH[i].Update();
			}
		}

		if (OSC_AMPLITUDE_SCALE_CH[i].IsNewValue() || force) {
			if (rpApp_OscSetAmplitudeScale((rpApp_osc_source)i,  OSC_AMPLITUDE_SCALE_CH[i].NewValue()) == RP_OK){
				OSC_AMPLITUDE_SCALE_CH[i].Update();
				if (rp_HPGetIsAttenuatorControllerPresentOrDefault()){
					// AUTO select gain on autoscale
					int val = OSC_AMPLITUDE_SCALE_CH[i].Value() > 0.1 ? RPAPP_OSC_IN_GAIN_HV : RPAPP_OSC_IN_GAIN_LV;
					rpApp_osc_in_gain_t cur_val;
					if (rpApp_OscGetInputGain((rp_channel_t)i,&cur_val) == RP_OK){
						if (val != cur_val && !force){
							if (rpApp_OscSetInputGain((rp_channel_t)i, (rpApp_osc_in_gain_t)val) == RP_OK)
								OSC_GAIN_CH[i].SendValue(val);
						}
					}
				}
			}
		}

		if (trig_source_new == i){
			trig_invert = OSC_INVERT_CH[i].NewValue();
			trig_inversion_changed = OSC_INVERT_CH[i].Value() != trig_invert;
		}

		if (OSC_PROBE_CH[i].IsNewValue() || force) {
			int res = rpApp_OscSetProbeAtt((rp_channel_t)i, OSC_PROBE_CH[i].NewValue());
			if (res == RP_OK) OSC_PROBE_CH[i].Update();
		}


		if (OSC_SMOOTH_CH[i].IsNewValue() || force) {
			int res = rpApp_OscSetSmoothMode((rp_channel_t)i, (rpApp_osc_interpolationMode) OSC_SMOOTH_CH[i].NewValue());
			if (res == RP_OK) OSC_SMOOTH_CH[i].Update();
		}

		if (rp_HPGetFastADCIsAC_DCOrDefault()){
			if (OSC_COUPLING_CH[i].IsNewValue() || force) {
				int res = rp_AcqSetAC_DC((rp_channel_t)i, OSC_COUPLING_CH[i].NewValue() == 0 ? RP_AC:RP_DC);
				if (res == RP_OK) OSC_COUPLING_CH[i].Update();
			}
		}

		if (rp_HPGetFastADCIsLV_HVOrDefault()){
			if (OSC_GAIN_CH[i].IsNewValue() || force) {
				int res = rpApp_OscSetInputGain((rp_channel_t)i, (rpApp_osc_in_gain_t)OSC_GAIN_CH[i].NewValue());
				if (res == RP_OK) OSC_GAIN_CH[i].Update();
			}
		} else {
			if (OSC_GAIN_CH[i].IsNewValue() || force) {
				int res = rpApp_OscSetInputGain((rp_channel_t)i, (rpApp_osc_in_gain_t)RPAPP_OSC_IN_GAIN_LV);
				if (res == RP_OK) OSC_GAIN_CH[i].Update();
			}
		}

	}

	if (OSC_TRIGGER_SWEET.IsNewValue() || force) {
		int res = rpApp_OscSetTriggerSweep((rpApp_osc_trig_sweep_t) OSC_TRIGGER_SWEET.NewValue());
		if (res == RP_OK) OSC_TRIGGER_SWEET.Update();
	}

	if (OSC_TRIGGER_SOURCE.IsNewValue() || force){
		int res =  rpApp_OscSetTriggerSource((rpApp_osc_trig_source_t)OSC_TRIGGER_SOURCE.NewValue());
		if (res == RP_OK) {
			OSC_TRIGGER_SOURCE.Update(); // Used in pair with OSC_TRIGGER_LEVEL
			OSC_TRIGGER_LEVEL.Update();
			requestSendTriggerLevel = true;

		}
	}
	// Trigger level must be settled after trigger source
	if (trig_inversion_changed || OSC_TRIGGER_LEVEL.IsNewValue() || force) {
		if (rpApp_OscSetTriggerLevel(trig_invert ? -OSC_TRIGGER_LEVEL.NewValue() : OSC_TRIGGER_LEVEL.NewValue()) == RP_OK) {
			OSC_TRIGGER_LEVEL.Update(); // Used in pait with OSC_TRIGGER_SOURCE
			OSC_TRIGGER_SOURCE.Update();
			requestSendTriggerLevel = true;
		}
	}

	if (trig_inversion_changed || OSC_TRIGGER_SLOPE.IsNewValue() || force) {
		rpApp_osc_trig_slope_t slope = static_cast<rpApp_osc_trig_slope_t>(OSC_TRIGGER_SLOPE.NewValue());

		if (trig_invert) {
			slope = (slope == RPAPP_OSC_TRIG_SLOPE_PE) ? RPAPP_OSC_TRIG_SLOPE_NE : RPAPP_OSC_TRIG_SLOPE_PE;
		}
		if (rpApp_OscSetTriggerSlope(slope) == RP_OK) {
			OSC_TRIGGER_SLOPE.Update();
		}
	}

	for (uint8_t i = 0; i < g_adc_channels; i++){
		if (OSC_INVERT_CH[i].IsNewValue() || force) {
			int res = rpApp_OscSetInverted((rpApp_osc_source)i, OSC_INVERT_CH[i].NewValue());
			if (res == RP_OK) OSC_INVERT_CH[i].Update();
		}
	}

	// IF_VALUE_CHANGED_FORCE(mathOperation, rpApp_OscSetMathOperation((rpApp_osc_math_oper_t) mathOperation.NewValue()),force)

	if (update_trig_level){
		// Update the level
		float trigg_level;
		rpApp_OscGetTriggerLevel(&trigg_level);
		OSC_TRIGGER_LEVEL.Value() = trig_invert ? -trigg_level : trigg_level;
		OSC_TRIGGER_LEVEL.Update(); // Used in pait with OSC_TRIGGER_SOURCE
		OSC_TRIGGER_SOURCE.Update();

		// Update the slope
		rpApp_osc_trig_slope_t slope = RPAPP_OSC_TRIG_SLOPE_PE;
		rpApp_OscGetTriggerSlope(&slope);

		if (trig_invert) {
			slope = (slope == RPAPP_OSC_TRIG_SLOPE_PE) ? RPAPP_OSC_TRIG_SLOPE_NE : RPAPP_OSC_TRIG_SLOPE_PE;
		}

		OSC_TRIGGER_SLOPE.Value() = slope;
		OSC_TRIGGER_SLOPE.Update();
		requestSendTriggerLevel = true;
	}

	if (requestSendForTimeCursor){
		OSC_TIME_SCALE.SendValue(OSC_TIME_SCALE.Value());
		OSC_VIEW_PART.SendValue(OSC_VIEW_PART.Value());
	}

	if (requestSendTriggerLevel){
		OSC_TRIGGER_SOURCE.SendValue(OSC_TRIGGER_SOURCE.Value());
		for(uint8_t i = 0; i < g_adc_channels; i++){
			OSC_OFFSET_CH[i].SendValue(OSC_OFFSET_CH[i].Value());
			OSC_AMPLITUDE_SCALE_CH[i].SendValue(OSC_AMPLITUDE_SCALE_CH[i].Value());
		}
	}
}

void Osc_updateSignalsToWeb()
{
	for(uint8_t i = 0; i < g_adc_channels; i++){
		if (OSC_SHOW_CH[i].Value()) {
			if (OSC_SIGNAL_CH[i].GetSize() != CH_SIGNAL_SIZE_DEFAULT)
				OSC_SIGNAL_CH[i].Resize(CH_SIGNAL_SIZE_DEFAULT);
			rpApp_OscGetViewData((rpApp_osc_source)i, &OSC_SIGNAL_CH[i][0], (uint32_t) CH_SIGNAL_SIZE_DEFAULT);
		} else {
			OSC_SIGNAL_CH[i].Resize(0);
		}
	}
	rpApp_OscRefreshViewData();
}

float Osc_getTimeScale()
{
	float tscale = OSC_TIME_SCALE.Value() / 1000;
	return tscale;
}

bool Osc_isAutoScale()
{
    return OSC_AUTOSCALE.NewValue();
}



/********************************************************/
/* Run and stop                                         */ 
/********************************************************/


void updateSlowDAC(bool force){
    if (SLOW_OUT_CH1.IsNewValue() || force){
        if (rp_AOpinSetValue(0, SLOW_OUT_CH1.NewValue()) == RP_OK)
            SLOW_OUT_CH1.Update();
    }
    if (SLOW_OUT_CH2.IsNewValue() || force){
        if (rp_AOpinSetValue(1, SLOW_OUT_CH2.NewValue()) == RP_OK)
            SLOW_OUT_CH2.Update();
    }
    if (SLOW_OUT_CH3.IsNewValue() || force){
        if (rp_AOpinSetValue(2, SLOW_OUT_CH3.NewValue()) == RP_OK)
            SLOW_OUT_CH3.Update();
    }
    if (SLOW_OUT_CH4.IsNewValue() || force){
        if (rp_AOpinSetValue(3, SLOW_OUT_CH4.NewValue()) == RP_OK)
            SLOW_OUT_CH4.Update();
    }
}

// Run or stop APP
void run_app()
{	
    rpApp_OscRun();

	Osc_init();

	// Init generator
	if (rp_HPIsFastDAC_PresentOrDefault()) {
		Gen_init();
		Gen_updateParams(true);
        rp_GenSynchronise();
	}

	Osc_updateParams(true);
    //Math_updateParams(true);
	Osc_updateTriggerLimit(true);
	updateSlowDAC(true);

	rpApp_OscRunMainThread();

	g_appState = true;
}

void stop_app()
{
	g_appState = false;

	// Stop oscilloscope
	rpApp_OscStop();
}



/********************************************************/
/* RP Application functions                             */
/********************************************************/

//Application description
const char *rp_app_desc(void)
{
	return (const char *)"Scope and generator application.\n";
}


//Application init
int rp_app_init(void)
{
	g_appState = false;

	srand(time(0));

	// Initialization of API
	if (rpApp_Init() != RP_OK) {
		Error("Red Pitaya API init failed!\n");
		return EXIT_FAILURE;
	}

	run_app();

	CDataManager::GetInstance()->SetParamInterval(10);
	CDataManager::GetInstance()->SetSignalInterval(10);

	CDataManager::GetInstance()->SendAllParams();
	
	Log("Red Pitaya API init success!\n");

	return 0;
}


//Application exit
int rp_app_exit(void)
{
	stop_app();

	rpApp_Release();
	
	Log("Unload Red Pitaya application\n----------------------------------------\n");
	return 0;
}

//Set parameters
int rp_set_params(rp_app_params_t *p, int len)
{
	return 0;
}

//Get parameters
int rp_get_params(rp_app_params_t **p)
{
	return 0;
}

//Get signals
int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
	return 0;
}

//Update signals
void UpdateSignals(void)
{
	Osc_updateSignalsToWeb();
	float tscale = Osc_getTimeScale();
	Gen_updateSignalsToWeb(tscale);
}


//Update parameters
void UpdateParams(void)
{
#ifdef DIGITAL_LOOP
	rp_EnableDigitalLoop(digitalLoop.Value());
#else
	static bool inited_loop = false;
	if (!inited_loop) {
		rp_EnableDigitalLoop(DIGITAL_LOOP.Value());
		inited_loop = true;
	}
#endif

    //bool is_auto_scale = Osc_isAutoScale();
	Osc_updateParamsToWeb();
	Gen_updateParamsToWeb();
    //Math_updateParamsToWeb(is_auto_scale);
}

void PostUpdateSignals(){}

void OnNewParams(void)
{
	Osc_updateParams();
	Gen_updateParams();
}

void OnNewSignals(void)
{
	UpdateSignals();
}


