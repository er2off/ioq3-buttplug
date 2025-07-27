/* Buttplug plugin export
 * (c) Er2 2025 <er2@dismail.de>
 * WTFPL License
 */

#ifndef BP_PUBLIC_H
#define BP_PUBLIC_H

#include "../qcommon/q_shared.h"

typedef struct {
	// print message on the local console
	void (QDECL *Printf)(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

	cvar_t* (*Cvar_Get)(const char* name, const char* value, int flags);
	void (*Cvar_Set)(const char* name, const char* value);
	void (*Cvar_SetValue)(const char* name, float value);
	void (*Cvar_CheckRange)(cvar_t* cv, float minVal, float maxVal, qboolean shouldBeIntegral);
	void (*Cvar_SetDescription)(cvar_t* cv, const char* description);

	void (*Cmd_AddCommand)(const char* name, void (*cmd)(void));
	void (*Cmd_RemoveCommand)(const char* name);

	int (*Cmd_Argc)(void);
	char* (*Cmd_Argv)(int i);
} bpimport_t;

typedef struct {
	// initializes cvars, cmds, etc.
	void (*Init)(void);
	void (*Stop)(void);
	qboolean (*Vibrate)(float intensity);
	void (*StopVibrate)(void);
} bpexport_t;

typedef bpexport_t* (QDECL *GetBPAPI_t)(bpimport_t* bimp);

// Client-only

#define BP_CH_SPEED		0
#define BP_CH_ATTACK		1
#define BP_CH_DMG		2
#define BP_MAX_CHANNELS		4

typedef struct {
	float intensity;
	int duration;
	int start;
	qboolean cycled;
} vibrate_t;

extern vibrate_t bp_vibrate[BP_MAX_CHANNELS];

extern void CL_AddVibration(int channel, float intensity, int duration, qboolean cycled);
extern void CL_StopVibrationCycle(int channel);

#endif
