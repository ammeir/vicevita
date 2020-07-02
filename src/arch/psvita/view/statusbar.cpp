
/* statusbar.cpp: Statusbar implementation.

   Copyright (C) 2019-2021 Amnon-Dan Meir.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: 
     Email: ammeir71@yahoo.com
*/

#include "statusbar.h"
#include "view.h"
#include "texter.h"
#include "resources.h"
#include "app_defs.h"
#include <string.h> // memcpy
#include <vita2d.h>

// Drive mask to drive number lookup
int drive_lookup[16] = {
	0,0,1,0,2,0,0,0,
	3,0,0,0,0,0,0,0
};

Statusbar::Statusbar()
{
	m_updated = false;
	m_driveLedMask = 0;
	m_driveDiskMask = 0;
	m_driveStatusMask = 1; // Drive 8 is the only active at boot.
	m_tapeMotor = 0;
	m_tapeControl = 0;
	m_tapeControlTex = NULL;
	m_lastActiveDrive = 0;
}

Statusbar::~Statusbar()
{
	int i = 0;
	while (m_bitmaps[i]){
		vita2d_free_texture (m_bitmaps[i++]);
	}

}

void Statusbar::init(View* view)
{
	m_view = view;

	loadResources();

	sprintf(m_counter, "000");
	sprintf(m_cpu, "000%%");
	sprintf(m_fps, "00");

	for (int i=0; i<4; ++i){
		m_drives[i].number = i+8;
		m_drives[i].led = 0;
		m_drives[i].bitmask = 0x01 << i;
		sprintf(m_drives[i].track, "00.0"); 
	}
}

void Statusbar::show()
{
	vita2d_start_drawing();
	vita2d_clear_screen();

	render();

	vita2d_end_drawing();
    vita2d_swap_buffers();
}

int Statusbar::render()
{
	// Statusbar layout. Drawing layout is faster than drawing it from scratch.
	vita2d_draw_texture(m_bitmaps[IMG_SB_STATUSBAR], 0, 513);

	// Drive leds and track.
	if (m_driveLedMask){
		if (m_drives[0].led)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 71, 522); 
		if (m_drives[1].led)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 94, 522);
		if (m_drives[2].led)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 117, 522); 
		if (m_drives[3].led)
			vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_RED], 140, 522); 

		txtr_draw_text(243, 534, YELLOW, m_drives[drive_lookup[m_driveLedMask]].track);
	}
	else{
		// No leds on. 
		if (!m_driveDiskMask){
			// No disks at any drive.
			txtr_draw_text(243, 534, YELLOW, "00.0");
		}
		else{
			txtr_draw_text(243, 534, YELLOW, m_drives[drive_lookup[m_lastActiveDrive]].track);
		}
	}

	// Disk presence.
	if (m_driveDiskMask){
		if (m_driveDiskMask & 0x01)
			vita2d_draw_line(76, 538, 82, 538, YELLOW);
		if (m_driveDiskMask & 0x02)
			vita2d_draw_line(99, 538, 105, 538, YELLOW);
		if (m_driveDiskMask & 0x04)
			vita2d_draw_line(122, 538, 128, 538, YELLOW);
		if (m_driveDiskMask & 0x08)
			vita2d_draw_line(145, 538, 151, 538, YELLOW);

	}

	// Disk power status.
	if (m_driveStatusMask){
		if (m_driveStatusMask & 0x01)
			vita2d_draw_rectangle(79, 518, 2, 2, GREEN);
		if (m_driveStatusMask & 0x02)
			vita2d_draw_rectangle(102, 518, 2, 2, GREEN);
		if (m_driveStatusMask & 0x04)
			vita2d_draw_rectangle(125, 518, 2, 2, GREEN);
		if (m_driveStatusMask & 0x08)
			vita2d_draw_rectangle(148, 518, 2, 2, GREEN);
	}

	// Tape control texture.
	if (m_tapeControlTex)
		vita2d_draw_texture(m_tapeControlTex, 374, 520); 

	// Tape counter, fps, cpu percentage.
	txtr_draw_text(453, 534, YELLOW, m_counter);
	txtr_draw_text(567, 534, YELLOW, m_fps);
	txtr_draw_text(668, 534, YELLOW, m_cpu);
	
	// Warp led.
	if (m_warpFlag)
		vita2d_draw_texture(m_bitmaps[IMG_SB_LED_ON_GREEN], 811, 522);

	m_updated = false;
	return 1;
}

void Statusbar::setSpeedData(int fps, int cpu, int warp_flag)
{
	static int prev_fps = 0;
	static int prev_cpu = 0;
	
	m_warpFlag = warp_flag;

	if (cpu != prev_cpu)
		cpu < 100? sprintf(m_cpu, "0%d%%", cpu): sprintf(m_cpu, "%d%%", cpu);
	if (fps != prev_fps)
		fps < 10? sprintf(m_fps, "0%d", fps): sprintf(m_fps, "%d", fps);
	
	prev_fps = fps;
	prev_cpu = cpu;

	m_updated = true;
}

void Statusbar::setDriveLed(int drive, int led)
{
	if (drive > 3)
		return;

	m_drives[drive].led = led;
	m_driveLedMask = led? m_driveLedMask | m_drives[drive].bitmask: 
		                  m_driveLedMask & ~m_drives[drive].bitmask;

	if (led)
		m_lastActiveDrive = drive;

	m_updated = true;
}

void Statusbar::setTapeControl(int control)
{
	string str;

	switch (control){
	case 0:
		str = "Stop"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON]: NULL;
		break;
	case 1: 
		str = "Play"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_START_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_START_MOTOR_OFF];
		break;
	case 2: 
		str = "Forward"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_OFF];
		break;
	case 3: 
		str = "Rewind"; 
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_OFF];
		break;
	case 4: 
		str = "Record";
		m_tapeControlTex = m_tapeMotor? m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_ON]: m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_OFF];
		break;
	case 5: 
		str = "Reset";
		break;
	default: 
		str = "Stop";
		m_tapeControlTex =  m_tapeMotor? m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON]: NULL;
	}

	// Send update to peripherals too. 
	m_view->onSettingChanged(DATASETTE_CONTROL, str.c_str(),0,0,0,1);
	m_tapeControl = control;
	m_updated = true;
}

void Statusbar::setTapeCounter(int counter)
{
	switch (counter){
	case 0 ... 9:
		sprintf(m_counter, "00%d", counter);
		break;
	case 10 ... 99:
		sprintf(m_counter, "0%d", counter);
		break;
	case 100 ... 999:
		sprintf(m_counter, "%d", counter);
		break;
	default:
		sprintf(m_counter, "000", counter);
		break;
	}
	
	m_updated = true;
}

void Statusbar::setTapeMotor(int motor)
{
	m_tapeMotor = motor;
	setTapeControl(m_tapeControl);
}

void Statusbar::setDriveTrack(unsigned int drive, unsigned int half_track)
{
	// half_track: Twice the value of the head location. 18.0 is 36, while 18.5 would be 37.
	if (drive > 3)
		return;

	if (half_track < 20)
		snprintf(m_drives[drive].track, 8, " %.1lf", half_track / 2.0); 
	else
		snprintf(m_drives[drive].track, 8, "%.1lf", half_track / 2.0); 
}

void Statusbar::setDriveDiskPresence(int drive, int disk_in)
{
	m_driveDiskMask = disk_in? m_driveDiskMask | m_drives[drive].bitmask: 
								m_driveDiskMask & ~m_drives[drive].bitmask;

	if (!disk_in)
		sprintf(m_drives[drive].track, "00.0"); 
}

void Statusbar::setDriveStatus(int drive, int active)
{
	if (drive > 3)
		return;

	m_driveStatusMask = active? m_driveStatusMask | m_drives[drive].bitmask: 
								m_driveStatusMask & ~m_drives[drive].bitmask;
}

bool Statusbar::isUpdated()
{
	return m_updated;
}

void Statusbar::notifyReset()
{
	// Computer was reset.

	// Turn off tape motor.
	m_tapeMotor = 0;
	// Set tape control to 'Stop'.
	setTapeControl(0);
}

void Statusbar::loadResources()
{
	m_bitmaps[IMG_SB_STATUSBAR] = vita2d_load_PNG_buffer(img_statusbar);
	m_bitmaps[IMG_SB_LED_ON_GREEN] = vita2d_load_PNG_buffer(img_led_on_green);
	m_bitmaps[IMG_SB_LED_ON_RED] = vita2d_load_PNG_buffer(img_led_on_red);
	m_bitmaps[IMG_SB_LED_OFF] = vita2d_load_PNG_buffer(img_led_off);
	m_bitmaps[IMG_SB_TAPE_STOP_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_stop_motor_on);
	m_bitmaps[IMG_SB_TAPE_START_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_start_motor_on);
	m_bitmaps[IMG_SB_TAPE_START_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_start_motor_off);
	m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_forward_motor_on);
	m_bitmaps[IMG_SB_TAPE_FORWARD_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_forward_motor_off);
	m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_rewind_motor_on);
	m_bitmaps[IMG_SB_TAPE_REWIND_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_rewind_motor_off);
	m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_ON] = vita2d_load_PNG_buffer(img_tape_record_motor_on);
	m_bitmaps[IMG_SB_TAPE_RECORD_MOTOR_OFF] = vita2d_load_PNG_buffer(img_tape_record_motor_off);
	m_bitmaps[IMG_SB_NULL] = NULL;
}
