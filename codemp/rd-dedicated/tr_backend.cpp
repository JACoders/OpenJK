#include "tr_local.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;

bool tr_stencilled = false;
extern qboolean tr_distortionPrePost; //tr_shadows.cpp
extern qboolean tr_distortionNegate; //tr_shadows.cpp
extern void RB_CaptureScreenImage(void); //tr_shadows.cpp
extern void RB_DistortionFill(void); //tr_shadows.cpp

// Whether we are currently rendering only glowing objects or not.
bool g_bRenderGlowingObjects = false;

// Whether the current hardware supports dynamic glows/flares.
bool g_bDynamicGlowSupported = false;

extern void R_RotateForViewer(void);
extern void R_SetupFrustum(void);
