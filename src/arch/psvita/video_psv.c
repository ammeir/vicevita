
/*
 * video_psv.c - PSVITA implementation of the video interface.
 *
 * Written by
 *  Amnon-Dan Meir <ammeir71@yahoo.com>
 *
 * Based on code by the VICE team
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

#include "vice.h"
#include "video.h"
#include "videoarch.h"
#include "palette.h"
#include "lib.h"
#include "raster.h"
#include "interrupt.h"
#include "sound.h"
#include "vsync.h"
#include "cmdline.h"
#include "resources.h"
#include "controller.h"
#include "debug_psv.h"
#include <string.h>


static struct video_canvas_s*	activeCanvas = NULL;
static float					last_framerate = 0;
static float					last_percent = 0;
static int						last_warp_flag = 0;
static int						drive_led_on = 0, tape_led_on = 0;
static video_canvas_t*			active_canvas = NULL;
static void						pause_trap(uint16_t unused_addr, void *data); 
static void						load_snapshot_trap(uint16_t addr, void *data);
void							video_psv_menu_show();


static const cmdline_option_t cmdline_options[] = {
    { NULL },
};

static const resource_int_t resources_int[] = {
    { NULL }
};

void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
	unsigned int x, y, width, height;
	
	if (!(canvas && canvas->draw_buffer && canvas->videoconfig)) {
        return;
    }
	
    width = canvas->draw_buffer->canvas_width;
    height = canvas->draw_buffer->canvas_height;

    /* Ignore bad values, or values that don't change anything */
    if (width == 0 || height == 0 || (canvas->width && height == canvas->height)) {
        return;
    }
	
    canvas->width = canvas->actual_width = width;
    canvas->height = canvas->actual_height = height;
	PSV_GetViewInfo(0,0,0,0,&canvas->depth);

	x = canvas->geometry->extra_offscreen_border_left;
	y = canvas->geometry->first_displayed_line;
	
	PSV_SetViewport(x, y, width, height);

	// Needed for palette
	if (!canvas->videoconfig->color_tables.updated){ 
		/* update colors as necessary */
		video_color_update_palette(canvas);
	}
}

video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
	// View already created in video_frame_buffer_alloc. Just get dimensions.
	PSV_GetViewInfo(width, height, 0, 0, &canvas->depth);
	activeCanvas = canvas;

    return canvas;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
	lib_free(canvas->video_draw_buffer_callback);
}

static int video_draw_buffer_alloc(video_canvas_t *canvas,
                                    uint8_t **draw_buffer,
                                    unsigned int fb_width,
                                    unsigned int fb_height,
                                    unsigned int *fb_pitch);

static void video_draw_buffer_free(video_canvas_t *canvas,
                                    uint8_t *draw_buffer);

static void video_draw_buffer_clear(video_canvas_t *canvas,
                                     uint8_t *draw_buffer,
                                     uint8_t value,
                                     unsigned int fb_width,
                                     unsigned int fb_height,
                                     unsigned int fb_pitch);

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
	// We get the best performace by allocating the draw buffer ourselves. 
	// VICE uses indexed palette values as pixel values. We match this format in the View by 
	// allocating an 8 bit indexed image with a palette. This way we can skip the video_canvas_render
	// function fase and get better performance. This is propably not the intended way to integrate but
	// we need more speed.

	canvas->video_draw_buffer_callback = lib_malloc(sizeof(video_draw_buffer_callback_t));
	canvas->video_draw_buffer_callback->draw_buffer_alloc = video_draw_buffer_alloc;
	canvas->video_draw_buffer_callback->draw_buffer_free = video_draw_buffer_free;
	canvas->video_draw_buffer_callback->draw_buffer_clear = video_draw_buffer_clear;
}

static int video_draw_buffer_alloc(video_canvas_t *canvas,
                                    uint8_t **draw_buffer,
                                    unsigned int fb_width,
                                    unsigned int fb_height,
                                    unsigned int *fb_pitch)
{
	PSV_CreateView(fb_width, fb_height, 8);
	PSV_GetViewInfo(0, 0, (unsigned char**)draw_buffer, fb_pitch, 0);
	return 0;
}

static void video_draw_buffer_free(video_canvas_t *canvas, uint8_t *draw_buffer)
{
}

static void video_draw_buffer_clear(video_canvas_t *canvas,
                                     uint8_t *draw_buffer,
                                     uint8_t value,
                                     unsigned int fb_width,
                                     unsigned int fb_height,
                                     unsigned int fb_pitch)
{
	memset(draw_buffer, value, fb_pitch * fb_height);
}

int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
	
	// Send the RGB color palette to the View.
   
	unsigned int i, col = 0, depth;

    if (palette == NULL){
        return 0;
	}

    canvas->palette = palette;

	unsigned char* rgb_byte_arr = (unsigned char*)lib_malloc(palette->num_entries*3);
	memset(rgb_byte_arr, 0, palette->num_entries*3);
	unsigned char* pstart = rgb_byte_arr;

	for (i = 0; i < palette->num_entries; i++) {
		rgb_byte_arr[0] = palette->entries[i].red;
		rgb_byte_arr[1] = palette->entries[i].green;
		rgb_byte_arr[2] = palette->entries[i].blue;
		rgb_byte_arr += 3;
	}

	PSV_NotifyPalette(pstart, palette->num_entries);
	lib_free(pstart);

    return 0; 
}

void video_canvas_refresh(struct video_canvas_s *canvas,
                          unsigned int xs, unsigned int ys,
                          unsigned int xi, unsigned int yi,
                          unsigned int w, unsigned int h)
{
	PSV_UpdateView();
}

int video_init()
{
	return 0;
}

void video_shutdown()
{
}

int video_arch_cmdline_options_init(void)
{
	return cmdline_register_options(cmdline_options);
}

int video_arch_resources_init()
{
	return resources_register_int(resources_int);
}

void video_arch_resources_shutdown()
{
}

void ui_display_speed(float percent, float framerate, int warp_flag)
{
	if (last_framerate != framerate || last_percent != percent || last_warp_flag != warp_flag)
		PSV_NotifyFPS((int)framerate, percent, warp_flag);

	last_framerate = framerate;
	last_percent = percent;
	last_warp_flag = warp_flag;
}

void ui_display_drive_led(int drive_number, unsigned int led_pwm1, unsigned int led_pwm2)
{
	int led_status = (led_pwm1 > 100);

	if (drive_led_on != led_status)
		PSV_NotifyDriveStatus(drive_number + 8, led_status);

	drive_led_on = led_status;
}

void ui_display_tape_motor_status(int motor)
{
	if (tape_led_on != motor)
		PSV_NotifyTapeMotorStatus(motor);

	tape_led_on = motor;
}

char video_canvas_can_resize(struct video_canvas_s *canvas)
{
	return 1;
}

void video_psv_ui_init_finalize(void)
{
	video_canvas_resize(active_canvas, 1);
	PSV_ApplySettings();
}

static void show_menu_trap(uint16_t unused_addr, void *data)
{
	vsync_suspend_speed_eval();
	PSV_ActivateMenu();
	last_framerate = 0;
	last_percent = 0;
	sound_resume();
	resources_set_int("SoundVolume", 100);
} 

static void show_menu_trap_pre(uint16_t unused_addr, void *data)
{
	resources_set_int("SoundVolume", 0);
	interrupt_maincpu_trigger_trap(show_menu_trap, NULL);
} 

void video_psv_menu_show()
{
	// The cpu has to be in interrupt state before showing the main menu. Otherwise all kinds of 
	// weird things can happen like e.g. snaphots won't work and borderless view won't show up.
	interrupt_maincpu_trigger_trap(show_menu_trap, NULL);
}

void video_psv_update_palette()
{
	// Request to update the View palette
	if (!activeCanvas->videoconfig->color_tables.updated){ 
		// Colors need an update.
		video_color_update_palette(activeCanvas);
	}

	video_canvas_set_palette(activeCanvas, activeCanvas->palette);
}

void video_psv_get_canvas(struct video_canvas_s** canvas)
{
	*canvas = activeCanvas;
}
