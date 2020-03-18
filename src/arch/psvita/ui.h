
#ifndef VICE_UI_H
#define VICE_UI_H

#include "vice.h"
#include "types.h"
#include "uiapi.h"


extern void ui_exit(void);
extern void ui_display_speed(float percent, float framerate, int warp_flag);
extern void ui_dispatch_next_event(void);
extern void ui_dispatch_events(void);
extern void ui_error_string(const char *text);
extern void ui_pause_emulation(int flag);
extern void ui_load_snapshot(const char* file);
extern int  ui_emulation_is_paused(void); 
extern void ui_check_mouse_cursor(void);

#endif

