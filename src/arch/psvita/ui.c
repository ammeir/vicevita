/*
 * ui.c - Common UI routines.
 *
 * Written by
 *  Amnon-Dan Meir <ammeir71@yahoo.com>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

/* 
 * Most of the functions here are just stubs and not relevant for the
 * Vita port but perhaps usefull in the future.
 */

#include "vice.h"
#include "uiapi.h"
#include "machine.h"
#include "videoarch.h"
#include "cmdline.h"
#include "interrupt.h"
#include "lib.h"
#include "vsync.h"
#include "archdep.h"
#include "resources.h"
#include "debug_psv.h"
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <stdio.h>
#include <string.h>


static int is_paused = 0;

int ui_init_finalize(void)
{
    video_psv_ui_init_finalize();
    return 0;
}

/* Show a CPU JAM dialog.  */
ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
	static char message[512];

	va_list ap;
	va_start (ap, format);
	vsnprintf(message, sizeof(message) - 1, format, ap);
	va_end (ap);

	PSV_ShowMessage((const char*)message, 0);
	PSV_NotifyReset();

	return UI_JAM_HARD_RESET;
}

/* Report an error to the user.  */
void ui_error(const char *format, ...)
{
	static char message[512];

	va_list ap;
	va_start (ap, format);
	vsnprintf(message, sizeof(message) - 1, format, ap);
	va_end (ap);

	PSV_ShowMessage((const char*)message, 0);
}


static const cmdline_option_t cmdline_options[] = {
    { NULL },
};

static const resource_int_t resources_int[] = {
    { NULL }
};

int ui_init(int *argc, char **argv)
{
    return 0;
}

void ui_shutdown(void)
{
}

int ui_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

int ui_resources_init(void)
{
  return resources_register_int(resources_int);
}

void ui_display_volume(int vol)
{
}

/* Display a mesage without interrupting emulation */
void ui_display_statustext(const char *text, int fade_out)
{
}

static void pause_trap(uint16_t addr, void *data)
{
    vsync_suspend_speed_eval();
    while (is_paused) {
		PSV_ScanControls();
		usleep(10000);
    }
}

static void load_snapshot_trap(uint16_t addr, void *data)
{
	machine_read_snapshot((char*)data, 0);	
	lib_free(data);
}

void ui_pause_emulation(int flag)
{
	if (flag && !is_paused) {
        is_paused = 1;
        interrupt_maincpu_trigger_trap(pause_trap, 0);
    } else {
        is_paused = 0;
    }
}

void ui_load_snapshot(const char* file)
{
	interrupt_maincpu_trigger_trap(load_snapshot_trap, (char*)file);
}

int ui_emulation_is_paused(void)
{
	return is_paused;
}

void ui_display_drive_current_image(unsigned int drive_number, const char *image)
{
}

void ui_display_tape_control_status(int control)
{
	static int tape_control = 0;

	if (tape_control != control)
		PSV_NotifyTapeControl(control);

	tape_control = control;
}

void ui_display_tape_counter(int counter)
{
	static int tape_counter = 0;

	if (tape_counter != counter) {
        PSV_NotifyTapeCounter(counter);
    }

    tape_counter = counter;
}

void ui_display_tape_current_image(const char *image)
{
}

void ui_display_playback(int playback_status, char *version)
{
}

void ui_display_recording(int recording_status)
{
}

void ui_display_drive_track(unsigned int drive_number,
                            unsigned int drive_base,
                            unsigned int half_track_number)
{
}

void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color)
{
}

void ui_set_tape_status(int tape_status)
{
}

/* Update all the menus according to the current settings.  */
void ui_update_menus(void)
{
}

int ui_extend_image_dialog()
{
  return 0;
}

void ui_dispatch_events()
{
}

void ui_resources_shutdown()
{
}

int ui_init_finish()
{
	return 0;
}

char* ui_get_file(const char *format,...)
{
    return NULL;
}

void ui_display_joyport(uint8_t *joyport)
{
}

void ui_display_event_time(unsigned int current, unsigned int total)
{
}

void ui_check_mouse_cursor(void)
{
} 

void fullscreen_capability()
{
}

int c64ui_init_early(void)
{
    return 0;
}

int c64scui_init_early(void)
{
    return 0;
}

int c64ui_init(void)
{
	return 0;
}

int c64scui_init(void)
{
	return 0;
}

void c64ui_shutdown(void)
{
}

void c64scui_shutdown(void)
{
}

 
 
