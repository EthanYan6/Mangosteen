
#ifndef APP_COMMON_H
#define APP_COMMON_H

#include "functions.h"
#include "settings.h"
#include "ui/ui.h"

void COMMON_KeypadLockToggle();
/* Manual A/B: dual/cross-band switches (+ home-card anim); MAIN ONLY beeps and refuses. */
void COMMON_SwitchVFOs();
void COMMON_SwitchVFOMode();

#endif