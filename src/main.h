#pragma once

#include <DataManager.h>
#include <CustomParameters.h>
#include "rpApp.h"
#include "rp.h"

#include <sys/syslog.h> //Add custom RP_LCR LOG system

#define CH_SIGNAL_SIZE_DEFAULT 16*1024 //Signal size
#define SIGNAL_UPDATE_INTERVAL 20
#define PARAMS_UPDATE_INTERVAL 10

#define RW CBaseParameter::RW
#define RWSA CBaseParameter::RWSA
#define RO CBaseParameter::RO
#define FPGA_UPDATE_NONE 0
#define FPGA_UPDATE 1

#define IF_VALUE_CHANGED(X) { \
	if (X.Value() != X.NewValue()) { \
		X.Update(); \
	} \
}

#define IF_VALUE_CHANGED_FORCE(X, FORCE) { \
	if (X.Value() != X.NewValue() || FORCE) { \
		X.Update(); \
	} \
}

#define IF_VALUE_CHANGED_ACTION(X, ACTION) { \
	if (X.Value() != X.NewValue()) { \
		ACTION; \
		X.Update(); \
	} \
}

#define IF_VALUE_CHANGED_ACTION_FORCE(X, ACTION, FORCE) { \
	if (X.Value() != X.NewValue() || FORCE) { \
		ACTION; \
		X.Update(); \
	} \
}

#define IF_VALUE_CHANGED_BOOL(X, ACTION1, ACTION2) { \
	if (X.Value() != X.NewValue()) { \
		if (X.NewValue()) { \
			ACTION1; X.Update(); \
		} else { \
			ACTION2; X.Update(); \
		} \
	} \
}

#define SFLOAT(VAR, size, value) CFloatSignal VAR(#VAR, size, value);
#define SFLOAT2(VAR, size, value) CFloatSignal VAR[2] = {{#VAR "1", size, value}, {#VAR "2", size, value}}
#define SFLOAT4(VAR, size, value) CFloatSignal VAR[4] = {{#VAR "1", size, value}, {#VAR "2", size, value}, {#VAR "3", size, value}, {#VAR "4", size, value}}

#define PBOOL(VAR, ...) CBooleanParameter VAR(#VAR, __VA_ARGS__)
#define PBOOL2(VAR, ...) CBooleanParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PBOOL4(VAR, ...) CBooleanParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}, {#VAR "3", __VA_ARGS__}, {#VAR "4", __VA_ARGS__}}

#define PINT(VAR, ...) CIntParameter VAR(#VAR, __VA_ARGS__)
#define PINT2(VAR, ...) CIntParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PINT4(VAR, ...) CIntParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}, {#VAR "3", __VA_ARGS__},{#VAR "4", __VA_ARGS__}}

#define PUINT(VAR, ...) CUIntParameter VAR(#VAR, __VA_ARGS__)
#define PUINT2(VAR, ...) CUIntParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PUINT4(VAR, ...) CUIntParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__},{#VAR "3", __VA_ARGS__},{#VAR "4", __VA_ARGS__}}

#define PFLOAT(VAR, ...) CFloatParameter VAR(#VAR, __VA_ARGS__)
#define PFLOAT2(VAR, ...) CFloatParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PFLOAT4(VAR, ...) CFloatParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}, {#VAR "3", __VA_ARGS__},{#VAR "4", __VA_ARGS__}}

#define PDOUBLE(VAR, ...) CDoubleParameter VAR(#VAR, __VA_ARGS__)
#define PDOUBLE2(VAR, ...) CDoubleParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PDOUBLE4(VAR, ...) CDoubleParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}, {#VAR "3", __VA_ARGS__},{#VAR "4", __VA_ARGS__}}

#define PSTRING(VAR, ...) CStringParameter VAR(#VAR, __VA_ARGS__)
#define PSTRING2(VAR, ...) CStringParameter VAR[2] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__}}
#define PSTRING4(VAR, ...) CStringParameter VAR[4] = {{#VAR "1", __VA_ARGS__},{#VAR "2", __VA_ARGS__},{#VAR "3", __VA_ARGS__},{#VAR "4", __VA_ARGS__}}




/********************************************************/
/* Generator functions                                  */
/********************************************************/

void Gen_init();

void Gen_updateParamsToWeb();

void checkBurstDelayChanged(rp_channel_t ch);

void Gen_updateParams(bool force = false);

void generate(rp_channel_t channel,float tscale);

void Gen_updateSignalsToWeb(float tscale);


/********************************************************/
/* Scope functions                                      */
/********************************************************/

void Osc_init();

/**
 * La fonction Osc_updateTriggerLimit est responsable de la mise à jour des valeurs de limite de
 * déclenchement et de niveau de déclenchement en fonction de la source de déclenchement sélectionnée
 * et des paramètres de gain des canaux d'acquisition.
 * La fonction prend une force d'entrée booléenne, qui détermine si la limite de déclenchement et
 * le niveau de déclenchement doivent être mis à jour quelles que soient leurs valeurs actuelles.
 * Le but de ce code est de garantir que les valeurs de limite de déclenchement et de niveau de
 * déclenchement sont correctement définies en fonction de la source de déclenchement sélectionnée et
 * des paramètres de gain des canaux d'acquisition. Ceci est important car les valeurs de limite de
 * déclenchement et de niveau de déclenchement sont utilisées pour déterminer le moment où l'oscilloscope
 * doit déclencher et capturer des données.
*/
void Osc_updateTriggerLimit(bool force = false);

int Osc_updateParamsToWeb();

void Osc_updateParams(bool force = false);

void Osc_updateSignalsToWeb();

float Osc_getTimeScale();


/********************************************************/
/* Run and stop                                         */ 
/********************************************************/

// Run or stop APP
void run_app();

void stop_app();




#ifdef __cplusplus
extern "C" {
#endif

/* Parameters description structure - must be the same for all RP controllers */
typedef struct rp_app_params_s {
	char  *name;
	float  value;
	int    fpga_update;
	int    read_only;
	float  min_val;
	float  max_val;
} rp_app_params_t;

//Rp app functions
const char *rp_app_desc(void);
int rp_app_init(void);
int rp_app_exit(void);
int rp_set_params(rp_app_params_t *p, int len);
int rp_get_params(rp_app_params_t **p);
int rp_get_signals(float ***s, int *sig_num, int *sig_len);

#ifdef __cplusplus
}
#endif
